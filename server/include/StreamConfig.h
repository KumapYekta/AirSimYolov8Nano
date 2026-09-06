#pragma once

#include <cstdint>
#include <string>

namespace airsim_yolov8
{
    enum class EncoderType
    {
        Vaapi,  // AMD 6700XT -> Mesa VA-API donanım encode BUT ONLY FOR LINUX WITH DIRECTED USE, OTHERWISE USE WITH VAON12 FOR DIRECTX12
        X264,   // CPU fallback
        Jpeg,   // dusuk latency, yuksek bant genisligi
        Amf     // NOTE: use this for windows os and amd gpu's.
    };

    struct StreamConfig
    {
        std::string mHost = "127.0.0.1";
        uint16_t mPort = 5600;

        int mWidth = 0;
        int mHeight = 0;
        int mChannelCount = 3;
        int mTargetFps = 30;

        uint32_t mBitrateKbps = 12000;
        int mKeyframeInterval = 30;
        int mMtu = 1400;

        EncoderType mEncoderType = EncoderType::Amf;

        // true  -> vektor GstBuffer'a tasınır, hic kopyalama yok
        // false -> gst_buffer_map + std::memcpy
        bool mUseZeroCopyBuffers = true;

        // Doluysa yukarıdaki ayarlar yok sayılıp bu pipeline kullanılır.
        // appsrc'ın adı "airsim_src" olmak zorunda.
        std::string mCustomPipeline;
    };
}
