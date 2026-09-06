#pragma once

#include "CameraFrame.h"
#include "GStreamerSenderPipelineManager.h"

#include <chrono>
#include <cstdint>

namespace airsim_yolov8
{
    // CameraFrame -> GstBuffer -> appsrc.
    // Iki mod var:
    //   zero-copy : vektorun sahipligi GstBuffer'a devredilir, hic kopyalama olmaz
    //   memcpy    : gst_buffer_map + std::memcpy
    class AppSrcFrameProvider
    {
    public:
        explicit AppSrcFrameProvider(GStreamerSenderPipelineManager& pPipelineManager);

        // pFrame non-const, cunku zero-copy modda mData tasınır.
        bool pushFrame(CameraFrame& pFrame);

        uint64_t getPushedFrameCount() const;
        uint64_t getDroppedFrameCount() const;
        uint64_t getPushedByteCount() const;

    private:
        GstBuffer* createBufferZeroCopy(CameraFrame& pFrame);
        GstBuffer* createBufferWithCopy(const CameraFrame& pFrame);
        uint64_t computePresentationTimestamp();

        static void destroyOwnedVector(gpointer pUserData);

        GStreamerSenderPipelineManager& mPipelineManager;
        std::chrono::steady_clock::time_point mStreamStartTime;
        uint64_t mPushedFrameCount;
        uint64_t mDroppedFrameCount;
        uint64_t mPushedByteCount;
        bool mIsFirstFrame;
    };
}
