#include "CudaMemoryManager.h"
#include "common/FrameStatisticsManager.h"
#include "GStreamerReceiverPipelineManager.h"
#include "common/GStreamerRuntimeManager.h"
#include "GpuFrame.h"
#include "GpuFrameProvider.h"
#include "ReceiverConfigProvider.h"

#include <atomic>
#include <csignal>
#include <iostream>

namespace
{
    std::atomic<bool> gIsRunning{ true };

    void signalHandler(int pSignal)
    {
        (void)pSignal;
        gIsRunning = false;
    }
}

int main(int argc, char** argv)
{
    std::signal(SIGINT, signalHandler);

    if (!common::GStreamerRuntimeManager::initialize(&argc, &argv))
    {
        return 1;
    }

    jetson_receiver::ReceiverConfigProvider tConfigProvider;
    tConfigProvider.loadFromEnvironment();
    tConfigProvider.printSummary();

    jetson_receiver::CudaMemoryManager::printDeviceInfo();

    jetson_receiver::CudaMemoryManager tMemoryManager;

    if (!tMemoryManager.initialize())
    {
        std::cerr << "[main] CUDA initialization failed." << std::endl;
        return 1;
    }

    jetson_receiver::GStreamerReceiverPipelineManager tPipelineManager;

    if (!tPipelineManager.build(tConfigProvider.getConfig()))
    {
        return 1;
    }

    if (!tPipelineManager.start())
    {
        return 1;
    }

    jetson_receiver::GpuFrameProvider tFrameProvider(tPipelineManager, tMemoryManager);
    common::FrameStatisticsManager tStatisticsManager(1.0);

    jetson_receiver::GpuFrame tGpuFrame;
    bool tIsFirstFrameLogged = false;

    while (gIsRunning)
    {
        if (!tPipelineManager.pollBus())
        {
            std::cerr << "[main] Pipeline reported a fatal error." << std::endl;
            break;
        }

        if (!tFrameProvider.fetchFrameToGpu(tGpuFrame, false))
        {
            continue;
        }

        tStatisticsManager.onFrameReceived();

        if (!tIsFirstFrameLogged)
        {
            std::cout << "[main] First GPU frame ready." << std::endl;
            std::cout << "[main]   resolution : " << tGpuFrame.mWidth << "x" << tGpuFrame.mHeight << std::endl;
            std::cout << "[main]   channels   : " << tGpuFrame.mChannelCount << std::endl;
            std::cout << "[main]   bytes      : " << tGpuFrame.mByteCount << std::endl;
            std::cout << "[main]   device ptr : " << static_cast<void*>(tGpuFrame.mDevicePointer) << std::endl;

            tIsFirstFrameLogged = true;
        }

        // ---------------------------------------------------------------------
        // Buradan sonrası yapay zeka asamasına ait.
        // tGpuFrame.mDevicePointer dogrudan TensorRT input binding'i olarak ya da
        // preprocess CUDA kernel'ine girdi olarak verilebilir. Aynı stream'i
        // (tGpuFrame.mStream) kullanırsan ekstra senkronizasyon gerekmez.
        //
        //   myPreprocessKernel<<<tGrid, tBlock, 0, tGpuFrame.mStream>>>(
        //       tGpuFrame.mDevicePointer, tGpuFrame.mWidth, tGpuFrame.mHeight);
        // ---------------------------------------------------------------------

        if (tStatisticsManager.shouldReport())
        {
            std::cout << "[main] fps: " << tStatisticsManager.getLastFps()
                      << " | received: " << tFrameProvider.getReceivedFrameCount()
                      << " | failed: " << tFrameProvider.getFailedFrameCount()
                      << " | pts: " << tGpuFrame.mPtsNs
                      << std::endl;
        }
    }

    std::cout << "[main] Shutting down. Total frames: " << tStatisticsManager.getTotalFrameCount() << std::endl;

    tPipelineManager.stop();
    tMemoryManager.shutdown();
    common::GStreamerRuntimeManager::shutdown();

    return 0;
}
