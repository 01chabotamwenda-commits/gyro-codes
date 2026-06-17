/**
 * motor.h
 * ESC / ledc control, non-blocking throttle ramp,
 * and motor start / stop / emergency-stop logic.
 *
 * Uses the same ledcAttach / ledcWrite approach as the verified tester sketch.
 * No third-party library required — only the built-in ESP32 ledc peripheral.
 *
 * Depends on: config.h, rpm.h (resetRPMState)
 */

#pragma once
#include "config.h"

// ─────────────────────────────────────────────────────────────────────────────
// Motor state
// ─────────────────────────────────────────────────────────────────────────────
bool motorRunning = false;          // extern'd by rpm.h
static int      throttle   = THROTTLE_ARM;  // current pulse width in µs
static bool     autoMode   = false;         // tilt-based auto-throttle — off by default

// Non-blocking ramp state
static bool     ramping    = false;
static int      rampTarget = THROTTLE_ARM;
static uint32_t lastRampMs = 0;

// ─────────────────────────────────────────────────────────────────────────────
// Low-level PWM output  (mirrors the working tester exactly)
// Converts µs pulse width → 16-bit duty cycle and writes via ledc.
// ─────────────────────────────────────────────────────────────────────────────
static void setThrottle(int us)
{
    us = constrain(us, THROTTLE_ARM, THROTTLE_MAX);
    uint32_t duty = (uint32_t)us * 65535UL / PWM_PERIOD_US;
    ledcWrite(PIN_ESC, duty);
    throttle = us;
}

// ─────────────────────────────────────────────────────────────────────────────
// Throttle ramp  (call setThrottleTarget once, then rampStep every loop)
// ─────────────────────────────────────────────────────────────────────────────
static void setThrottleTarget(int targetUs)
{
    targetUs = constrain(targetUs, THROTTLE_MIN, THROTTLE_MAX);
    if (targetUs == rampTarget) return;
    rampTarget = targetUs;
    ramping    = true;
    lastRampMs = millis();
}

static void rampStep()
{
    if (!ramping) return;
    if ((millis() - lastRampMs) < RAMP_STEP_MS) return;
    lastRampMs = millis();

    if (throttle < rampTarget)
        setThrottle(min(throttle + RAMP_STEP_US, rampTarget));
    else if (throttle > rampTarget)
        setThrottle(max(throttle - RAMP_STEP_US, rampTarget));
    else
        ramping = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Motor start / stop / emergency stop
// ─────────────────────────────────────────────────────────────────────────────
static void startMotor()
{
    if (motorRunning) { Serial.println("STATUS:MOTOR=ON"); return; }
    motorRunning = true;
    resetRPMState();
    Serial.println("--- RAMPING MOTOR UP ---");
    setThrottleTarget(THROTTLE_MIN);
    Serial.println("STATUS:MOTOR=ON");
}

static void stopMotor()
{
    motorRunning = false;
    ramping      = false;
    setThrottle(THROTTLE_ARM);
    rampTarget   = THROTTLE_ARM;
    resetRPMState();
    Serial.println("STATUS:MOTOR=OFF");
}

static void emergencyStop()
{
    motorRunning = false;
    ramping      = false;
    setThrottle(THROTTLE_ARM);
    rampTarget   = THROTTLE_ARM;
    resetRPMState();
    Serial.println("STATUS:MOTOR=ESTOP");
    Serial.println("!!! EMERGENCY STOP !!!");
}

// ─────────────────────────────────────────────────────────────────────────────
// ESC hardware initialisation  (call once in setup)
// Mirrors the tester sketch: ledcAttach → send 1000 µs → wait 3 s → armed.
// ─────────────────────────────────────────────────────────────────────────────
static void initESC()
{
    ledcAttach(PIN_ESC, PWM_FREQ, PWM_RESOLUTION);

    Serial.println("[ARM] Sending min throttle (1000 µs)...");
    setThrottle(THROTTLE_ARM);
    Serial.println("[ARM] Hold for 3 seconds — listen for ESC beeps.");
    delay(3000);
    Serial.println("[ARM] ESC armed. Ready.");
    Serial.println("STATUS:MOTOR=OFF");
}
