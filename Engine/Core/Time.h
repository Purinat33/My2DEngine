#pragma once
#include <cstdint>

namespace my2d
{
    class Time
    {
    public:
        void Reset(uint64_t perfCounter)
        {
            m_lastCounter = perfCounter;
            m_deltaSeconds = 0.0;
            m_totalSeconds = 0.0;
            m_frameIndex = 0;
        }

        void Tick(uint64_t perfCounter, uint64_t perfFreq)
        {
            const uint64_t delta = perfCounter - m_lastCounter;
            m_lastCounter = perfCounter;

            const double seconds = (perfFreq > 0) ? (static_cast<double>(delta) / static_cast<double>(perfFreq)) : 0.0;

            m_deltaSeconds = seconds;
            m_totalSeconds += seconds;
            ++m_frameIndex;
        }

        double DeltaSeconds() const { return m_deltaSeconds; }
        double TotalSeconds() const { return m_totalSeconds; }
        uint64_t FrameIndex() const { return m_frameIndex; }

    private:
        uint64_t m_lastCounter = 0;
        double m_deltaSeconds = 0.0;
        double m_totalSeconds = 0.0;
        uint64_t m_frameIndex = 0;
    };
}