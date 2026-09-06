#include "GStreamerReceiverPipelineManager.h"

#include <iostream>
#include <sstream>

namespace jetson_receiver
{
    namespace
    {
        const char* const kAppSinkName = "gpu_sink";
    }

    GStreamerReceiverPipelineManager::GStreamerReceiverPipelineManager()
        : mPipeline(nullptr)
        , mAppSink(nullptr)
        , mBus(nullptr)
        , mConfig()
        , mIsRunning(false)
    {
    }

    GStreamerReceiverPipelineManager::~GStreamerReceiverPipelineManager()
    {
        stop();
        releasePipeline();
    }

    std::string GStreamerReceiverPipelineManager::buildPipelineDescription(const ReceiverConfig& pConfig) const
    {
        if (!pConfig.mCustomPipeline.empty())
        {
            return pConfig.mCustomPipeline;
        }

        std::ostringstream tStream;

        tStream << "udpsrc port=" << pConfig.mPort
                << " buffer-size=" << pConfig.mSocketBufferSize;

        if (pConfig.mDecoderType == DecoderType::H264)
        {
            tStream << " caps=\"application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)H264,payload=(int)96\""
                    << " ! rtpjitterbuffer latency=" << pConfig.mJitterBufferLatencyMs << " drop-on-latency=true"
                    << " ! rtph264depay"
                    << " ! h264parse"
                    // disable-dpb + max-performance -> decoder gecikmesi minimum
                    << " ! nvv4l2decoder enable-max-performance=1 disable-dpb=true"
                    << " ! nvvidconv";
        }
        else
        {
            tStream << " caps=\"application/x-rtp,media=(string)video,clock-rate=(int)90000,encoding-name=(string)JPEG,payload=(int)26\""
                    << " ! rtpjitterbuffer latency=" << pConfig.mJitterBufferLatencyMs << " drop-on-latency=true"
                    << " ! rtpjpegdepay"
                    << " ! nvjpegdec"
                    << " ! nvvidconv";
        }

        tStream << " ! video/x-raw,format=" << pConfig.mOutputFormat
                << " ! appsink name=" << kAppSinkName
                << " emit-signals=false sync=false max-buffers=1 drop=true";

        return tStream.str();
    }

    bool GStreamerReceiverPipelineManager::build(const ReceiverConfig& pConfig)
    {
        releasePipeline();
        mConfig = pConfig;

        const std::string tDescription = buildPipelineDescription(mConfig);
        std::cout << "[ReceiverPipeline] " << tDescription << std::endl;

        GError* tError = nullptr;
        mPipeline = gst_parse_launch(tDescription.c_str(), &tError);

        if ((mPipeline == nullptr) || (tError != nullptr))
        {
            const std::string tMessage = (tError != nullptr) ? tError->message : "unknown error";
            std::cerr << "[ReceiverPipeline] parse_launch failed: " << tMessage << std::endl;

            if (tError != nullptr)
            {
                g_error_free(tError);
            }

            releasePipeline();
            return false;
        }

        GstElement* tAppSinkElement = gst_bin_get_by_name(GST_BIN(mPipeline), kAppSinkName);

        if (tAppSinkElement == nullptr)
        {
            std::cerr << "[ReceiverPipeline] appsink named '" << kAppSinkName << "' not found." << std::endl;
            releasePipeline();
            return false;
        }

        mAppSink = GST_APP_SINK(tAppSinkElement);
        mBus = gst_element_get_bus(mPipeline);

        configureAppSink();
        return true;
    }

    void GStreamerReceiverPipelineManager::configureAppSink()
    {
        gst_app_sink_set_max_buffers(mAppSink, 1);
        gst_app_sink_set_drop(mAppSink, TRUE);
        gst_app_sink_set_emit_signals(mAppSink, FALSE);

        g_object_set(G_OBJECT(mAppSink), "sync", FALSE, nullptr);
    }

    bool GStreamerReceiverPipelineManager::start()
    {
        if (mPipeline == nullptr)
        {
            return false;
        }

        const GstStateChangeReturn tReturn = gst_element_set_state(mPipeline, GST_STATE_PLAYING);

        if (tReturn == GST_STATE_CHANGE_FAILURE)
        {
            std::cerr << "[ReceiverPipeline] Failed to set PLAYING state." << std::endl;
            return false;
        }

        mIsRunning = true;
        std::cout << "[ReceiverPipeline] Listening on UDP port " << mConfig.mPort << std::endl;

        return true;
    }

    void GStreamerReceiverPipelineManager::stop()
    {
        if (mPipeline == nullptr)
        {
            return;
        }

        gst_element_set_state(mPipeline, GST_STATE_NULL);
        mIsRunning = false;
    }

    bool GStreamerReceiverPipelineManager::pollBus()
    {
        if (mBus == nullptr)
        {
            return true;
        }

        bool tIsHealthy = true;

        while (true)
        {
            GstMessage* tMessage = gst_bus_pop_filtered(mBus, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_WARNING | GST_MESSAGE_EOS));

            if (tMessage == nullptr)
            {
                break;
            }

            if (GST_MESSAGE_TYPE(tMessage) == GST_MESSAGE_ERROR)
            {
                GError* tError = nullptr;
                gchar* tDebug = nullptr;
                gst_message_parse_error(tMessage, &tError, &tDebug);

                std::cerr << "[ReceiverPipeline] ERROR: " << tError->message << std::endl;

                if (tDebug != nullptr)
                {
                    std::cerr << "[ReceiverPipeline] DEBUG: " << tDebug << std::endl;
                    g_free(tDebug);
                }

                g_error_free(tError);
                tIsHealthy = false;
            }
            else if (GST_MESSAGE_TYPE(tMessage) == GST_MESSAGE_WARNING)
            {
                GError* tError = nullptr;
                gchar* tDebug = nullptr;
                gst_message_parse_warning(tMessage, &tError, &tDebug);

                std::cerr << "[ReceiverPipeline] WARN: " << tError->message << std::endl;

                if (tDebug != nullptr)
                {
                    g_free(tDebug);
                }

                g_error_free(tError);
            }
            else
            {
                std::cout << "[ReceiverPipeline] EOS received." << std::endl;
                tIsHealthy = false;
            }

            gst_message_unref(tMessage);
        }

        return tIsHealthy;
    }

    bool GStreamerReceiverPipelineManager::isRunning() const
    {
        return mIsRunning;
    }

    GstAppSink* GStreamerReceiverPipelineManager::getAppSink() const
    {
        return mAppSink;
    }

    const ReceiverConfig& GStreamerReceiverPipelineManager::getConfig() const
    {
        return mConfig;
    }

    void GStreamerReceiverPipelineManager::releasePipeline()
    {
        if (mBus != nullptr)
        {
            gst_object_unref(mBus);
            mBus = nullptr;
        }

        if (mAppSink != nullptr)
        {
            gst_object_unref(mAppSink);
            mAppSink = nullptr;
        }

        if (mPipeline != nullptr)
        {
            gst_object_unref(mPipeline);
            mPipeline = nullptr;
        }
    }
}
