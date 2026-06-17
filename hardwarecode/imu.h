/**
 * imu.h
 * MPU-6050 initialisation, raw data read, Kalman filter,
 * angle computation, vibration monitor, and reference-angle logic.
 *
 * Depends on: config.h
 */

#pragma once
#include "config.h"

// ─────────────────────────────────────────────────────────────────────────────
// Kalman filter state  (one instance per axis)
// ─────────────────────────────────────────────────────────────────────────────
struct KalmanAxis
{
    float angle = 0.0f;
    float bias  = 0.0f;
    float P[2][2] = {{1.0f, 0.0f}, {0.0f, 1.0f}};
};

static KalmanAxis kfX, kfY, kfZ;

// ─────────────────────────────────────────────────────────────────────────────
// Raw sensor readings  (updated by readMPU6050)
// ─────────────────────────────────────────────────────────────────────────────
static float accelX_g, accelY_g, accelZ_g;
static float gyroX_dps, gyroY_dps, gyroZ_dps;

// ─────────────────────────────────────────────────────────────────────────────
// Computed angles & errors
// ─────────────────────────────────────────────────────────────────────────────
static float currentAngleX = 0.0f, currentAngleY = 0.0f, currentAngleZ = 0.0f;
static float refAngleX     = 0.0f, refAngleY     = 0.0f, refAngleZ     = 0.0f;
static float errorX        = 0.0f, errorY        = 0.0f, errorZ        = 0.0f;

static bool imuInitialized = false;
static bool refSet         = false;
static uint32_t lastImuMicros = 0;

// ─────────────────────────────────────────────────────────────────────────────
// Vibration monitor  (RMS over rolling window)
// ─────────────────────────────────────────────────────────────────────────────
static float   vibrationRms         = 0.0f;
static float   vibrationWindow[VIBRATION_WINDOW] = {0.0f};
static uint8_t vibrationIndex       = 0;
static float   vibrationSumSquares  = 0.0f;
static uint8_t vibrationSampleCount = 0;

// ─────────────────────────────────────────────────────────────────────────────
// Utilities
// ─────────────────────────────────────────────────────────────────────────────
static void normalizeAngle(float &a)
{
    while (a >  180.0f) a -= 360.0f;
    while (a < -180.0f) a += 360.0f;
}

static float mapClamped(float x, float inLo, float inHi, float outLo, float outHi)
{
    if (x <= inLo) return outLo;
    if (x >= inHi) return outHi;
    return outLo + (x - inLo) * (outHi - outLo) / (inHi - inLo);
}

// ─────────────────────────────────────────────────────────────────────────────
// Kalman update  (one axis)
// ─────────────────────────────────────────────────────────────────────────────
static void kalmanUpdate(KalmanAxis &k, float measuredAngle, float gyroRate, float dt)
{
    // Predict
    float rate = gyroRate - k.bias;
    k.angle += dt * rate;
    k.P[0][0] += dt * (dt * k.P[1][1] - k.P[0][1] - k.P[1][0] + KF_Q_ANGLE);
    k.P[0][1] -= dt * k.P[1][1];
    k.P[1][0] -= dt * k.P[1][1];
    k.P[1][1] += KF_Q_BIAS * dt;

    // Update
    float S  = k.P[0][0] + KF_R_MEASURE;
    float K0 = k.P[0][0] / S;
    float K1 = k.P[1][0] / S;
    float y  = measuredAngle - k.angle;
    k.angle += K0 * y;
    k.bias  += K1 * y;
    float p00 = k.P[0][0], p01 = k.P[0][1];
    k.P[0][0] -= K0 * p00;
    k.P[0][1] -= K0 * p01;
    k.P[1][0] -= K1 * p00;
    k.P[1][1] -= K1 * p01;
}

// ─────────────────────────────────────────────────────────────────────────────
// MPU-6050 initialisation
// ─────────────────────────────────────────────────────────────────────────────
static void initMPU6050()
{
    // Wake chip
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(REG_PWR_MGMT1);
    Wire.write(0x00);
    Wire.endTransmission(true);
    delay(100);

    // Accel ±8 g  (0x10 → 4096 LSB/g)
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(REG_ACCEL_CFG);
    Wire.write(0x10);
    Wire.endTransmission(true);
    delay(10);

    // Gyro ±250 °/s  (0x00 → 131 LSB/°/s)
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(REG_GYRO_CFG);
    Wire.write(0x00);
    Wire.endTransmission(true);
    delay(10);
}

