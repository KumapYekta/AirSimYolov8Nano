#include "GpuFrameProvider.h"

#include <iostream>

namespace jetson_receiver
{
    GpuFrameProvider::GpuFrameProvider(GStreamerReceiverPipelineManager& pPipelineManager, CudaMemoryManager& pMemoryManager)
        : mPipelineManager(pPipelineManager)
        , mMemoryManager(pMemoryManager)
        , mVideoInfo()
        , mIsVideoInfoValid(false)
        , mReceivedFrameCount(0)
        , mFailedFrameCount(0)
    {
        gst_video_info_init(&mVideoInfo);
    }

    bool GpuFrameProvider::readVideoInfo(GstCaps* pCaps)
    {
        if (pCaps == nullptr)
        {
            return false;
        }

        if (gst_video_info_from_caps(&mVideoInfo, pCaps) == FALSE)
        {
            std::cerr << "[GpuFrameProvider] Could not parse video info from caps." << std::endl;
            return false;
        }

        mIsVideoInfoValid = true;

        std::cout << "[GpuFrameProvider] Negotiated: "
                  << GST_VIDEO_INFO_WIDTH(&mVideoInfo) << "x" << GST_VIDEO_INFO_HEIGHT(&mVideoInfo)
                  << " format=" << GST_VIDEO_INFO_NAME(&mVideoInfo)
                  << " stride=" << GST_VIDEO_INFO_PLANE_STRIDE(&mVideoInfo, 0)
                  << std::endl;

        return true;
    }

    bool GpuFrameProvider::fetchFrameToGpu(GpuFrame& pOutFrame, bool pShouldSynchronize)
    {
        GstAppSink* tAppSink = mPipelineManager.getAppSink();

        if ((tAppSink == nullptr) || (!mPipelineManager.isRunning()))
        {
            return false;
        }

        const ReceiverConfig& tConfig = mPipelineManager.getConfig();
        const GstClockTime tTimeout = static_cast<GstClockTime>(tConfig.mPullTimeoutMs) * GST_MSECOND;

        GstSample* tSample = gst_app_sink_try_pull_sample(tAppSink, tTimeout);

        if (tSample == nullptr)
        {
            return false;
        }

        GstCaps* tCaps = gst_sample_get_caps(tSample);

        if ((!mIsVideoInfoValid) && (!readVideoInfo(tCaps)))
        {
            gst_sample_unref(tSample);
            mFailedFrameCount = mFailedFrameCount + 1;
            return false;
        }

        GstBuffer* tBuffer = gst_sample_get_buffer(tSample);

        if (tBuffer == nullptr)
        {
            gst_sample_unref(tSample);
            mFailedFrameCount = mFailedFrameCount + 1;
            return false;
        }

        // gst_video_frame_map stride/offset bilgisini de verir,
        // nvvidconv cıktısı padding'li gelebildigi icin duz map yerine bunu kullanıyoruz.
        GstVideoFrame tVideoFrame;

        if (gst_video_frame_map(&tVideoFrame, &mVideoInfo, tBuffer, GST_MAP_READ) == FALSE)
        {
            gst_sample_unref(tSample);
            mFailedFrameCount = mFailedFrameCount + 1;
            return false;
        }

        const int tWidth = GST_VIDEO_FRAME_WIDTH(&tVideoFrame);
        const int tHeight = GST_VIDEO_FRAME_HEIGHT(&tVideoFrame);
        const int tComponentCount = GST_VIDEO_FRAME_N_COMPONENTS(&tVideoFrame);
        const size_t tSourcePitch = static_cast<size_t>(GST_VIDEO_FRAME_PLANE_STRIDE(&tVideoFrame, 0));
        const size_t tRowByteCount = static_cast<size_t>(tWidth) * static_cast<size_t>(tComponentCount);
        const size_t tTotalByteCount = tRowByteCount * static_cast<size_t>(tHeight);

        uint8_t* tHostPointer = static_cast<uint8_t*>(GST_VIDEO_FRAME_PLANE_DATA(&tVideoFrame, 0));

        if (!mMemoryManager.ensureCapacity(tTotalByteCount))
        {
            gst_video_frame_unmap(&tVideoFrame);
            gst_sample_unref(tSample);
            mFailedFrameCount = mFailedFrameCount + 1;
            return false;
        }

        if (tConfig.mUseHostRegister)
        {
            // Buffer pool aynı adresleri dondugu icin bu ilk birkac frame'den
            // sonra hep cache hit olur, kopyalar pinned/DMA yoluna duser.
            mMemoryManager.registerHostMemory(tHostPointer, tSourcePitch * static_cast<size_t>(tHeight));
        }

        const bool tIsUploaded = mMemoryManager.uploadAsync2D(tHostPointer, tSourcePitch, tRowByteCount, static_cast<size_t>(tHeight));

        if (!tIsUploaded)
        {
            gst_video_frame_unmap(&tVideoFrame);
            gst_sample_unref(tSample);
            mFailedFrameCount = mFailedFrameCount + 1;
            return false;
        }

        // Kopya bitmeden GstBuffer'ı serbest bırakamayız.
        mMemoryManager.synchronize();

        pOutFrame.mDevicePointer = mMemoryManager.getDevicePointer();
        pOutFrame.mStream = mMemoryManager.getStream();
        pOutFrame.mWidth = tWidth;
        pOutFrame.mHeight = tHeight;
        pOutFrame.mChannelCount = tComponentCount;
        pOutFrame.mPitchBytes = tRowByteCount;
        pOutFrame.mByteCount = tTotalByteCount;
        pOutFrame.mPtsNs = static_cast<uint64_t>(GST_BUFFER_PTS(tBuffer));
        pOutFrame.mFrameIndex = mReceivedFrameCount;

        gst_video_frame_unmap(&tVideoFrame);
        gst_sample_unref(tSample);

        if (pShouldSynchronize)
        {
            mMemoryManager.synchronize();
        }

        mReceivedFrameCount = mReceivedFrameCount + 1;
        return true;
    }

    uint64_t GpuFrameProvider::getReceivedFrameCount() const
    {
        return mReceivedFrameCount;
    }

    uint64_t GpuFrameProvider::getFailedFrameCount() const
    {
        return mFailedFrameCount;
    }
}
