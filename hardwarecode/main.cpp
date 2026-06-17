/**
 * main.cpp
 * ESP32 — MPU-6050 + TCRT5000 IR RPM Sensor + ESC Controller  v3.2
 *
 * PlatformIO build (platformio.ini: esp32doit-devkit-v1, espressif32, Arduino)
 *
 * Wiring:
 *   MPU-6050  SDA → GPIO 21  |  SCL → GPIO 22  |  VCC → 3.3 V  |  GND → GND
 *   ESC signal    → GPIO  4
 *   TCRT5000 DO   → GPIO 16  (falling-edge, 1 pulse/revolution)
 *   Reset button  → GPIO 23  (active LOW, internal pull-up)
 *   Onboard LED   → GPIO  2  (blinks on each IR pulse)
 *
 * CRITICAL — setup() order:
 *   1. escPwmInit()  runs first — sends 1000 µs to ESC within ms of boot.
 *   2. Everything else (Serial banner, GPIO, IMU) runs while ESC holds 1000 µs.
 *   3. armESC()      runs last  — confirms 1000 µs and waits 3 s for ESC beeps.
 */

// ── Include order matters: each file depends on the ones above it ───────────
#include "config.h"        // 1. constants — no dependencies
#include "imu.h"           // 2. IMU + Kalman
#include "rpm.h"           // 3. IR/RPM (uses motorRunning from motor.h — declared extern)
#include "motor.h"         // 4. ESC + motor control
#include "auto_throttle.h" // 5. closed-loop auto-throttle
#include "serial_cmd.h"    // 6. serial commands (uses motor + imu)
#include "telemetry.h"     // 7. telemetry (uses all of the above)

// ── Auto-throttle controller instance ────────────────────────────────────────
static AutoThrottleController atc;

// ═════════════════════════════════════════════════════════════════════════════
// setup
// ═════════════════════════════════════════════════════════════════════════════
void setup()
{
    // ── STEP 1: ESC PWM init — must be first ─────────────────────────────────
    // Drives 1000 µs on GPIO 4 within a few ms of boot.
    // ESC will beep once to confirm it received the arming signal.
    escPwmInit();

    // ── STEP 2: Serial ────────────────────────────────────────────────────────
    Serial.begin(115200);
    delay(100);

    Serial.println("DEVICE_INFO:esp32|VERSION:3.2|COMPONENTS:MPU6050,TCRT5000|BAUD:115200");
    Serial.println("==============================================");
    Serial.println("  ESP32 IMU + RPM + ESC CONTROLLER  v3.2");
    Serial.println("==============================================");
    Serial.println("Auto-throttle OFF by default: use 'A' to enable");
    Serial.println("Keys: Z=Ref  S=Start  X=Stop  H=Up  L=Down  A=Auto");
    Serial.println("CMDs: CMD:MOTOR_START / STOP / EMERGENCY_STOP");
    Serial.println("      CMD:SPEED_UP / SPEED_DOWN / RESET_REF");
    Serial.println("      CMD:AUTO_ON / AUTO_OFF");
    Serial.println("==============================================");
    Serial.println();

    // ── STEP 3: GPIO ──────────────────────────────────────────────────────────
    pinMode(PIN_RESET_BTN, INPUT_PULLUP);
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);
    pinMode(PIN_TCRT, INPUT);

    // ── STEP 4: IR interrupt ──────────────────────────────────────────────────
    attachInterrupt(digitalPinToInterrupt(PIN_TCRT), irPulseISR, FALLING);

    // ── STEP 5: IMU ──────────────────────────────────────────────────────────
    Wire.begin(21, 22);
    Wire.setClock(400000);
    initMPU6050();

    // ── STEP 6: Arm ESC (3-second wait — listen for ESC beep sequence) ────────
    armESC();

    // ── STEP 7: Seed timing & reset PID ────────────────────────────────────
    lastTelemetryMs = millis();
    lastRpmReportMs = millis();
    atc.reset();
    Serial.println("STATUS:AUTO=OFF");
}

// ═════════════════════════════════════════════════════════════════════════════
// loop
// ═════════════════════════════════════════════════════════════════════════════
void loop()
{
    // ── Hardware reset button (50 ms debounce) ────────────────────────────────
    if (digitalRead(PIN_RESET_BTN) == LOW)
    {
        delay(50);
        if (digitalRead(PIN_RESET_BTN) == LOW)
        {
            Serial.println("[Button] Reset reference");
            resetReference();
            while (digitalRead(PIN_RESET_BTN) == LOW) {}
        }
    }

    // ── IMU read + angle processing ───────────────────────────────────────────
    if (readMPU6050())
    {
        updateAngles();
        updateAngleErrors();
        updateVibration();

        if (!refSet && imuInitialized)
            resetReference();
    }

    // ── RPM ───────────────────────────────────────────────────────────────────
    processRPMPulse();
    checkRPMTimeout();
    publishRPMValue();

    // ── Auto-throttle (single-loop PID: tilt → throttle) ───────────────────
    // Uses real-time tilt (not RPM) because the motor responds fast enough
    // and the RPM sensor is too slow for a tight inner loop.
    if (autoMode && motorRunning && refSet)
    {
        float tiltMag = max(fabsf(errorX), fabsf(errorY));
        int   target  = atc.update(tiltMag);
        setThrottleTarget(target);
    }

    // ── Non-blocking throttle ramp ─────────────────────────────────────────
    rampStep();

    // ── Serial commands ──────────────────────────────────────────────────────
    pollSerial();

    // ── Telemetry ─────────────────────────────────────────────────────────────
    sendTelemetry();
}
