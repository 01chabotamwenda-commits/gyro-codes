/**
 * telemetry.h
 * Periodic telemetry output over Serial.
 * Sends both a human-readable line and a compact JSON line every TELEMETRY_MS.
 *
 * Depends on: config.h, imu.h, rpm.h, motor.h, auto_throttle.h
 *
 * ── Human-readable ────────────────────────────────────────────────────────────
 *   RPM: 1234 | Tilt: 1.2/0.5/0.0 | Vib: 0.012 g RMS | THR: 1270 µs (45.2%) | Auto: ON
 *   When Auto=ON:  PID: e=1.2 o=23.0 i=12.3 (throttle diagnostics)
 *
 * ── JSON (for companion app / Electron bridge) ────────────────────────────────
 *   {"rpm":1234,"tiltX":1.2,"tiltY":0.5,"tiltZ":0.0,
 *    "rotationZ":0.12,"avgTiltMag":1.3,"temp":25.0,
 *    "throttle":1270,"pwmPct":27.0,"vibration":0.012,"autoMode":true,
 *    "pidError":1.2,"pidOutput":23.0,"pidIntegral":12.3}
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

    Serial.printf("RPM: %lu | Tilt: %.1f/%.1f/%.1f | Vib: %.3f g RMS | THR: %d µs (%.1f%%) | Auto: %s",
                  rpmValue, errorX, errorY, errorZ,
                  vibrationRms, throttle, pwmPct, autoMode ? "ON" : "OFF");

    // Auto-throttle diagnostics
    if (autoMode)
    {
        Serial.printf(" | PID: e=%.1f o=%.1f i=%.1f",
                      atc.getLastError(), atc.getLastOutput(), atc.getLastIntegral());
    }
    Serial.println();

    // JSON
    Serial.printf("{\"rpm\":%lu,\"tiltX\":%.1f,\"tiltY\":%.1f,\"tiltZ\":%.1f,"
                  "\"rotationZ\":%.2f,\"avgTiltMag\":%.2f,\"temp\":25.0,"
                  "\"throttle\":%d,\"pwmPct\":%.1f,\"vibration\":%.3f,\"autoMode\":%s",
                  rpmValue, errorX, errorY, errorZ,
                  gyroZ_dps, tiltMag, throttle, pwmPct, vibrationRms,
                  autoMode ? "true" : "false");

    // PID diagnostics in JSON
    if (autoMode)
    {
        Serial.printf(",\"pidError\":%.1f,\"pidOutput\":%.1f,\"pidIntegral\":%.1f",
                      atc.getLastError(), atc.getLastOutput(), atc.getLastIntegral());
    }
    Serial.println("}");
}
