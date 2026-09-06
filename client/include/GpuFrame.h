#pragma once

#include <cuda_runtime.h>

#include <cstdint>

namespace jetson_receiver
{
    // GPU tarafında duran frame'in tanımı.
    // mDevicePointer unified memory oldugu icin hem host hem device'tan erisilebilir,
    // ama host'tan okumadan once stream senkronizasyonu sart.
    struct GpuFrame
    {
        uint8_t* mDevicePointer = nullptr;
        cudaStream_t mStream = nullptr;

        int mWidth = 0;
        int mHeight = 0;
        int mChannelCount = 0;
        size_t mPitchBytes = 0;
        size_t mByteCount = 0;
        uint64_t mPtsNs = 0;
        uint64_t mFrameIndex = 0;

        bool isValid() const
        {
            return (mDevicePointer != nullptr) && (mWidth > 0) && (mHeight > 0);
        }
    };
}
