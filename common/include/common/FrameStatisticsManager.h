#pragma once

#include <chrono>
#include <cstdint>

namespace common
{
    // FPS ve toplam frame sayısı gibi olcumleri tutar.
    class FrameStatisticsManager
    {
    public:
        explicit FrameStatisticsManager(double pReportPeriodSeconds = 1.0);

        void onFrameReceived();

        // Rapor periyodu dolduysa true doner ve sayacları sıfırlar.
        bool shouldReport();

        double getLastFps() const;
        uint64_t getTotalFrameCount() const;

    private:
        std::chrono::steady_clock::time_point mWindowStartTime;
        double mReportPeriodSeconds;
        double mLastFps;
        uint64_t mTotalFrameCount;
        uint64_t mWindowFrameCount;
    };
}
