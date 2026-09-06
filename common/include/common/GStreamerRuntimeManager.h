#pragma once

#include <string>

namespace common
{
    // gst_init / gst_deinit tek noktadan. Birden fazla cagrılsa bile
    // GStreamer'ı yalnızca bir kez baslatır.
    class GStreamerRuntimeManager
    {
    public:
        static bool initialize(int* pArgc, char*** pArgv);
        static void shutdown();
        static bool isInitialized();
        static std::string getVersionString();

    private:
        static bool mIsInitialized;
    };
}
