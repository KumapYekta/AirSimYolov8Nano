#include "AppSrcFrameProvider.h"

#include <cstring>
#include <iostream>
#include <vector>

namespace airsim_yolov8
{
    AppSrcFrameProvider::AppSrcFrameProvider(GStreamerSenderPipelineManager& pPipelineManager)
        : mPipelineManager(pPipelineManager)
        , mStreamStartTime()
        , mPushedFrameCount(0)
        , mDroppedFrameCount(0)
        , mPushedByteCount(0)
        , mIsFirstFrame(true)
    {
    }

    void AppSrcFrameProvider::destroyOwnedVector(gpointer pUserData)
    {
        std::vector<uint8_t>* tOwnedData = static_cast<std::vector<uint8_t>*>(pUserData);
        delete tOwnedData;
    }

    GstBuffer* AppSrcFrameProvider::createBufferZeroCopy(CameraFrame& pFrame)
    {
        // Vektor heap'e tasınır, GstBuffer onun sahibi olur ve
        // referans sayısı sıfırlanınca destroyOwnedVector cagrılır.
        std::vector<uint8_t>* tOwnedData = new std::vector<uint8_t>(std::move(pFrame.mData));
        const gsize tByteCount = static_cast<gsize>(tOwnedData->size());

        GstBuffer* tBuffer = gst_buffer_new_wrapped_full(GST_MEMORY_FLAG_READONLY,
                                                         tOwnedData->data(),
                                                         tByteCount,
                                                         0,
                                                         tByteCount,
                                                         tOwnedData,
                                                         &AppSrcFrameProvider::destroyOwnedVector);

        if (tBuffer == nullptr)
        {
            delete tOwnedData;
        }

        return tBuffer;
    }

    GstBuffer* AppSrcFrameProvider::createBufferWithCopy(const CameraFrame& pFrame)
    {
        const gsize tByteCount = static_cast<gsize>(pFrame.mData.size());
        GstBuffer* tBuffer = gst_buffer_new_allocate(nullptr, tByteCount, nullptr);

        if (tBuffer == nullptr)
        {
            return nullptr;
        }

        GstMapInfo tMapInfo;

        if (gst_buffer_map(tBuffer, &tMapInfo, GST_MAP_WRITE) == FALSE)
        {
            gst_buffer_unref(tBuffer);
            return nullptr;
        }

        std::memcpy(tMapInfo.data, pFrame.mData.data(), tByteCount);
        gst_buffer_unmap(tBuffer, &tMapInfo);

        return tBuffer;
    }

    uint64_t AppSrcFrameProvider::computePresentationTimestamp()
    {
        const std::chrono::steady_clock::time_point tNow = std::chrono::steady_clock::now();

        if (mIsFirstFrame)
        {
            mStreamStartTime = tNow;
            mIsFirstFrame = false;
            return 0;
        }

        const std::chrono::nanoseconds tElapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(tNow - mStreamStartTime);
        return static_cast<uint64_t>(tElapsed.count());
    }

    bool AppSrcFrameProvider::pushFrame(CameraFrame& pFrame)
    {
        GstAppSrc* tAppSrc = mPipelineManager.getAppSrc();

        if ((tAppSrc == nullptr) || (!mPipelineManager.isRunning()))
        {
            mDroppedFrameCount = mDroppedFrameCount + 1;
            return false;
        }

        if (!pFrame.isValid())
        {
            mDroppedFrameCount = mDroppedFrameCount + 1;
            return false;
        }

        const StreamConfig& tConfig = mPipelineManager.getConfig();
        const size_t tExpectedByteCount = static_cast<size_t>(tConfig.mWidth) * static_cast<size_t>(tConfig.mHeight) * static_cast<size_t>(tConfig.mChannelCount);

        if (pFrame.mData.size() != tExpectedByteCount)
        {
            std::cerr << "[AppSrcFrameProvider] Size mismatch. expected=" << tExpectedByteCount
                      << " got=" << pFrame.mData.size() << std::endl;
            mDroppedFrameCount = mDroppedFrameCount + 1;
            return false;
        }

        const size_t tByteCount = pFrame.mData.size();

        GstBuffer* tBuffer = nullptr;

        if (tConfig.mUseZeroCopyBuffers)
        {
            tBuffer = createBufferZeroCopy(pFrame);
        }
        else
        {
            tBuffer = createBufferWithCopy(pFrame);
        }

        if (tBuffer == nullptr)
        {
            mDroppedFrameCount = mDroppedFrameCount + 1;
            return false;
        }

        const uint64_t tPts = computePresentationTimestamp();
        const int tFps = (tConfig.mTargetFps > 0) ? tConfig.mTargetFps : 30;

        GST_BUFFER_PTS(tBuffer) = static_cast<GstClockTime>(tPts);
        GST_BUFFER_DTS(tBuffer) = static_cast<GstClockTime>(tPts);
        GST_BUFFER_DURATION(tBuffer) = gst_util_uint64_scale_int(GST_SECOND, 1, tFps);
        GST_BUFFER_OFFSET(tBuffer) = mPushedFrameCount;

        const GstFlowReturn tFlowReturn = gst_app_src_push_buffer(tAppSrc, tBuffer);

        if (tFlowReturn != GST_FLOW_OK)
        {
            std::cerr << "[AppSrcFrameProvider] push_buffer returned " << gst_flow_get_name(tFlowReturn) << std::endl;
            mDroppedFrameCount = mDroppedFrameCount + 1;
            return false;
        }

        mPushedFrameCount = mPushedFrameCount + 1;
        mPushedByteCount = mPushedByteCount + tByteCount;

        return true;
    }

    uint64_t AppSrcFrameProvider::getPushedFrameCount() const
    {
        return mPushedFrameCount;
    }

    uint64_t AppSrcFrameProvider::getDroppedFrameCount() const
    {
        return mDroppedFrameCount;
    }

    uint64_t AppSrcFrameProvider::getPushedByteCount() const
    {
        return mPushedByteCount;
    }
}
