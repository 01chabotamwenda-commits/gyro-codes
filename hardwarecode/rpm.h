/**
 * rpm.h
 * TCRT5000 IR sensor ISR, RPM averaging, stale-out detection,
 * and periodic RPM publication.
 *
 * Depends on: config.h
 */

#pragma once
#include "config.h"

// ─────────────────────────────────────────────────────────────────────────────
// Volatile ISR ↔ loop handoff variables
// ─────────────────────────────────────────────────────────────────────────────
static volatile uint32_t isrLastMicros  = 0;
static volatile uint32_t isrPeriodMs   = 0;   // period of last revolution [ms]
static volatile bool     isrPulseReady = false;

// ─────────────────────────────────────────────────────────────────────────────
// Safe loop-side copies
// ─────────────────────────────────────────────────────────────────────────────
static uint32_t rpmValue              = 0;
static float    rpmFreqHz             = 0.0f;
static uint32_t rpmPeriodMs           = 0;
static uint32_t lastPulseMicrosSnapshot = 0;

// ─────────────────────────────────────────────────────────────────────────────
// Rolling average buffer  (RPM_AVG_SAMPLES samples)
// ─────────────────────────────────────────────────────────────────────────────
static uint32_t rpmPeriodBuffer[RPM_AVG_SAMPLES] = {0};
static uint8_t  rpmPeriodWriteIdx = 0;
static uint8_t  rpmPeriodCount    = 0;
static float    filteredRpm       = 0.0f;
static float    filteredFreqHz    = 0.0f;
static uint32_t lastRpmReportMs   = 0;

// ─────────────────────────────────────────────────────────────────────────────
// ISR — runs in IRAM, work kept minimal
// ─────────────────────────────────────────────────────────────────────────────
void IRAM_ATTR irPulseISR()
{
    static uint32_t lastAcceptedUs = 0;
    uint32_t now = micros();

    if (lastAcceptedUs > 0)
    {
        uint32_t deltaUs = now - lastAcceptedUs;
        if (deltaUs < IR_DEBOUNCE_US) return;   // ignore bounce / spurious edges
    }
    lastAcceptedUs = now;

    if (isrLastMicros > 0)
    {
        uint32_t periodUs = now - isrLastMicros;
        if (periodUs >= IR_MIN_PERIOD_US && periodUs <= IR_MAX_PERIOD_US)
        {
            uint32_t periodMs = periodUs / 1000UL;
            if (periodMs > 0) isrPeriodMs = periodMs;
        }
    }
    isrLastMicros = now;
    isrPulseReady = true;

    // Toggle LED without reading it first (avoids any blocking call in ISR)
    gpio_set_level((gpio_num_t)PIN_LED,
                   !gpio_get_level((gpio_num_t)PIN_LED));
}

// ─────────────────────────────────────────────────────────────────────────────
// Reset all RPM state  (call on motor stop or start)
// ─────────────────────────────────────────────────────────────────────────────
static void resetRPMState()
{
    rpmValue     = 0;
    rpmFreqHz    = 0.0f;
    rpmPeriodMs  = 0;
    filteredRpm  = 0.0f;
    filteredFreqHz      = 0.0f;
    rpmPeriodWriteIdx   = 0;
    rpmPeriodCount      = 0;
    lastPulseMicrosSnapshot = 0;
    for (uint8_t i = 0; i < RPM_AVG_SAMPLES; i++) rpmPeriodBuffer[i] = 0;

    noInterrupts();
    isrPeriodMs   = 0;
    isrPulseReady = false;
    isrLastMicros = 0;
    interrupts();
}

// ─────────────────────────────────────────────────────────────────────────────
// Process one IR pulse (called from loop when isrPulseReady is set)
// ─────────────────────────────────────────────────────────────────────────────
// Forward declaration — motorRunning is defined in motor.h
extern bool motorRunning;

static void processRPMPulse()
{
    if (!motorRunning || !isrPulseReady) return;

    noInterrupts();
    isrPulseReady = false;
    uint32_t period = isrPeriodMs;
    uint32_t stamp  = isrLastMicros;
    interrupts();

    lastPulseMicrosSnapshot = stamp;

    if (period > 0)
    {
        rpmPeriodBuffer[rpmPeriodWriteIdx] = period;
        rpmPeriodWriteIdx = (rpmPeriodWriteIdx + 1) % RPM_AVG_SAMPLES;
        if (rpmPeriodCount < RPM_AVG_SAMPLES) rpmPeriodCount++;

        uint32_t sumPeriods = 0;
        uint8_t  count      = rpmPeriodCount;
        for (uint8_t i = 0; i < count; i++)
        {
            uint8_t idx = (rpmPeriodWriteIdx + RPM_AVG_SAMPLES - 1 - i) % RPM_AVG_SAMPLES;
            sumPeriods += rpmPeriodBuffer[idx];
        }

        rpmPeriodMs = sumPeriods / count;
        if (rpmPeriodMs > 0)
        {
            float candidateRpm  = 60000.0f / (float)rpmPeriodMs;
            float candidateFreq = 1000.0f  / (float)rpmPeriodMs;

            if (filteredRpm <= 0.0f)
            {
                filteredRpm    = candidateRpm;
                filteredFreqHz = candidateFreq;
            }
            else
            {
                // Exponential moving average — 75 % old / 25 % new
                filteredRpm    = filteredRpm    * 0.75f + candidateRpm  * 0.25f;
                filteredFreqHz = filteredFreqHz * 0.75f + candidateFreq * 0.25f;
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Zero RPM if no pulse received within RPM_TIMEOUT_MS
// ─────────────────────────────────────────────────────────────────────────────
static void checkRPMTimeout()
{
    if (!motorRunning || lastPulseMicrosSnapshot == 0) return;
    uint32_t elapsedMs = (micros() - lastPulseMicrosSnapshot) / 1000UL;
    if (elapsedMs > RPM_TIMEOUT_MS)
    {
        rpmValue    = 0;
        rpmFreqHz   = 0.0f;
        rpmPeriodMs = 0;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Publish RPM at fixed cadence (RPM_REPORT_INTERVAL_MS)
// ─────────────────────────────────────────────────────────────────────────────
static void publishRPMValue()
{
    if (!motorRunning)
    {
        if (rpmValue != 0 || rpmFreqHz != 0.0f || rpmPeriodMs != 0 || filteredRpm != 0.0f)
            resetRPMState();
        return;
    }
    if ((millis() - lastRpmReportMs) < RPM_REPORT_INTERVAL_MS) return;
    lastRpmReportMs = millis();

    if (rpmPeriodMs > 0 && filteredRpm > 0.0f)
    {
        rpmValue  = (uint32_t)filteredRpm;
        rpmFreqHz = filteredFreqHz;
    }
    else
    {
        rpmValue  = 0;
        rpmFreqHz = 0.0f;
    }
}
