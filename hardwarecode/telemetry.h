/**
 * telemetry.h
 * Periodic telemetry output over Serial.
 * Sends both a human-readable line and a compact JSON line every TELEMETRY_MS.
 *
 * Depends on: config.h, imu.h, rpm.h, motor.h
 *
 * ── Human-readable ────────────────────────────────────────────────────────────
 *   RPM: 1234 | Tilt: 1.2/0.5/0.0 | Vib: 0.012 g RMS | PWM: 45.23 | Auto: ON
 *
 * ── JSON (for companion app / Electron bridge) ────────────────────────────────
 *   {"rpm":1234,"tiltX":1.2,"tiltY":0.5,"tiltZ":0.0,
 *    "rotationZ":0.12,"avgTiltMag":1.3,"temp":25.0,
 *    "pwm":45.23,"vibration":0.012,"autoMode":true}
 */

#pragma once
#include "config.h"

static uint32_t lastTelemetryMs = 0;

static void sendTelemetry()
{
    if ((millis() - lastTelemetryMs) < TELEMETRY_MS) return;
    lastTelemetryMs = millis();

    float tiltMag = sqrtf(errorX * errorX + errorY * errorY);

    // Human-readable — shows raw µs (THR) and % (PWM) so you can see exactly what
    // the ESC is receiving.  THR = 1000–2000 µs is the real signal.
    float pwmPct = 0.0f;
    if (throttle > THROTTLE_ARM)
        pwmPct = (float)(throttle - THROTTLE_ARM) /
                 (float)(THROTTLE_MAX - THROTTLE_ARM) * 100.0f;

    Serial.printf("RPM: %lu | Tilt: %.1f/%.1f/%.1f | Vib: %.3f g RMS | THR: %d µs (%.1f%%) | Auto: %s\n",
                  rpmValue, errorX, errorY, errorZ,
                  vibrationRms, throttle, pwmPct, autoMode ? "ON" : "OFF");

    // JSON
    Serial.printf("{\"rpm\":%lu,\"tiltX\":%.1f,\"tiltY\":%.1f,\"tiltZ\":%.1f,"
                  "\"rotationZ\":%.2f,\"avgTiltMag\":%.2f,\"temp\":25.0,"
                  "\"throttle\":%d,\"pwmPct\":%.1f,\"vibration\":%.3f,\"autoMode\":%s}\n",
                  rpmValue, errorX, errorY, errorZ,
                  gyroZ_dps, tiltMag, throttle, pwmPct, vibrationRms,
                  autoMode ? "true" : "false");
}
