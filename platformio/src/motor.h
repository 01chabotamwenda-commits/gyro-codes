/**
 * motor.h
 * ESC / servo control, non-blocking throttle ramp,
 * and motor start / stop / emergency-stop logic.
 *
 * Depends on: config.h, rpm.h (resetRPMState)
 */

#pragma once
#include "config.h"

// ─────────────────────────────────────────────────────────────────────────────
// ESC / motor state
// ─────────────────────────────────────────────────────────────────────────────
static Servo esc;
bool motorRunning = false;          // extern'd by rpm.h
static int      throttle   = THROTTLE_ARM;
static bool     autoMode   = false; // tilt-based auto-throttle — off by default

// Non-blocking ramp state
static bool     ramping    = false;
static int      rampTarget = THROTTLE_ARM;
static uint32_t lastRampMs = 0;

// ─────────────────────────────────────────────────────────────────────────────
// Throttle ramp  (call setThrottleTarget once, then rampStep every loop)
// ─────────────────────────────────────────────────────────────────────────────
static void setThrottleTarget(int target)
{
    target = constrain(target, THROTTLE_MIN, THROTTLE_MAX);
    if (target == rampTarget) return;  // no change needed
    rampTarget = target;
    ramping    = true;
    lastRampMs = millis();
}

static void rampStep()
{
    if (!ramping) return;
    if ((millis() - lastRampMs) < RAMP_STEP_MS) return;
    lastRampMs = millis();

    if      (throttle < rampTarget) throttle++;
    else if (throttle > rampTarget) throttle--;
    else { ramping = false; return; }

    esc.write(throttle);
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
    throttle     = THROTTLE_ARM;
    esc.write(THROTTLE_ARM);
    resetRPMState();
    Serial.println("STATUS:MOTOR=OFF");
}

static void emergencyStop()
{
    motorRunning = false;
    ramping      = false;
    throttle     = THROTTLE_ARM;
    esc.write(THROTTLE_ARM);
    resetRPMState();
    Serial.println("STATUS:MOTOR=ESTOP");
    Serial.println("!!! EMERGENCY STOP !!!");
}

// ─────────────────────────────────────────────────────────────────────────────
// ESC hardware initialisation  (call once in setup)
// ─────────────────────────────────────────────────────────────────────────────
static void initESC()
{
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);
    esc.setPeriodHertz(50);           // standard 50 Hz servo/ESC PWM
    esc.attach(PIN_ESC, 1000, 2000);  // 1000–2000 µs pulse range

    Serial.println("Arming ESC (3 s) ...");
    esc.write(THROTTLE_ARM);
    delay(3000);
    Serial.println("ESC armed.");
    Serial.println("STATUS:MOTOR=OFF");
}
