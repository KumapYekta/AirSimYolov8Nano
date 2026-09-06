#pragma once

#include <cuda_runtime.h>

#include <cstddef>
#include <cstdint>
#include <unordered_map>

namespace jetson_receiver
{
    // CUDA bellek yasam dongusu ve async kopyalar.
    // Jetson'da iGPU oldugu icin cudaMallocManaged fiziksel olarak paylasımlı
    // bellege dusuyor, bu yuzden ekstra bir device kopyası olusmuyor.
    class CudaMemoryManager
    {
    public:
        CudaMemoryManager();
        ~CudaMemoryManager();

        CudaMemoryManager(const CudaMemoryManager&) = delete;
        CudaMemoryManager& operator=(const CudaMemoryManager&) = delete;

        bool initialize();
        void shutdown();

        // Gerekirse buyutur, aynı boyutta tekrar allocate etmez.
        bool ensureCapacity(size_t pByteCount);

        // Satır satır kopya. Kaynak stride hedef stride'dan farklı olabilir
        // (nvvidconv cıktısı padding'li gelebiliyor).
        bool uploadAsync2D(const uint8_t* pHostData, size_t pSourcePitch, size_t pRowByteCount, size_t pRowCount);

        bool synchronize();

        // GStreamer buffer pool aynı adresleri tekrar kullandıgı icin
        // ilk goruste pinned yapıp cache'liyoruz -> sonraki kopyalar DMA.
        bool registerHostMemory(void* pHostPointer, size_t pByteCount);
        void unregisterAllHostMemory();

        uint8_t* getDevicePointer() const;
        size_t getCapacity() const;
        cudaStream_t getStream() const;

        static void printDeviceInfo();

    private:
        uint8_t* mDeviceBuffer;
        size_t mCapacity;
        cudaStream_t mStream;
        bool mIsInitialized;
        std::unordered_map<void*, size_t> mRegisteredHostBlocks;
    };
}
