#include "GStreamerSenderPipelineManager.h"

#include <gst/video/video.h>

#include <iostream>
#include <sstream>

namespace airsim_yolov8
{
    namespace
    {
        const char* const kAppSrcName = "airsim_src";

        std::string getRawFormatName(int pChannelCount)
        {
            if (pChannelCount == 4)
            {
                return "BGRA";
            }

            return "BGR";
        }
    }

    GStreamerSenderPipelineManager::GStreamerSenderPipelineManager()
        : mPipeline(nullptr)
        , mAppSrc(nullptr)
        , mBus(nullptr)
        , mConfig()
        , mIsRunning(false)
    {
    }

    GStreamerSenderPipelineManager::~GStreamerSenderPipelineManager()
    {
        stop();
        releasePipeline();
    }

    std::string GStreamerSenderPipelineManager::buildPipelineDescription(const StreamConfig& pConfig) const
    {
        if (!pConfig.mCustomPipeline.empty())
        {
            return pConfig.mCustomPipeline;
        }

        std::ostringstream tStream;

        // << " ! videoconvert n-threads=4"
        // << " ! video/x-raw,format=I420"
        // << " ! x264enc tune=zerolatency speed-preset=ultrafast bitrate=12000 key-int-max=30"
        // << " ! video/x-h264,profile=constrained-baseline "
        // << " ! h264parse";

        // appsrc: canlı kaynak, blocking push, kucuk kuyruk -> latency birikmesin.
        tStream << "appsrc name=" << kAppSrcName
                << " is-live=true do-timestamp=false format=time"
                << " block=true max-bytes=0 max-buffers=4 leaky-type=downstream"    // NOTE: MAX 4 FRAMES. DROP EARLIEST FRAME.
                << " ! queue max-size-buffers=2 leaky=downstream"   // NOTE: queue yapma sebebimiz pushframe, videoconvert, gpuencode hepsinin multi thread yapılmasıdır.
                << " ! videoconvert n-threads=4";   // NOTE: ONLY USING 4 THREADS FOR NOW. (make sure to experiment).

        if (pConfig.mEncoderType == EncoderType::Vaapi)
        {
            tStream << " ! video/x-raw,format=NV12"
                    << " ! vaapih264enc rate-control=cbr bitrate=" << pConfig.mBitrateKbps
                    << " keyframe-period=" << pConfig.mKeyframeInterval
                    << " tune=low-power quality-level=7"
                    << " ! video/x-h264,profile=constrained-baseline"
                    << " ! h264parse config-interval=-1"
                    << " ! rtph264pay pt=96 config-interval=1 aggregate-mode=zero-latency mtu=" << pConfig.mMtu;
        }
        else if (pConfig.mEncoderType == EncoderType::X264)
        {
            tStream << " ! video/x-raw,format=I420"
                    << " ! x264enc tune=zerolatency speed-preset=ultrafast bframes=0 sliced-threads=true"
                    << " bitrate=" << pConfig.mBitrateKbps
                    << " key-int-max=" << pConfig.mKeyframeInterval
                    << " ! video/x-h264,profile=constrained-baseline"
                    << " ! h264parse config-interval=-1"
                    << " ! rtph264pay pt=96 config-interval=1 aggregate-mode=zero-latency mtu=" << pConfig.mMtu;
        }
        else if (pConfig.mEncoderType == EncoderType::Amf)
        {
            tStream << " ! video/x-raw,format=NV12"
                    << " ! amfh264enc usage=ultra-low-latency rate-control=cbr bitrate=" << pConfig.mBitrateKbps
                    << " gop-size=" << pConfig.mKeyframeInterval    // NOTE: LOWER GOP FOR MORE AGRESSIVE SCREEN REFRESH
                    << " b-frames=0"
                    // << " intra-refresh=true"    // NOTE: BAZI AMD GPU DRIVERLARDA ACIK OLMAYABILIR DESTEKLENMEYEBILIR.
                    << " ! video/x-h264,profile=baseline"
                    << " ! h264parse config-interval=-1"
                    << " ! rtph264pay pt=96 config-interval=1 aggregate-mode=zero-latency mtu=" << pConfig.mMtu;    // NOTE: UDP'YE RTP GIYDIRIYORUM ve SPS/PPS NAL config=1 ile yapilir.
        }
        else
        {
            tStream << " ! video/x-raw,format=I420"
                    << " ! jpegenc quality=85"
                    << " ! rtpjpegpay pt=26 mtu=" << pConfig.mMtu;
        }

        tStream << " ! udpsink host=" << pConfig.mHost
                << " port=" << pConfig.mPort
                << " sync=false async=false buffer-size=2097152";

        return tStream.str();
    }

