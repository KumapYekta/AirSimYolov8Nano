#pragma once

#include "AirSimConnectionManager.h"
#include "CameraFrame.h"
#include "ImageRequestProvider.h"

#include <string>
#include <vector>

namespace airsim_yolov8
{
    // simGetImages call yapıp AirSim cevabını proje ici CameraFrame
    // yapısına cevirir. Baglantıyı sahiplenmez, sadece kullanır.
    class CameraImageProvider
    {
    public:
        CameraImageProvider(AirSimConnectionManager& pConnectionManager, const std::string& pVehicleName);

        ImageRequestProvider& getRequestProvider();

        // Image Response vector'unu return eder.
        bool fetchFrames(std::vector<CameraFrame>& pOutFrames);

        uint64_t getFetchedFrameCount() const;

    private:
        AirSimConnectionManager& mConnectionManager;
        ImageRequestProvider mRequestProvider;
        std::string mVehicleName;
        uint64_t mFetchedFrameCount;
    };
}
