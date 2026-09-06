#pragma once

#include "CudaMemoryManager.h"
#include "GStreamerReceiverPipelineManager.h"
#include "GpuFrame.h"

#include <gst/video/video.h>

#include <cstdint>

namespace jetson_receiver
{
    // appsink'ten sample ceker, video meta'sını okur ve
    // cudaMemcpy2DAsync ile unified memory'ye tasır.
    // Donen GpuFrame dogrudan TensorRT / CUDA kernel'e verilebilir.
    class GpuFrameProvider
    {
    public:
        GpuFrameProvider(GStreamerReceiverPipelineManager& pPipelineManager, CudaMemoryManager& pMemoryManager);

        // Timeout icinde sample gelmezse false doner.
        // pShouldSynchronize false ise kopya asenkron kalır; kernel'i aynı
        // stream'de calıstıracaksan senkronizasyona gerek yok.
        bool fetchFrameToGpu(GpuFrame& pOutFrame, bool pShouldSynchronize = false);

        uint64_t getReceivedFrameCount() const;
        uint64_t getFailedFrameCount() const;

    private:
        bool readVideoInfo(GstCaps* pCaps);

        GStreamerReceiverPipelineManager& mPipelineManager;
        CudaMemoryManager& mMemoryManager;

        GstVideoInfo mVideoInfo;
        bool mIsVideoInfoValid;

        uint64_t mReceivedFrameCount;
        uint64_t mFailedFrameCount;
    };
}