    bool GStreamerSenderPipelineManager::build(const StreamConfig& pConfig)
    {
        releasePipeline();
        mConfig = pConfig;

        if ((mConfig.mWidth <= 0) || (mConfig.mHeight <= 0))
        {
            std::cerr << "[SenderPipeline] Invalid frame size in config." << std::endl;
            return false;
        }

        const std::string tDescription = buildPipelineDescription(mConfig);
        std::cout << "[SenderPipeline] " << tDescription << std::endl;

        GError* tError = nullptr;
        mPipeline = gst_parse_launch(tDescription.c_str(), &tError);

        if ((mPipeline == nullptr) || (tError != nullptr))
        {
            const std::string tMessage = (tError != nullptr) ? tError->message : "unknown error";
            std::cerr << "[SenderPipeline] parse_launch failed: " << tMessage << std::endl;

            if (tError != nullptr)
            {
                g_error_free(tError);
            }

            releasePipeline();
            return false;
        }

        GstElement* tAppSrcElement = gst_bin_get_by_name(GST_BIN(mPipeline), kAppSrcName);

        if (tAppSrcElement == nullptr)
        {
            std::cerr << "[SenderPipeline] appsrc named '" << kAppSrcName << "' not found." << std::endl;
            releasePipeline();
            return false;
        }

        mAppSrc = GST_APP_SRC(tAppSrcElement);
        mBus = gst_element_get_bus(mPipeline);

        configureAppSrc();
        return true;
    }

    void GStreamerSenderPipelineManager::configureAppSrc()
    {
        const std::string tFormat = getRawFormatName(mConfig.mChannelCount);

        GstCaps* tCaps = gst_caps_new_simple("video/x-raw",
                                             "format", G_TYPE_STRING, tFormat.c_str(),
                                             "width", G_TYPE_INT, mConfig.mWidth,
                                             "height", G_TYPE_INT, mConfig.mHeight,
                                             "framerate", GST_TYPE_FRACTION, mConfig.mTargetFps, 1,
                                             nullptr);

        gst_app_src_set_caps(mAppSrc, tCaps);
        gst_caps_unref(tCaps);

        gst_app_src_set_stream_type(mAppSrc, GST_APP_STREAM_TYPE_STREAM);
        gst_app_src_set_latency(mAppSrc, 0, 0);

        g_object_set(G_OBJECT(mAppSrc),
                     "is-live", TRUE,
                     "do-timestamp", FALSE,
                     "format", GST_FORMAT_TIME,
                     nullptr);
    }

    bool GStreamerSenderPipelineManager::start()
    {
        if (mPipeline == nullptr)
        {
            return false;
        }

        const GstStateChangeReturn tReturn = gst_element_set_state(mPipeline, GST_STATE_PLAYING);

        if (tReturn == GST_STATE_CHANGE_FAILURE)
        {
            std::cerr << "[SenderPipeline] Failed to set PLAYING state." << std::endl;
            return false;
        }

        mIsRunning = true;
        std::cout << "[SenderPipeline] Streaming to " << mConfig.mHost << ":" << mConfig.mPort << std::endl;

        return true;
    }

    void GStreamerSenderPipelineManager::stop()
    {
        if (mPipeline == nullptr)
        {
            return;
        }

        if (mAppSrc != nullptr)
        {
            gst_app_src_end_of_stream(mAppSrc);
        }

        gst_element_set_state(mPipeline, GST_STATE_NULL);
        mIsRunning = false;
    }

    bool GStreamerSenderPipelineManager::pollBus()
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

                std::cerr << "[SenderPipeline] ERROR: " << tError->message << std::endl;

                if (tDebug != nullptr)
                {
                    std::cerr << "[SenderPipeline] DEBUG: " << tDebug << std::endl;
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

                std::cerr << "[SenderPipeline] WARN: " << tError->message << std::endl;

                if (tDebug != nullptr)
                {
                    g_free(tDebug);
                }

                g_error_free(tError);
            }
            else
            {
                std::cout << "[SenderPipeline] EOS received." << std::endl;
                tIsHealthy = false;
            }

            gst_message_unref(tMessage);
        }

        return tIsHealthy;
    }

    bool GStreamerSenderPipelineManager::isRunning() const
    {
        return mIsRunning;
    }

    GstAppSrc* GStreamerSenderPipelineManager::getAppSrc() const
    {
        return mAppSrc;
    }

    const StreamConfig& GStreamerSenderPipelineManager::getConfig() const
    {
        return mConfig;
    }

    void GStreamerSenderPipelineManager::releasePipeline()
    {
        if (mBus != nullptr)
        {
            gst_object_unref(mBus);
            mBus = nullptr;
        }

        if (mAppSrc != nullptr)
        {
            gst_object_unref(mAppSrc);
            mAppSrc = nullptr;
        }

        if (mPipeline != nullptr)
        {
            gst_object_unref(mPipeline);
            mPipeline = nullptr;
        }
    }
}
