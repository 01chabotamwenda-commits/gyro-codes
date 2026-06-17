/**
 * auto_throttle.h
 * Cascade controller for ESP32 gyro stabilizer.
 *
 * Architecture:  Tilt -> RPM target (outer PID) -> Throttle (inner PID + Majorana)
 *
 * -- Outer loop (1 Hz nominal) ----------------------------------------
 *   Input:  tiltMag (deg) -- max(|errorX|, |errorY|)
 *   Output: rpmTarget (RPM)
 *
 *   rpmTarget = majoranaRpm(tiltMag) + outerPid(tiltMag)
 *   majoranaRpm(tilt) = omega_s_min(tilt) * 60 / (2*pi)
 *   omega_s_min(tilt) = sqrt( (4*It*m*g*h / Is^2) * sin(theta) )
 *
 * -- Inner loop (every Ts = 10 ms) -----------------------------------
 *   Input:  rpmTarget - rpmActual
 *   Output: throttle correction
 *
 *   throttle = rpmToThrottle(rpmTarget) + innerPid(rpmTarget - rpmActual)
 *
 *   rpmToThrottle(rpm) = rpm / Kv / V_batt -> servo units
 *   Kv -- motor RPM per volt (estimate ~6000 RPM/V for small drone motors)
 *
 * -- Anti-windup -------------------------------------------------------
 *   Both loops: conditional integration -- stop integrating when output
 *   is saturated in the same direction as the error.
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

struct PidLimits
{
    float integral = 0.0f;
    float output   = 0.0f;
};

// ---------------------------------------------------------------------------
// Generic PID with conditional integration
// ---------------------------------------------------------------------------
class PidController
{
public:
    PidGains gains;
    PidLimits limits;
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

        float derivative = (error - _prevError) / dt;
        _prevError = error;

        // Conditional integration anti-windup
        float outTest = gains.Kp * error + gains.Ki * _integral + gains.Kd * derivative;
        bool satHigh = (outTest >= limits.output) && (error > 0.0f);
        bool satLow  = (outTest <= -limits.output) && (error < 0.0f);
        if (!satHigh && !satLow)
        {
            _integral += error * dt;
            _integral = constrain(_integral, -limits.integral, limits.integral);
        }

        float out = gains.Kp * error + gains.Ki * _integral + gains.Kd * derivative;
        return constrain(out, -limits.output, limits.output);
    }

private:
    float _integral = 0.0f;
    float _prevError = 0.0f;
    bool _firstCall = true;
};

// ---------------------------------------------------------------------------
// AutoThrottleConfig
// ---------------------------------------------------------------------------
struct AutoThrottleConfig
{
    // -- Outer loop (tilt -> RPM target) --------------------------------
    PidGains outerGains;
    PidLimits outerLimits;

    // -- Inner loop (RPM error -> throttle) -----------------------------
    PidGains innerGains;
    PidLimits innerLimits;

    // -- Sampling period (seconds) ----------------------------------------
    float Ts = 0.01f;

    // -- Motor <-> RPM model --------------------------------------------
    float rpmKv = 6000.0f;        // motor RPM per volt (estimate)
    float batteryVoltage = 11.1f; // 3S LiPo nominal [V]
    float supplyScale = 0.8f;     // Simulink supply scaling factor

    // -- Majorana criterion ---------------------------------------------
    float It = 2.5e-5f;   // transverse moment of inertia [kg*m^2]
    float Is = 5.0e-5f;   // spin moment of inertia [kg*m^2]
    float m  = 0.050f;    // rotor mass [kg]
    float h  = 0.015f;    // CoM height above pivot [m]
    float g  = 9.81f;     // gravitational acceleration [m/s^2]
    float Ke = 0.003f;    // back-EMF constant [V*s/rad]

    // -- Tilt dead-band -------------------------------------------------
    float tiltDeadDeg = 0.1f;

    // -- Constructor ----------------------------------------------------
    AutoThrottleConfig()
    {
        // Default outer loop (tilt -> RPM): conservative
        outerGains.Kp = 500.0f;    // 500 RPM per degree
        outerGains.Ki = 20.0f;     // slow integral
        outerGains.Kd = 50.0f;     // damping
        outerLimits.integral = 5000.0f;
        outerLimits.output   = 5000.0f;

        // Default inner loop (RPM -> throttle): faster
        innerGains.Kp = 0.08f;     // 0.08 throttle units per RPM error
        innerGains.Ki = 0.002f;    // slow buildup
        innerGains.Kd = 0.02f;     // damping
        innerLimits.integral = 20.0f;
        innerLimits.output   = 40.0f;
    }
};

// ---------------------------------------------------------------------------
// AutoThrottleController
// ---------------------------------------------------------------------------
class AutoThrottleController
{
public:
    AutoThrottleConfig cfg;
    PidController outerPid;
    PidController innerPid;

    AutoThrottleController() { reset(); }

    void reset()
    {
        outerPid.reset();
        innerPid.reset();
        _vNowPrev = 0.0f;
    }

    /**
     * update() -- cascade controller
     *
     * @param tiltMag      max(|errorX|, |errorY|) in degrees
     * @param rpmActual    current RPM from IR sensor
     * @param throttleNow  current throttle (for supply path)
     * @return             new target throttle
     */
    int update(float tiltMag, uint32_t rpmActual, int throttleNow)
    {
        // -- 0. Dead-band --------------------------------------------------
        if (tiltMag < cfg.tiltDeadDeg)
            tiltMag = 0.0f;

        // -- 1. Majorana feedforward -> minimum RPM for stability ----------
        float tiltRad = tiltMag * (float)DEG_TO_RAD;
        float sinTilt = sinf(tiltRad);
        if (sinTilt < 0.0f) sinTilt = 0.0f;

        float majoranaGain = (4.0f * cfg.It * cfg.m * cfg.g * cfg.h)
                             / (cfg.Is * cfg.Is);
        float omegaMinRad = (majoranaGain * sinTilt > 0.0f)
                            ? sqrtf(majoranaGain * sinTilt)
                            : 0.0f;
        // omega [rad/s] -> RPM:  RPM = omega * 60 / (2*pi)
        float majoranaRpm = omegaMinRad * 60.0f / (2.0f * (float)PI);

        // -- 2. Outer loop: tilt -> RPM target -----------------------------
        //   error = 0 - tiltMag (we want zero tilt)
        float tiltError = 0.0f - tiltMag;
        outerPid.gains = cfg.outerGains;
        outerPid.limits = cfg.outerLimits;
        outerPid.Ts = cfg.Ts;
        float outerOut = outerPid.update(tiltError);

        // RPM target = minimum for stability + correction from tilt deviation
        float rpmTarget = majoranaRpm + outerOut;
        if (rpmTarget < 0.0f) rpmTarget = 0.0f;

        // -- 3. Inner loop: RPM error -> throttle correction ---------------
        // If RPM is 0, the sensor is dead (motor not spinning yet). Use feedforward only.
        float innerOut = 0.0f;
        float rpmError = 0.0f;
        if (rpmActual > 0)
        {
            rpmError = rpmTarget - (float)rpmActual;
            innerPid.gains = cfg.innerGains;
            innerPid.limits = cfg.innerLimits;
            innerPid.Ts = cfg.Ts;
            innerOut = innerPid.update(rpmError);
        }
        else
        {
            innerPid.reset();  // clear PID state when sensor is dead
        }

        // -- 4. Feedforward throttle from RPM target -----------------------
        //   RPM = Kv * V  ->  V = RPM / Kv
        //   throttle = (V / V_batt) * range + THROTTLE_MIN
        float rpmVoltage = rpmTarget / cfg.rpmKv;
        float vSupply = fabsf(rpmVoltage * cfg.supplyScale);
        vSupply = constrain(vSupply, 0.0f, cfg.batteryVoltage);
        float throttleFF = voltageToThrottle(vSupply);

        // -- 5. Total throttle = feedforward + inner PID correction --------
        float throttleOut = throttleFF + innerOut;
        _vNowPrev = vSupply;
        _lastRpmTarget = rpmTarget;
        _lastRpmError  = rpmError;

        return (int)constrain(throttleOut,
                              (float)THROTTLE_MIN,
                              (float)THROTTLE_MAX);
    }

    // -- Conversion helpers ---------------------------------------------
    float voltageToThrottle(float v) const
    {
        float t = v / cfg.batteryVoltage;
        t = constrain(t, 0.0f, 1.0f);
        return (float)THROTTLE_MIN + t * (float)(THROTTLE_MAX - THROTTLE_MIN);
    }

    // Getters for telemetry
    float getLastRpmTarget() const { return _lastRpmTarget; }
    float getLastRpmError()  const { return _lastRpmError; }

private:
    float _vNowPrev = 0.0f;
    float _lastRpmTarget = 0.0f;
    float _lastRpmError  = 0.0f;
};
