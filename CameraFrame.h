#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace airsim_yolov8
{
    // AirSim'den gelen tek bir goruntunun CPU tarafındaki karsılıgı.
    // Ileride bu buffer dogrudan Jetson'ın GPU belleğine kopyalanacak,
    // o yuzden veri duz bir uint8 dizisi olarak tutuluyor.
    struct CameraFrame
    {
        std::vector<uint8_t> mData;
        std::string mCameraName;
        int mWidth = 0;
        int mHeight = 0;
        int mChannelCount = 0;
        uint64_t mTimestampNs = 0;
        bool mIsCompressed = false;

        bool isValid() const
        {
            return (mWidth > 0) && (mHeight > 0) && (!mData.empty());
        }

        size_t getByteCount() const
        {
            return mData.size();
        }
    };
}
