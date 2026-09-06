#pragma once

#include <cstdint>
#include <string>

namespace jetson_receiver
{
    enum class DecoderType
    {
        H264,  // nvv4l2decoder (donanım)
        Jpeg   // nvjpegdec
    };

    struct ReceiverConfig
    {
        uint16_t mPort = 5600;

        // rtpjitterbuffer gecikmesi (ms). Dusuk = dusuk latency, yuksek = daha az jitter.
        int mJitterBufferLatencyMs = 10;

        // udpsrc kernel soket buffer'ı. Kucuk kalırsa yuksek bitrate'te paket duser.
        int mSocketBufferSize = 4194304;

        DecoderType mDecoderType = DecoderType::H264;

        // appsink'ten cıkacak format. nvvidconv RGBA/BGRx uretebiliyor.
        std::string mOutputFormat = "RGBA";

        // appsink'ten sample beklerken timeout (ms). 0 = bloklamadan dene.
        int mPullTimeoutMs = 200;

        // Doluysa yukarıdaki ayarlar yok sayılır. appsink adı "gpu_sink" olmalı.
        std::string mCustomPipeline;

        // GstBuffer'ın host bellegini cudaHostRegister ile pinned yapar.
        // GStreamer buffer pool adresleri tekrar kullandıgı icin cache tutuyoruz.
        bool mUseHostRegister = true;
    };
}
