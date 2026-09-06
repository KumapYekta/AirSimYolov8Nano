#include "ImageRequestProvider.h"

namespace airsim_yolov8
{
    void ImageRequestProvider::addSceneRequest(const std::string& pCameraName, bool pCompressed)
    {
        addRequest(pCameraName, ImageType::Scene, false, pCompressed);
    }

    void ImageRequestProvider::addRequest(const std::string& pCameraName, ImageType pImageType, bool pPixelsAsFloat, bool pCompressed)
    {
        ImageRequest tRequest(pCameraName, pImageType, pPixelsAsFloat, pCompressed);
        mRequests.push_back(tRequest);
    }

    void ImageRequestProvider::clear()
    {
        mRequests.clear();
    }

    bool ImageRequestProvider::isEmpty() const
    {
        return mRequests.empty();
    }

    size_t ImageRequestProvider::getRequestCount() const
    {
        return mRequests.size();
    }

    const std::vector<ImageRequestProvider::ImageRequest>& ImageRequestProvider::getRequests() const
    {
        return mRequests;
    }
}
