#pragma once

#include "AirSimIncludes.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace airsim_yolov8
{
    // Sadece RPC baglantisinin yasam dongusunden sorumlu.
    // Goruntu cekme, istek olusturma gibi isler bu sinifin disinda.
    class AirSimConnectionManager
    {
    public:
        AirSimConnectionManager(const std::string& pHost, uint16_t pPort);
        ~AirSimConnectionManager();

        AirSimConnectionManager(const AirSimConnectionManager&) = delete;
        AirSimConnectionManager& operator=(const AirSimConnectionManager&) = delete;

        bool connect();
        void disconnect();
        bool isConnected() const;

        std::vector<std::string> getVehicleNames() const;
        void printVehicleNames() const;

        msr::airlib::MultirotorRpcLibClient& getClient();

    private:
        std::unique_ptr<msr::airlib::MultirotorRpcLibClient> mClient;
        std::string mHost;
        uint16_t mPort;
        bool mIsConnected;
    };
}
