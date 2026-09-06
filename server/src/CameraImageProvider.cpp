#include "CameraImageProvider.h"

#include <iostream>

namespace airsim_yolov8
{
    CameraImageProvider::CameraImageProvider(AirSimConnectionManager& pConnectionManager, const std::string& pVehicleName)
        : mConnectionManager(pConnectionManager)
        , mRequestProvider()
        , mVehicleName(pVehicleName)
        , mFetchedFrameCount(0)
    {
    }

    ImageRequestProvider& CameraImageProvider::getRequestProvider()
    {
        return mRequestProvider;
    }

    bool CameraImageProvider::fetchFrames(std::vector<CameraFrame>& pOutFrames)
    {
        pOutFrames.clear();

        if (!mConnectionManager.isConnected())
        {
            std::cerr << "[CameraImageProvider] Not connected." << std::endl;
            return false;
        }

        if (mRequestProvider.isEmpty())
        {
            std::cerr << "[CameraImageProvider] Request list is empty." << std::endl;
            return false;
        }

        std::vector<msr::airlib::ImageCaptureBase::ImageResponse> tResponses;

        try
        {
            tResponses = mConnectionManager.getClient().simGetImages(mRequestProvider.getRequests(), mVehicleName);
        }
        catch (const rpc::rpc_error& tError)
        {
            const std::string tMessage = tError.what();
            std::cerr << "[CameraImageProvider] simGetImages RPC error: " << tMessage << std::endl;
            return false;
        }
        catch (const std::exception& tError)
        {
            std::cerr << "[CameraImageProvider] simGetImages failed: " << tError.what() << std::endl;
            return false;
        }

        if (tResponses.empty())
        {
            return false;
        }

        // TODO: her seferinde poutframes'i clearlayip reservelemek mantikli mi? response'larin size'i zaten degismiyor sanki.
        pOutFrames.reserve(tResponses.size());

        for (const msr::airlib::ImageCaptureBase::ImageResponse& tResponse : tResponses)
        {
            if (tResponse.image_data_uint8.empty())
            {
                continue;
            }

            CameraFrame tFrame;
            tFrame.mData = tResponse.image_data_uint8;
            tFrame.mCameraName = tResponse.camera_name;
            tFrame.mWidth = tResponse.width;
            tFrame.mHeight = tResponse.height;
            tFrame.mTimestampNs = static_cast<uint64_t>(tResponse.time_stamp);
            tFrame.mIsCompressed = tResponse.compress;

            if (!tFrame.mIsCompressed && (tFrame.mWidth > 0) && (tFrame.mHeight > 0))
            {
                const size_t tPixelCount = static_cast<size_t>(tFrame.mWidth) * static_cast<size_t>(tFrame.mHeight);
                tFrame.mChannelCount = static_cast<int>(tFrame.mData.size() / tPixelCount);
            }

            pOutFrames.push_back(std::move(tFrame));
        }

        if (pOutFrames.empty())
        {
            return false;
        }

        mFetchedFrameCount = mFetchedFrameCount + 1;
        return true;
    }

    uint64_t CameraImageProvider::getFetchedFrameCount() const
    {
        return mFetchedFrameCount;
    }
}
