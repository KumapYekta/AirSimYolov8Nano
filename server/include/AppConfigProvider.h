#pragma once

#include "StreamConfig.h"

#include <cstdint>
#include <string>

namespace airsim_yolov8
{
    // Tum calısma zamanı ayarları environment variable'lardan okunur.
    class AppConfigProvider
    {
    public:
        AppConfigProvider();

        void loadFromEnvironment();
        void printSummary() const;

        const std::string& getAirSimHost() const;
        uint16_t getAirSimPort() const;
        const std::string& getVehicleName() const;
        const std::string& getCameraName() const;

        // Frame boyutu ilk goruntu geldikten sonra doldurulur.
        StreamConfig getStreamConfig() const;

    private:
        static std::string readEnvironment(const char* pName, const std::string& pDefaultValue);
        static EncoderType parseEncoderType(const std::string& pValue);

        std::string mAirSimHost;
        uint16_t mAirSimPort;
        std::string mVehicleName;
        std::string mCameraName;
        StreamConfig mStreamConfig;
    };
}
