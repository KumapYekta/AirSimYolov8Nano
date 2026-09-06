#pragma once

#include "ReceiverConfig.h"

#include <gst/app/gstappsink.h>
#include <gst/gst.h>

#include <string>

namespace jetson_receiver
{
    // UDP -> donanım decode -> appsink pipeline'ının kurulumu ve yasam dongusu.
    class GStreamerReceiverPipelineManager
    {
    public:
        GStreamerReceiverPipelineManager();
        ~GStreamerReceiverPipelineManager();

        GStreamerReceiverPipelineManager(const GStreamerReceiverPipelineManager&) = delete;
        GStreamerReceiverPipelineManager& operator=(const GStreamerReceiverPipelineManager&) = delete;

        bool build(const ReceiverConfig& pConfig);
        bool start();
        void stop();

        bool pollBus();
        bool isRunning() const;

        GstAppSink* getAppSink() const;
        const ReceiverConfig& getConfig() const;

        std::string buildPipelineDescription(const ReceiverConfig& pConfig) const;

    private:
        void configureAppSink();
        void releasePipeline();

        GstElement* mPipeline;
        GstAppSink* mAppSink;
        GstBus* mBus;
        ReceiverConfig mConfig;
        bool mIsRunning;
    };
}
