#include "common/GStreamerRuntimeManager.h"

#include <gst/gst.h>

#include <iostream>

namespace common
{
    bool GStreamerRuntimeManager::mIsInitialized = false;

    bool GStreamerRuntimeManager::initialize(int* pArgc, char*** pArgv)
    {
        if (mIsInitialized)
        {
            return true;
        }

        GError* tError = nullptr;

        if (gst_init_check(pArgc, pArgv, &tError) == FALSE)
        {
            const std::string tMessage = (tError != nullptr) ? tError->message : "unknown error";
            std::cerr << "[GStreamerRuntimeManager] gst_init failed: " << tMessage << std::endl;

            if (tError != nullptr)
            {
                g_error_free(tError);
            }

            return false;
        }

        mIsInitialized = true;
        std::cout << "[GStreamerRuntimeManager] " << getVersionString() << std::endl;

        return true;
    }

    void GStreamerRuntimeManager::shutdown()
    {
        if (!mIsInitialized)
        {
            return;
        }

        gst_deinit();
        mIsInitialized = false;
    }

    bool GStreamerRuntimeManager::isInitialized()
    {
        return mIsInitialized;
    }

    std::string GStreamerRuntimeManager::getVersionString()
    {
        gchar* tVersion = gst_version_string();
        const std::string tResult(tVersion);
        g_free(tVersion);

        return tResult;
    }
}
