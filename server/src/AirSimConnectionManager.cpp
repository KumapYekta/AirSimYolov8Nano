#include "AirSimConnectionManager.h"

#include <iostream>

namespace airsim_yolov8
{
    AirSimConnectionManager::AirSimConnectionManager(const std::string& pHost, uint16_t pPort)
        : mClient(nullptr)
        , mHost(pHost)
        , mPort(pPort)
        , mIsConnected(false)
    {
    }

    AirSimConnectionManager::~AirSimConnectionManager()
    {
        disconnect();
    }

    bool AirSimConnectionManager::connect()
    {
        if (mIsConnected)
        {
            return true;
        }

        try
        {
            mClient = std::make_unique<msr::airlib::MultirotorRpcLibClient>(mHost, mPort);
            mClient->confirmConnection();
            mIsConnected = true;

            std::cout << "[ConnectionManager] Connected to AirSim at " << mHost << ":" << mPort << std::endl;
        }
        catch (const rpc::rpc_error& tError)
        {
            const std::string tMessage = tError.what();
            std::cerr << "[ConnectionManager] RPC error while connecting: " << tMessage << std::endl;
            mClient.reset();
            mIsConnected = false;
        }
        catch (const std::exception& tError)
        {
            std::cerr << "[ConnectionManager] Connection failed: " << tError.what() << std::endl;
            mClient.reset();
            mIsConnected = false;
        }

        return mIsConnected;
    }

    void AirSimConnectionManager::disconnect()
    {
        if (mClient != nullptr)
        {
            mClient.reset();
        }

        mIsConnected = false;
    }

    bool AirSimConnectionManager::isConnected() const
    {
        return mIsConnected && (mClient != nullptr);
    }

    std::vector<std::string> AirSimConnectionManager::getVehicleNames() const
    {
        std::vector<std::string> tVehicleNames;

        if (!isConnected())
        {
            return tVehicleNames;
        }

        try
        {
            tVehicleNames = mClient->listVehicles();
        }
        catch (const std::exception& tError)
        {
            std::cerr << "[ConnectionManager] listVehicles failed: " << tError.what() << std::endl;
        }

        return tVehicleNames;
    }

    void AirSimConnectionManager::printVehicleNames() const
    {
        const std::vector<std::string> tVehicleNames = getVehicleNames();

        std::cout << "[ConnectionManager] Active vehicle count: " << tVehicleNames.size() << std::endl;

        for (const std::string& tName : tVehicleNames)
        {
            std::cout << "[ConnectionManager] Vehicle: '" << tName << "'" << std::endl;
        }
    }

    msr::airlib::MultirotorRpcLibClient& AirSimConnectionManager::getClient()
    {
        return *mClient;
    }
}
