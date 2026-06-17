/**
 * config.h
 * All pin assignments, hardware constants, and tuning parameters.
 * Include this first — every other module depends on it.
 */

#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include <ESP32Servo.h>

// ── Pin assignments ───────────────────────────────────────────────────────────
static const int PIN_ESC       = 4;
static const int PIN_RESET_BTN = 23;  // active LOW, internal pull-up
static const int PIN_TCRT      = 16;  // IR sensor — falling edge per revolution
                                       // NOTE: GPIO16 conflicts with PSRAM on
                                       // ESP32-WROVER boards; move if needed.
static const int PIN_LED       = 2;   // onboard LED — blinks each IR pulse

// ── MPU-6050 I2C address & registers ─────────────────────────────────────────
static const uint8_t MPU_ADDR      = 0x68;
static const uint8_t REG_PWR_MGMT1 = 0x6B;
static const uint8_t REG_ACCEL_CFG = 0x1C;
static const uint8_t REG_GYRO_CFG  = 0x1B;
static const uint8_t REG_ACCEL_OUT = 0x3B;

// ── ESC / throttle constants ──────────────────────────────────────────────────
static const int THROTTLE_ARM  = 105;  // ESC arm / idle value
static const int THROTTLE_MIN  = 115;  // minimum running throttle
static const int THROTTLE_MAX  = 180;  // maximum throttle
static const int THROTTLE_STEP = 5;    // manual speed-up / speed-down step

// ── Kalman filter tuning ──────────────────────────────────────────────────────
static const float KF_Q_ANGLE   = 0.001f;
static const float KF_Q_BIAS    = 0.003f;
static const float KF_R_MEASURE = 0.030f;

// ── Telemetry interval ────────────────────────────────────────────────────────
static const uint32_t TELEMETRY_MS = 250;

// ── RPM sensor constants ──────────────────────────────────────────────────────
static const uint32_t RPM_TIMEOUT_MS         = 2000;
static const uint32_t RPM_REPORT_INTERVAL_MS = 250;
static const uint32_t IR_DEBOUNCE_US         = 2500UL;
static const uint32_t IR_MIN_PERIOD_US       = 2000UL;
static const uint32_t IR_MAX_PERIOD_US       = 10000000UL;
static const uint8_t  RPM_AVG_SAMPLES        = 8;

// ── Non-blocking ramp timing ──────────────────────────────────────────────────
static const uint32_t RAMP_STEP_MS = 12;  // ms between each 1-unit throttle increment

// ── Vibration monitor ─────────────────────────────────────────────────────────
static const uint8_t VIBRATION_WINDOW = 16;
