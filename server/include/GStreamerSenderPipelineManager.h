#pragma once

#include "StreamConfig.h"

#include <gst/app/gstappsrc.h>
#include <gst/gst.h>

#include <string>

namespace airsim_yolov8
{
    // Gonderici pipeline'ının kurulumu ve yasam dongusu.
    // Frame pushlamak bu sınıfın isi degil, sadece appsrc'ı disarıya verir.
    class GStreamerSenderPipelineManager
    {
    public:
        GStreamerSenderPipelineManager();
        ~GStreamerSenderPipelineManager();

        GStreamerSenderPipelineManager(const GStreamerSenderPipelineManager&) = delete;
        GStreamerSenderPipelineManager& operator=(const GStreamerSenderPipelineManager&) = delete;

        bool build(const StreamConfig& pConfig);
        bool start();
        void stop();

        // Bus'ta biriken hata/uyarıları bloklamadan isler.
        // Fatal hata varsa false doner.
        bool pollBus();

        bool isRunning() const;
        GstAppSrc* getAppSrc() const;
        const StreamConfig& getConfig() const;

        std::string buildPipelineDescription(const StreamConfig& pConfig) const;

    private:
        void configureAppSrc();
        void releasePipeline();

        GstElement* mPipeline;
        GstAppSrc* mAppSrc;
        GstBus* mBus;
        StreamConfig mConfig;
        bool mIsRunning;
    };
}
