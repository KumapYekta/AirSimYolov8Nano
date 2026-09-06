#include "AirSimConnectionManager.h"
#include "AppConfigProvider.h"
#include "AppSrcFrameProvider.h"
#include "CameraFrame.h"
#include "CameraImageProvider.h"
#include "common/FrameStatisticsManager.h"
#include "common/GStreamerRuntimeManager.h"
#include "GStreamerSenderPipelineManager.h"

#include <atomic>
#include <csignal>
#include <iostream>
#include <vector>

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

    airsim_yolov8::AppConfigProvider tConfigProvider;
    tConfigProvider.loadFromEnvironment();
    tConfigProvider.printSummary();

    airsim_yolov8::AirSimConnectionManager tConnectionManager(tConfigProvider.getAirSimHost(), tConfigProvider.getAirSimPort());

    if (!tConnectionManager.connect())
    {
        std::cerr << "[main] Could not connect to AirSim." << std::endl;
        return 1;
    }

    tConnectionManager.printVehicleNames();

    airsim_yolov8::CameraImageProvider tImageProvider(tConnectionManager, tConfigProvider.getVehicleName());
    tImageProvider.getRequestProvider().addSceneRequest(tConfigProvider.getCameraName(), false);

    airsim_yolov8::GStreamerSenderPipelineManager tPipelineManager;
    airsim_yolov8::AppSrcFrameProvider tFrameProvider(tPipelineManager);
    common::FrameStatisticsManager tStatisticsManager(1.0);

    std::vector<airsim_yolov8::CameraFrame> tFrames;
    bool tIsPipelineReady = false;

    while (gIsRunning)
    {
        if (!tImageProvider.fetchFrames(tFrames))
        {
            continue;
        }

        airsim_yolov8::CameraFrame& tFrame = tFrames[0];

        // Pipeline'ı ilk frame'in gercek cozunurlugu ile kuruyoruz,
        // boylece AirSim ayarları degisse bile caps dogru olur.
        if (!tIsPipelineReady)
        {
            airsim_yolov8::StreamConfig tStreamConfig = tConfigProvider.getStreamConfig();
            tStreamConfig.mWidth = tFrame.mWidth;
            tStreamConfig.mHeight = tFrame.mHeight;
            tStreamConfig.mChannelCount = tFrame.mChannelCount;

            std::cout << "[main] First frame: " << tFrame.mWidth << "x" << tFrame.mHeight
                      << " channels=" << tFrame.mChannelCount
                      << " bytes=" << tFrame.getByteCount() << std::endl;

            if ((tFrame.mChannelCount == 3) && (((tFrame.mWidth * 3) % 4) != 0))
            {
                std::cerr << "[main] WARNING: width*3 is not 4-byte aligned, GStreamer stride may mismatch." << std::endl;
            }

            if (!tPipelineManager.build(tStreamConfig))
            {
                return 1;
            }

            if (!tPipelineManager.start())
            {
                return 1;
            }

            tIsPipelineReady = true;
        }

        // Dikkat: zero-copy modda pushFrame tFrame.mData'yı tasır,
        // bu cagrıdan sonra frame'in verisi bos olur.
        // TODO: bu boolean neden boş yani false ya da true return etmesi sanki hiçbir şeyi değiştirmiyor ve her türlü framereceived çağrılacak gibi.
        tFrameProvider.pushFrame(tFrame);
        tStatisticsManager.onFrameReceived();

        if (!tPipelineManager.pollBus())
        {
            std::cerr << "[main] Pipeline reported a fatal error." << std::endl;
            break;
        }

        if (tStatisticsManager.shouldReport())
        {
            std::cout << "[main] fps: " << tStatisticsManager.getLastFps()
                      << " | pushed: " << tFrameProvider.getPushedFrameCount()
                      << " | dropped: " << tFrameProvider.getDroppedFrameCount()
                      << " | MB sent(raw): " << (tFrameProvider.getPushedByteCount() / (1024 * 1024))
                      << std::endl;
        }
    }

    std::cout << "[main] Shutting down." << std::endl;

    tPipelineManager.stop();
    tConnectionManager.disconnect();
    common::GStreamerRuntimeManager::shutdown();

    return 0;
}
