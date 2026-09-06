#include "CudaMemoryManager.h"

#include <iostream>

namespace jetson_receiver
{
    namespace
    {
        bool checkCudaError(cudaError_t pError, const char* pContext)
        {
            if (pError == cudaSuccess)
            {
                return true;
            }

            std::cerr << "[CudaMemoryManager] " << pContext << " failed: "
                      << cudaGetErrorString(pError) << std::endl;
            return false;
        }
    }

    CudaMemoryManager::CudaMemoryManager()
        : mDeviceBuffer(nullptr)
        , mCapacity(0)
        , mStream(nullptr)
        , mIsInitialized(false)
        , mRegisteredHostBlocks()
    {
    }

    CudaMemoryManager::~CudaMemoryManager()
    {
        shutdown();
    }

    bool CudaMemoryManager::initialize()
    {
        if (mIsInitialized)
        {
            return true;
        }

        if (!checkCudaError(cudaSetDevice(0), "cudaSetDevice"))
        {
            return false;
        }

        // Non-blocking stream: default stream ile implicit senkronizasyon olmasın.
        if (!checkCudaError(cudaStreamCreateWithFlags(&mStream, cudaStreamNonBlocking), "cudaStreamCreateWithFlags"))
        {
            return false;
        }

        mIsInitialized = true;
        return true;
    }

    void CudaMemoryManager::shutdown()
    {
        if (!mIsInitialized)
        {
            return;
        }

        unregisterAllHostMemory();

        if (mDeviceBuffer != nullptr)
        {
            cudaFree(mDeviceBuffer);
            mDeviceBuffer = nullptr;
            mCapacity = 0;
        }

        if (mStream != nullptr)
        {
            cudaStreamDestroy(mStream);
            mStream = nullptr;
        }

        mIsInitialized = false;
    }

    bool CudaMemoryManager::ensureCapacity(size_t pByteCount)
    {
        if (!mIsInitialized)
        {
            return false;
        }

        if (pByteCount <= mCapacity)
        {
            return true;
        }

        if (mDeviceBuffer != nullptr)
        {
            cudaStreamSynchronize(mStream);
            cudaFree(mDeviceBuffer);
            mDeviceBuffer = nullptr;
            mCapacity = 0;
        }

        void* tPointer = nullptr;

        if (!checkCudaError(cudaMallocManaged(&tPointer, pByteCount, cudaMemAttachGlobal), "cudaMallocManaged"))
        {
            return false;
        }

        mDeviceBuffer = static_cast<uint8_t*>(tPointer);
        mCapacity = pByteCount;

        // Bu bellegi yalnızca bu stream kullanacak -> daha az migration overhead'i.
        cudaStreamAttachMemAsync(mStream, mDeviceBuffer, 0, cudaMemAttachSingle);
        cudaStreamSynchronize(mStream);

        std::cout << "[CudaMemoryManager] Managed buffer allocated: " << mCapacity << " bytes" << std::endl;
        return true;
    }

    bool CudaMemoryManager::uploadAsync2D(const uint8_t* pHostData, size_t pSourcePitch, size_t pRowByteCount, size_t pRowCount)
    {
        if ((!mIsInitialized) || (mDeviceBuffer == nullptr) || (pHostData == nullptr))
        {
            return false;
        }

        const size_t tRequiredBytes = pRowByteCount * pRowCount;

        if (tRequiredBytes > mCapacity)
        {
            std::cerr << "[CudaMemoryManager] Buffer too small: need " << tRequiredBytes
                      << " have " << mCapacity << std::endl;
            return false;
        }

        const cudaError_t tError = cudaMemcpy2DAsync(mDeviceBuffer,
                                                     pRowByteCount,
                                                     pHostData,
                                                     pSourcePitch,
                                                     pRowByteCount,
                                                     pRowCount,
                                                     cudaMemcpyHostToDevice,
                                                     mStream);

        return checkCudaError(tError, "cudaMemcpy2DAsync");
    }

    bool CudaMemoryManager::synchronize()
    {
        if (!mIsInitialized)
        {
            return false;
        }

        return checkCudaError(cudaStreamSynchronize(mStream), "cudaStreamSynchronize");
    }

    bool CudaMemoryManager::registerHostMemory(void* pHostPointer, size_t pByteCount)
    {
        if ((!mIsInitialized) || (pHostPointer == nullptr))
        {
            return false;
        }

        const std::unordered_map<void*, size_t>::const_iterator tIterator = mRegisteredHostBlocks.find(pHostPointer);

        if (tIterator != mRegisteredHostBlocks.end())
        {
            // Aynı adres farklı boyutla geldiyse yeniden kaydetmeye calısma.
            return (tIterator->second >= pByteCount);
        }

        const cudaError_t tError = cudaHostRegister(pHostPointer, pByteCount, cudaHostRegisterDefault);

        if (tError != cudaSuccess)
        {
            // Kayıt basarısız olsa da kopyalama calısır, sadece daha yavas olur.
            cudaGetLastError();
            return false;
        }

        mRegisteredHostBlocks[pHostPointer] = pByteCount;
        return true;
    }

    void CudaMemoryManager::unregisterAllHostMemory()
    {
        for (const std::pair<void* const, size_t>& tEntry : mRegisteredHostBlocks)
        {
            cudaHostUnregister(tEntry.first);
        }

        mRegisteredHostBlocks.clear();
        cudaGetLastError();
    }

    uint8_t* CudaMemoryManager::getDevicePointer() const
    {
        return mDeviceBuffer;
    }

    size_t CudaMemoryManager::getCapacity() const
    {
        return mCapacity;
    }

    cudaStream_t CudaMemoryManager::getStream() const
    {
        return mStream;
    }

    void CudaMemoryManager::printDeviceInfo()
    {
        cudaDeviceProp tProperties;

        if (cudaGetDeviceProperties(&tProperties, 0) != cudaSuccess)
        {
            std::cerr << "[CudaMemoryManager] cudaGetDeviceProperties failed." << std::endl;
            return;
        }

        std::cout << "[CudaMemoryManager] Device        : " << tProperties.name << std::endl;
        std::cout << "[CudaMemoryManager] Compute cap.  : " << tProperties.major << "." << tProperties.minor << std::endl;
        std::cout << "[CudaMemoryManager] Integrated    : " << (tProperties.integrated ? "yes" : "no") << std::endl;
        std::cout << "[CudaMemoryManager] Unified addr. : " << (tProperties.unifiedAddressing ? "yes" : "no") << std::endl;
        std::cout << "[CudaMemoryManager] Managed mem.  : " << (tProperties.managedMemory ? "yes" : "no") << std::endl;
    }
}
