#include "AppConfigProvider.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>

namespace airsim_yolov8
{
    AppConfigProvider::AppConfigProvider()
        : mAirSimHost("127.0.0.1")
        , mAirSimPort(41451)
        , mVehicleName("SimpleFlight")
        , mCameraName("0")
        , mStreamConfig()
    {
    }

    std::string AppConfigProvider::readEnvironment(const char* pName, const std::string& pDefaultValue)
    {
        // TODO: getenv deprecated.
        const char* tValue = std::getenv(pName);

        if (tValue == nullptr)
        {
            return pDefaultValue;
        }

        return std::string(tValue);
    }

    EncoderType AppConfigProvider::parseEncoderType(const std::string& pValue)
    {
        std::string tLowerValue = pValue;
        std::transform(tLowerValue.begin(), tLowerValue.end(), tLowerValue.begin(), ::tolower);

        if (tLowerValue == "x264")
        {
            return EncoderType::X264;
        }
        else if (tLowerValue == "jpeg")
        {
            return EncoderType::Jpeg;
        }
        else if (tLowerValue == "vaapi")
        {
            return EncoderType::Vaapi;
        }
        else if (tLowerValue == "amf")
        {
            return EncoderType::Amf;
        }

        return EncoderType::Jpeg;
    }

    void AppConfigProvider::loadFromEnvironment()
    {
        mAirSimHost = readEnvironment("AIRSIM_HOST", mAirSimHost);
        mAirSimPort = static_cast<uint16_t>(std::stoi(readEnvironment("AIRSIM_PORT", "41451")));
        mVehicleName = readEnvironment("AIRSIM_VEHICLE", mVehicleName);
        mCameraName = readEnvironment("AIRSIM_CAMERA", mCameraName);

        mStreamConfig.mHost = readEnvironment("STREAM_TARGET_HOST", "192.168.1.50");
        mStreamConfig.mPort = static_cast<uint16_t>(std::stoi(readEnvironment("STREAM_TARGET_PORT", "5600")));
        mStreamConfig.mTargetFps = std::stoi(readEnvironment("STREAM_FPS", "30"));
        mStreamConfig.mBitrateKbps = static_cast<uint32_t>(std::stoul(readEnvironment("STREAM_BITRATE_KBPS", "12000")));
        mStreamConfig.mKeyframeInterval = std::stoi(readEnvironment("STREAM_KEYFRAME_INTERVAL", "30"));
        mStreamConfig.mMtu = std::stoi(readEnvironment("STREAM_MTU", "1400"));
        mStreamConfig.mEncoderType = parseEncoderType(readEnvironment("STREAM_ENCODER", "amf"));
        mStreamConfig.mUseZeroCopyBuffers = (readEnvironment("STREAM_ZERO_COPY", "1") != "0");
        mStreamConfig.mCustomPipeline = readEnvironment("STREAM_PIPELINE", "");
    }

    void AppConfigProvider::printSummary() const
    {
        std::cout << "[AppConfigProvider] AirSim  : " << mAirSimHost << ":" << mAirSimPort
                  << " vehicle='" << mVehicleName << "' camera='" << mCameraName << "'" << std::endl;
        std::cout << "[AppConfigProvider] Stream  : " << mStreamConfig.mHost << ":" << mStreamConfig.mPort
                  << " fps=" << mStreamConfig.mTargetFps
                  << " bitrate=" << mStreamConfig.mBitrateKbps << "kbps"
                  << " zeroCopy=" << (mStreamConfig.mUseZeroCopyBuffers ? "on" : "off") << std::endl;
    }

    const std::string& AppConfigProvider::getAirSimHost() const
    {
        return mAirSimHost;
    }

    uint16_t AppConfigProvider::getAirSimPort() const
    {
        return mAirSimPort;
    }

    const std::string& AppConfigProvider::getVehicleName() const
    {
        return mVehicleName;
    }

    const std::string& AppConfigProvider::getCameraName() const
    {
        return mCameraName;
    }

    StreamConfig AppConfigProvider::getStreamConfig() const
    {
        return mStreamConfig;
    }
}
