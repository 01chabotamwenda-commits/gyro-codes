/**
 * motor.h
 * ESC / ledc control, non-blocking throttle ramp,
 * and motor start / stop / emergency-stop logic.
 *
 * ESC init is split into two calls — BOTH must be in setup() in this order:
 *   1. escPwmInit()  — call FIRST, before any delay or other init.
 *                      Sets up ledc and immediately sends 1000 µs so the
 *                      ESC sees the arming signal within ms of power-on.
 *   2. armESC()      — call AFTER other peripheral init.
 *                      Sends 1000 µs again and holds for 3 s (ESC beeps).
 *
 * Uses ledcSetup + ledcAttachPin + ledcWrite (ESP32 Arduino core 2.x API).
 * No third-party library required.
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
// Low-level PWM output
// Converts µs pulse width → 16-bit duty cycle and writes via ledc.
// Same formula as the verified tester: duty = us * 65535 / 20000
// ─────────────────────────────────────────────────────────────────────────────
static void setThrottle(int us)
{
    us = constrain(us, THROTTLE_ARM, THROTTLE_MAX);
    uint32_t duty = (uint32_t)us * 65535UL / 20000UL;
    ledcWrite(PWM_CHANNEL, duty);
    throttle = us;
}

// ─────────────────────────────────────────────────────────────────────────────
// PHASE 1 — call at the very top of setup(), before anything else.
// Configures ledc and immediately drives 1000 µs so the ESC sees the
// arming signal within a few milliseconds of power-on.
// ─────────────────────────────────────────────────────────────────────────────
static void escPwmInit()
{
    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(PIN_ESC, PWM_CHANNEL);
    setThrottle(THROTTLE_ARM);  // 1000 µs — ESC arming signal
    delay(100);
}

// ─────────────────────────────────────────────────────────────────────────────
// PHASE 2 — call after other peripheral init (IMU, GPIO, etc.).
// Sends 1000 µs again and holds for 3 s while ESC plays its beep sequence.
// ─────────────────────────────────────────────────────────────────────────────
static void armESC()
{
    Serial.println("[ARM] Sending min throttle (1000 us)...");
    setThrottle(THROTTLE_ARM);
    Serial.println("[ARM] Hold for 3 seconds — listen for ESC beeps.");
    delay(3000);
    Serial.println("[ARM] ESC armed. Ready.");
    Serial.println("STATUS:MOTOR=OFF");
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
