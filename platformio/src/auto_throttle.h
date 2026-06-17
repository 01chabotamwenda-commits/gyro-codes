/**
 * auto_throttle.h
 * Tilt-based auto-throttle controller.
 *
 * Architecture:  Tilt -> PID -> Throttle
 *
 * Simplified single-loop PID (no cascade needed for this hardware):
 *   When the gyro tilts, the motor must spin faster to generate restoring torque.
 *   The controller maps tilt angle directly to throttle increase.
 *
 *   throttle = THROTTLE_MIN + Kp * tilt + Ki * integral(tilt)
 *
 *   tilt → error (positive = tilted) → PID output (positive = increase throttle)
 *   → throttle (constrained to [THROTTLE_MIN, THROTTLE_MAX])
 *
 *   No RPM inner loop is needed because the motor response is fast enough
 *   and the RPM sensor is too slow (250ms) for closed-loop RPM control.
 *
 *   Anti-windup: integral is clamped when throttle is saturated.
 *
 * Depends on: config.h
 */

#pragma once
#include "config.h"

// ---------------------------------------------------------------------------
// PID parameters
// ---------------------------------------------------------------------------
struct PidGains
{
    float Kp = 0.0f;
    float Ki = 0.0f;
    float Kd = 0.0f;
};

// ---------------------------------------------------------------------------
// Single-loop PID with anti-windup
// ---------------------------------------------------------------------------
class PidController
{
public:
    PidGains gains;
    float integralMax = 0.0f;
    float outputMax   = 0.0f;
    float Ts = 0.01f;

    void reset()
    {
        _integral = 0.0f;
        _prevError = 0.0f;
        _firstCall = true;
    }

    float update(float error)
    {
        float dt = Ts;
        if (_firstCall)
        {
            _prevError = error;
            _firstCall = false;
        }

        // Derivative with small low-pass filter for noise
        float rawDerivative = (error - _prevError) / dt;
        _derivative = _derivative * 0.7f + rawDerivative * 0.3f;
        _prevError = error;

        // Proportional + Derivative
        float P = gains.Kp * error;
        float D = gains.Kd * _derivative;
        float outTest = P + _integral + D;

        // Anti-windup: only integrate if not saturated in the same direction
        bool satHigh = (outTest >= outputMax) && (error > 0.0f);
        bool satLow  = (outTest <= -outputMax) && (error < 0.0f);
        if (!satHigh && !satLow)
        {
            _integral += error * dt * gains.Ki;
            _integral = constrain(_integral, -integralMax, integralMax);
        }

        float out = P + _integral + D;
        return constrain(out, -outputMax, outputMax);
    }

    float getIntegral() const { return _integral; }

private:
    float _integral = 0.0f;
    float _prevError = 0.0f;
    float _derivative = 0.0f;
    bool _firstCall = true;
};

// ---------------------------------------------------------------------------
// AutoThrottleController
// ---------------------------------------------------------------------------
class AutoThrottleController
{
public:
    // --- Tuning parameters (all in units that make sense for the hardware) ---

    // P gain: throttle [µs] per degree tilt.
    //   If the motor needs 1500 µs to stabilise at 10° tilt, and idle is 1270 µs,
    //   then Kp = (1500 - 1270) / 10 ≈ 23 µs/°.
    //   Start with 25 and adjust based on the response.
    float Kp = 25.0f;

    // I gain: throttle [µs] per degree-second.
    //   Slow integral to eliminate steady-state error.
    //   Start with 2.0 and increase if the gyro settles with a tilt.
    float Ki = 2.0f;

    // D gain: throttle [µs] per degree/second.
    //   Damping to prevent overshoot.
    //   Start with 0.5 and increase if the gyro oscillates.
    float Kd = 0.5f;

    // Integral and output limits [throttle units = µs]
    float integralMax = 200.0f;   // max integral windup
    float outputMax   = 730.0f;   // max throttle increase (2000 - 1270 = 730 µs)

    // Sampling period [seconds]
    float Ts = 0.01f;  // 10 ms → 100 Hz

    // Tilt dead-band [degrees]
    float tiltDeadDeg = 0.5f;

    // Controller state
    float lastTilt = 0.0f;
    float lastOutput = 0.0f;
    float lastError = 0.0f;
    float lastIntegral = 0.0f;

    AutoThrottleController() { reset(); }

    void reset()
    {
        pid.reset();
        lastTilt = 0.0f;
        lastOutput = 0.0f;
        lastError = 0.0f;
        lastIntegral = 0.0f;
    }

    /**
     * update()
     *
     * @param tiltMag    max(|errorX|, |errorY|) in degrees
     * @return           throttle target [µs]
     */
    int update(float tiltMag)
    {
        // Dead-band
        if (tiltMag < tiltDeadDeg)
            tiltMag = 0.0f;

        // Set gains
        pid.gains.Kp = Kp;
        pid.gains.Ki = Ki;
        pid.gains.Kd = Kd;
        pid.integralMax = integralMax;
        pid.outputMax = outputMax;
        pid.Ts = Ts;

        // Error = tilt (positive = need more throttle)
        float error = tiltMag;
        float output = pid.update(error);

        // Store diagnostics
        lastTilt = tiltMag;
        lastOutput = output;
        lastError = error;
        lastIntegral = pid.getIntegral();

        // Total throttle = idle + PID output
        float throttleOut = (float)THROTTLE_MIN + output;
        return (int)constrain(throttleOut,
                              (float)THROTTLE_MIN,
                              (float)THROTTLE_MAX);
    }

    // Getters for telemetry
    float getLastTilt()    const { return lastTilt; }
    float getLastOutput()  const { return lastOutput; }
    float getLastError()   const { return lastError; }
    float getLastIntegral() const { return lastIntegral; }

private:
    PidController pid;
};

// Global controller instance (defined in main.cpp)
extern AutoThrottleController atc;
