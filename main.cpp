#include "AirSimConnectionManager.h"
#include "CameraFrame.h"
#include "CameraImageProvider.h"
#include "FrameStatisticsManager.h"

#include <atomic>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace
{
    std::atomic<bool> gIsRunning{ true };

    void signalHandler(int pSignal)
    {
        (void)pSignal;
        gIsRunning = false;
    }

    std::string getEnvironmentOrDefault(const char* pName, const std::string& pDefaultValue)
    {
        const char* tValue = std::getenv(pName);

        if (tValue == nullptr)
        {
            return pDefaultValue;
        }

        return std::string(tValue);
    }
}

int main()
{
    std::signal(SIGINT, signalHandler);

    const std::string tHost = getEnvironmentOrDefault("AIRSIM_HOST", "127.0.0.1");
    const uint16_t tPort = static_cast<uint16_t>(std::stoi(getEnvironmentOrDefault("AIRSIM_PORT", "41451")));
    const std::string tVehicleName = getEnvironmentOrDefault("AIRSIM_VEHICLE", "SimpleFlight");
    const std::string tCameraName = getEnvironmentOrDefault("AIRSIM_CAMERA", "0");

    airsim_yolov8::AirSimConnectionManager tConnectionManager(tHost, tPort);

    if (!tConnectionManager.connect())
    {
        std::cerr << "[main] Could not connect to AirSim." << std::endl;
        return 1;
    }

    tConnectionManager.printVehicleNames();

    airsim_yolov8::CameraImageProvider tImageProvider(tConnectionManager, tVehicleName);

    // Sıkıstırma kapalı: ham BGR byte dizisi geliyor.
    // Ilerideki adımda bu buffer dogrudan cudaMemcpy ile GPU'ya gidecek.
    tImageProvider.getRequestProvider().addSceneRequest(tCameraName, false);

    airsim_yolov8::FrameStatisticsManager tStatisticsManager(1.0);

    std::vector<airsim_yolov8::CameraFrame> tFrames;
    bool tIsFirstFrameLogged = false;

    while (gIsRunning)
    {
        if (!tImageProvider.fetchFrames(tFrames))
        {
            continue;
        }

        tStatisticsManager.onFrameReceived();

        if (!tIsFirstFrameLogged)
        {
            const airsim_yolov8::CameraFrame& tFrame = tFrames[0];

            std::cout << "[main] First frame received." << std::endl;
            std::cout << "[main]   camera      : '" << tFrame.mCameraName << "'" << std::endl;
            std::cout << "[main]   resolution  : " << tFrame.mWidth << "x" << tFrame.mHeight << std::endl;
            std::cout << "[main]   channels    : " << tFrame.mChannelCount << std::endl;
            std::cout << "[main]   bytes       : " << tFrame.getByteCount() << std::endl;
            std::cout << "[main]   compressed  : " << (tFrame.mIsCompressed ? "true" : "false") << std::endl;
            std::cout << "[main]   timestamp   : " << tFrame.mTimestampNs << std::endl;

            tIsFirstFrameLogged = true;
        }

        if (tStatisticsManager.shouldReport())
        {
            const airsim_yolov8::CameraFrame& tFrame = tFrames[0];

            std::cout << "[main] fps: " << tStatisticsManager.getLastFps()
                      << " | total: " << tStatisticsManager.getTotalFrameCount()
                      << " | " << tFrame.mWidth << "x" << tFrame.mHeight
                      << " | bytes: " << tFrame.getByteCount()
                      << " | ts: " << tFrame.mTimestampNs << std::endl;
        }
    }

    std::cout << "[main] Shutting down. Total frames: " << tStatisticsManager.getTotalFrameCount() << std::endl;

    tConnectionManager.disconnect();
    return 0;
}
