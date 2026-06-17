/**
 * serial_cmd.h
 * Non-blocking serial line reader, CMD: protocol handler,
 * and single-key shortcut handler.
 *
 * Depends on: config.h, motor.h, imu.h
 *
 * ── Command protocol ─────────────────────────────────────────────────────────
 *   CMD:MOTOR_START      start motor
 *   CMD:MOTOR_STOP       stop motor gracefully
 *   CMD:EMERGENCY_STOP   immediate ESC disarm
 *   CMD:SPEED_UP         +THROTTLE_STEP (manual mode only)
 *   CMD:SPEED_DOWN       -THROTTLE_STEP (manual mode only)
 *   CMD:RESET_REF        zero reference angles
 *   CMD:AUTO_ON          enable tilt-based auto-throttle
 *   CMD:AUTO_OFF         disable auto-throttle
 *
 * ── Single-key shortcuts ──────────────────────────────────────────────────────
 *   Z  reset reference      S  start motor      X  stop motor
 *   H  throttle up          L  throttle down     A  toggle auto mode
 */

#pragma once
#include "config.h"

// ─────────────────────────────────────────────────────────────────────────────
// Serial line buffer
// ─────────────────────────────────────────────────────────────────────────────
static char    serialBuf[64];
static uint8_t serialLen = 0;

// ─────────────────────────────────────────────────────────────────────────────
// CMD: protocol handler
// ─────────────────────────────────────────────────────────────────────────────
static void handleCommand(const char *cmd)
{
    if (strcmp(cmd, "CMD:MOTOR_START") == 0)
    {
        startMotor();
    }
    else if (strcmp(cmd, "CMD:MOTOR_STOP") == 0)
    {
        stopMotor();
    }
    else if (strcmp(cmd, "CMD:EMERGENCY_STOP") == 0)
    {
        emergencyStop();
    }
    else if (strcmp(cmd, "CMD:RESET_REF") == 0)
    {
        resetReference();
    }
    else if (strcmp(cmd, "CMD:AUTO_ON") == 0)
    {
        autoMode = true;
        Serial.println("STATUS:AUTO=ON");
    }
    else if (strcmp(cmd, "CMD:AUTO_OFF") == 0)
    {
        autoMode = false;
        Serial.println("STATUS:AUTO=OFF");
    }
    else if (strcmp(cmd, "CMD:SPEED_UP") == 0)
    {
        if (!motorRunning) return;
        if (autoMode)  { Serial.println("[INFO] SPEED_UP ignored — auto mode ON"); return; }
        setThrottleTarget(throttle + THROTTLE_STEP);
    }
    else if (strcmp(cmd, "CMD:SPEED_DOWN") == 0)
    {
        if (!motorRunning) return;
        if (autoMode)  { Serial.println("[INFO] SPEED_DOWN ignored — auto mode ON"); return; }
        setThrottleTarget(throttle - THROTTLE_STEP);
    }
    else
    {
        Serial.print("STATUS:CMD_UNKNOWN=");
        Serial.println(cmd);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Single-key handler
// ─────────────────────────────────────────────────────────────────────────────
static void handleKey(char c)
{
    switch (c)
    {
    case 'Z': case 'z':
        Serial.println("[Key] Reset reference");
        resetReference();
        break;
    case 'S': case 's':
        Serial.println("[Key] Start motor");
        startMotor();
        break;
    case 'X': case 'x':
        Serial.println("[Key] Stop motor");
        stopMotor();
        break;
    case 'H': case 'h':
        if (!motorRunning) { Serial.println("[Key] Speed up ignored (motor off)"); break; }
        if (autoMode)      { Serial.println("[Key] Speed up ignored (auto ON)"); break; }
        Serial.println("[Key] Speed up");
        setThrottleTarget(throttle + THROTTLE_STEP);
        break;
    case 'L': case 'l':
        if (!motorRunning) { Serial.println("[Key] Speed down ignored (motor off)"); break; }
        if (autoMode)      { Serial.println("[Key] Speed down ignored (auto ON)"); break; }
        Serial.println("[Key] Speed down");
        setThrottleTarget(throttle - THROTTLE_STEP);
        break;
    case 'A': case 'a':
        autoMode = !autoMode;
        Serial.println(autoMode ? "STATUS:AUTO=ON" : "STATUS:AUTO=OFF");
        break;
    default:
        Serial.printf("[Key] Unknown: '%c'\n", c);
        break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Non-blocking byte-by-byte serial reader — call every loop()
// Accumulates into serialBuf until '\n', then dispatches.
// Single characters typed without Enter are dispatched immediately.
// ─────────────────────────────────────────────────────────────────────────────
static void pollSerial()
{
    while (Serial.available() > 0)
    {
        char c = (char)Serial.read();
        if (c == '\r') continue;

        if (c == '\n')
        {
            serialBuf[serialLen] = '\0';
            if (serialLen > 0)
            {
                if (strncmp(serialBuf, "CMD:", 4) == 0)
                    handleCommand(serialBuf);
                else if (serialLen == 1)
                    handleKey(serialBuf[0]);
                else
                    Serial.printf("[Serial] Unknown input: %s\n", serialBuf);
            }
            serialLen = 0;
        }
        else
        {
            // Immediate single-char dispatch (no Enter pressed yet)
            if (serialLen == 0 && Serial.available() == 0)
            {
                handleKey(c);
            }
            else
            {
                if (serialLen < (uint8_t)(sizeof(serialBuf) - 1))
                    serialBuf[serialLen++] = c;
            }
        }
    }
}
