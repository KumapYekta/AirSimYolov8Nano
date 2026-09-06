#include "FrameStatisticsManager.h"

namespace airsim_yolov8
{
    FrameStatisticsManager::FrameStatisticsManager(double pReportPeriodSeconds)
        : mWindowStartTime(std::chrono::steady_clock::now())
        , mReportPeriodSeconds(pReportPeriodSeconds)
        , mLastFps(0.0)
        , mTotalFrameCount(0)
        , mWindowFrameCount(0)
    {
    }

    void FrameStatisticsManager::onFrameReceived()
    {
        mTotalFrameCount = mTotalFrameCount + 1;
        mWindowFrameCount = mWindowFrameCount + 1;
    }

    bool FrameStatisticsManager::shouldReport()
    {
        const std::chrono::steady_clock::time_point tNow = std::chrono::steady_clock::now();
        const std::chrono::duration<double> tElapsed = tNow - mWindowStartTime;

        if (tElapsed.count() < mReportPeriodSeconds)
        {
            return false;
        }

        mLastFps = static_cast<double>(mWindowFrameCount) / tElapsed.count();
        mWindowFrameCount = 0;
        mWindowStartTime = tNow;

        return true;
    }

    double FrameStatisticsManager::getLastFps() const
    {
        return mLastFps;
    }

    uint64_t FrameStatisticsManager::getTotalFrameCount() const
    {
        return mTotalFrameCount;
    }
}
