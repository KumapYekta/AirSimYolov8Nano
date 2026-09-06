#pragma once

#include "AirSimIncludes.h"

#include <string>
#include <vector>

namespace airsim_yolov8
{
    // simGetImages'e verilecek istek listesini uretir.
    // Kamera adı / tip / sıkıstırma gibi kararlar burada toplanıyor.
    class ImageRequestProvider
    {
    public:
        using ImageRequest = msr::airlib::ImageCaptureBase::ImageRequest;
        using ImageType = msr::airlib::ImageCaptureBase::ImageType;

        ImageRequestProvider() = default;

        // pCompressed = false ise ham BGR byte dizisi gelir (GPU'ya dogrudan
        // gonderilecegi icin varsayılan bu). true ise PNG olarak gelir.
        void addSceneRequest(const std::string& pCameraName, bool pCompressed = false);

        void addRequest(const std::string& pCameraName, ImageType pImageType, bool pPixelsAsFloat, bool pCompressed);

        void clear();
        bool isEmpty() const;
        size_t getRequestCount() const;

        // ImageRequest vector'unu return eder.
        const std::vector<ImageRequest>& getRequests() const;

    private:
        std::vector<ImageRequest> mRequests;
    };
}