// Returns true if 14-byte read succeeded
static bool readMPU6050()
{
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(REG_ACCEL_OUT);
    if (Wire.endTransmission(false) != 0) return false;

    uint8_t got = Wire.requestFrom((uint8_t)MPU_ADDR, (uint8_t)14, (uint8_t)true);
    if (got < 14) return false;

    int16_t ax = (Wire.read() << 8) | Wire.read();
    int16_t ay = (Wire.read() << 8) | Wire.read();
    int16_t az = (Wire.read() << 8) | Wire.read();
    Wire.read(); Wire.read();                        // temperature — discard
    int16_t gx = (Wire.read() << 8) | Wire.read();
    int16_t gy = (Wire.read() << 8) | Wire.read();
    int16_t gz = (Wire.read() << 8) | Wire.read();

    accelX_g  = ax / 4096.0f;
    accelY_g  = ay / 4096.0f;
    accelZ_g  = az / 4096.0f;
    gyroX_dps = gx / 131.0f;
    gyroY_dps = gy / 131.0f;
    gyroZ_dps = gz / 131.0f;

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Vibration monitor
// ─────────────────────────────────────────────────────────────────────────────
static void updateVibration()
{
    float magnitude = sqrtf(accelX_g * accelX_g + accelY_g * accelY_g + accelZ_g * accelZ_g);
    float dynamic   = fabsf(magnitude - 1.0f);  // deviation from 1 g (gravity only)

    vibrationSumSquares -= vibrationWindow[vibrationIndex] * vibrationWindow[vibrationIndex];
    vibrationWindow[vibrationIndex] = dynamic;
    vibrationSumSquares += dynamic * dynamic;
    vibrationIndex = (vibrationIndex + 1) % VIBRATION_WINDOW;

    if (vibrationSampleCount < VIBRATION_WINDOW) vibrationSampleCount++;
    float count = (float)((vibrationSampleCount < VIBRATION_WINDOW)
                          ? vibrationSampleCount : VIBRATION_WINDOW);
    vibrationRms = sqrtf(vibrationSumSquares / count);
}

// ─────────────────────────────────────────────────────────────────────────────
// Angle computation  (accelerometer tilt → Kalman → normalise)
// ─────────────────────────────────────────────────────────────────────────────
static void updateAngles()
{
    float aX = atan2f(accelX_g, sqrtf(accelY_g * accelY_g + accelZ_g * accelZ_g)) * RAD_TO_DEG;
    float aY = atan2f(accelY_g, sqrtf(accelX_g * accelX_g + accelZ_g * accelZ_g)) * RAD_TO_DEG;
    float aZ = atan2f(accelZ_g, sqrtf(accelX_g * accelX_g + accelY_g * accelY_g)) * RAD_TO_DEG;

    uint32_t nowUs = micros();
    float dt = (lastImuMicros == 0) ? 0.01f : (float)(nowUs - lastImuMicros) / 1e6f;
    if (dt <= 0.0f || dt > 0.5f) dt = 0.01f;
    lastImuMicros = nowUs;

    if (!imuInitialized)
    {
        kfX.angle = aX;
        kfY.angle = aY;
        kfZ.angle = aZ;
        imuInitialized = true;
    }
    else
    {
        kalmanUpdate(kfX, aX, gyroX_dps, dt);
        kalmanUpdate(kfY, aY, gyroY_dps, dt);
        kalmanUpdate(kfZ, aZ, gyroZ_dps, dt);
    }

    currentAngleX = kfX.angle; normalizeAngle(currentAngleX);
    currentAngleY = kfY.angle; normalizeAngle(currentAngleY);
    currentAngleZ = kfZ.angle; normalizeAngle(currentAngleZ);
}

// ─────────────────────────────────────────────────────────────────────────────
// Error calculation  (deviation from reference angles)
// ─────────────────────────────────────────────────────────────────────────────
static void updateAngleErrors()
{
    errorX = -(currentAngleX - refAngleX);  // invert X so forward tilt = positive
    errorY =   currentAngleY - refAngleY;
    errorZ =   currentAngleZ - refAngleZ;
    normalizeAngle(errorX);
    normalizeAngle(errorY);
    normalizeAngle(errorZ);

    // Dead-band — suppress sensor noise at rest
    if (fabsf(errorX) < 0.4f) errorX = 0.0f;
    if (fabsf(errorY) < 0.4f) errorY = 0.0f;
    if (fabsf(errorZ) < 0.4f) errorZ = 0.0f;
}

// ─────────────────────────────────────────────────────────────────────────────
// Reference angle capture  (call on 'Z' key or CMD:RESET_REF)
// ─────────────────────────────────────────────────────────────────────────────
static void resetReference()
{
    refAngleX = currentAngleX;
    refAngleY = currentAngleY;
    refAngleZ = currentAngleZ;
    errorX = errorY = errorZ = 0.0f;
    refSet = true;
    Serial.println("STATUS:REF_RESET=OK");
}
