#pragma once

#include "ReceiverConfig.h"

#include <string>

namespace jetson_receiver
{
    class ReceiverConfigProvider
    {
    public:
        ReceiverConfigProvider();

        void loadFromEnvironment();
        void printSummary() const;

        const ReceiverConfig& getConfig() const;

    private:
        static std::string readEnvironment(const char* pName, const std::string& pDefaultValue);
        static DecoderType parseDecoderType(const std::string& pValue);

        ReceiverConfig mConfig;
    };
}
