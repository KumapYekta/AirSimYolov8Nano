#include "ReceiverConfigProvider.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>

namespace jetson_receiver
{
    ReceiverConfigProvider::ReceiverConfigProvider()
        : mConfig()
    {
    }

    std::string ReceiverConfigProvider::readEnvironment(const char* pName, const std::string& pDefaultValue)
    {
        const char* tValue = std::getenv(pName);

        if (tValue == nullptr)
        {
            return pDefaultValue;
        }

        return std::string(tValue);
    }

    DecoderType ReceiverConfigProvider::parseDecoderType(const std::string& pValue)
    {
        std::string tLowerValue = pValue;
        std::transform(tLowerValue.begin(), tLowerValue.end(), tLowerValue.begin(), ::tolower);

        if (tLowerValue == "jpeg")
        {
            return DecoderType::Jpeg;
        }

        return DecoderType::H264;
    }

    void ReceiverConfigProvider::loadFromEnvironment()
    {
        mConfig.mPort = static_cast<uint16_t>(std::stoi(readEnvironment("RECEIVER_PORT", "5600")));
        mConfig.mJitterBufferLatencyMs = std::stoi(readEnvironment("RECEIVER_JITTER_MS", "10"));
        mConfig.mSocketBufferSize = std::stoi(readEnvironment("RECEIVER_SOCKET_BUFFER", "4194304"));
        mConfig.mDecoderType = parseDecoderType(readEnvironment("RECEIVER_DECODER", "h264"));
        mConfig.mOutputFormat = readEnvironment("RECEIVER_FORMAT", "RGBA");
        mConfig.mPullTimeoutMs = std::stoi(readEnvironment("RECEIVER_PULL_TIMEOUT_MS", "200"));
        mConfig.mUseHostRegister = (readEnvironment("RECEIVER_HOST_REGISTER", "1") != "0");
        mConfig.mCustomPipeline = readEnvironment("RECEIVER_PIPELINE", "");
    }

    void ReceiverConfigProvider::printSummary() const
    {
        std::cout << "[ReceiverConfigProvider] port=" << mConfig.mPort
                  << " decoder=" << ((mConfig.mDecoderType == DecoderType::H264) ? "h264" : "jpeg")
                  << " format=" << mConfig.mOutputFormat
                  << " jitter=" << mConfig.mJitterBufferLatencyMs << "ms"
                  << " hostRegister=" << (mConfig.mUseHostRegister ? "on" : "off")
                  << std::endl;
    }

    const ReceiverConfig& ReceiverConfigProvider::getConfig() const
    {
        return mConfig;
    }
}
