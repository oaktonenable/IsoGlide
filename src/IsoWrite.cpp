#include <Wire.h>
#include <Arduino_BMI270_BMM150.h>

// ============================================================
// TCA9548A
// ============================================================

#define TCA_ADDRESS 0x70

#define TCA_CHANNEL_MOTOR_1 0
#define TCA_CHANNEL_MOTOR_2 1

// ============================================================
// DRV2605L
// ============================================================

#define DRV2605_ADDRESS 0x5A

#define DRV_REG_STATUS        0x00
#define DRV_REG_MODE          0x01
#define DRV_REG_RTP_INPUT     0x02
#define DRV_REG_LIBRARY       0x03
#define DRV_REG_WAVESEQ1      0x04
#define DRV_REG_GO            0x0C

// DRV2605L modes
#define DRV_MODE_INTERNAL_TRIGGER 0
#define DRV_MODE_REAL_TIME        5
#define DRV_MODE_STANDBY          4

// ============================================================
// IMU SETTINGS
// ============================================================

const float SAMPLE_RATE = 100.0f;

// Approximate tremor frequency range.
// This is intentionally broad for the first prototype.
const float TREMOR_LOW_HZ  = 3.0f;
const float TREMOR_HIGH_HZ = 12.0f;

// ============================================================
// HAPTIC SETTINGS
// ============================================================

// Minimum haptic output.
// Set to 0 if you want the motors completely off when
// little tremor is detected.
const int MIN_HAPTIC = 0;

// Maximum DRV2605L real-time playback value.
const int MAX_HAPTIC = 127;

// Amount of smoothing.
// Higher = smoother but slower response.
const float SMOOTHING = 0.85f;

// Threshold for detected motion.
// You will probably need to tune this experimentally.
const float TREMOR_THRESHOLD = 0.035f;

// ============================================================
// FILTER VARIABLES
// ============================================================

float previousMagnitude = 0.0f;

float lowPassed = 0.0f;
float highPassed = 0.0f;

float previousHighPassed = 0.0f;

float tremorStrength = 0.0f;

// ============================================================
// TIMING
// ============================================================

unsigned long lastSampleTime = 0;

const unsigned long SAMPLE_PERIOD_US = 10000; // 100 Hz


// ============================================================
// TCA9548A FUNCTIONS
// ============================================================

void selectTCAChannel(uint8_t channel) {

  if (channel > 7) {
    return;
  }

  Wire.beginTransmission(TCA_ADDRESS);
  Wire.write(1 << channel);
  Wire.endTransmission();
}


// ============================================================
// DRV2605L FUNCTIONS
// ============================================================

void writeDRVRegister(uint8_t reg, uint8_t value) {

  Wire.beginTransmission(DRV2605_ADDRESS);

  Wire.write(reg);
  Wire.write(value);

  Wire.endTransmission();
}


uint8_t readDRVRegister(uint8_t reg) {

  Wire.beginTransmission(DRV2605_ADDRESS);
  Wire.write(reg);
  Wire.endTransmission(false);

  Wire.requestFrom(DRV2605_ADDRESS, (uint8_t)1);

  if (Wire.available()) {
    return Wire.read();
  }

  return 0;
}


// ============================================================
// INITIALIZE ONE DRV2605L
// ============================================================

bool initializeDRV2605(uint8_t channel) {

  selectTCAChannel(channel);

  delay(5);

  // Check whether the DRV2605L responds.
  Wire.beginTransmission(DRV2605_ADDRESS);

  if (Wire.endTransmission() != 0) {

    Serial.print("DRV2605L not detected on TCA channel ");
    Serial.println(channel);

    return false;
  }

  Serial.print("DRV2605L detected on TCA channel ");
  Serial.println(channel);


  // Put driver into standby before configuring it.
  writeDRVRegister(DRV_REG_MODE, DRV_MODE_STANDBY);

  delay(10);


  // Use the LRA library.
  //
  // Library 6 is commonly used for LRA effects.
  //
  writeDRVRegister(DRV_REG_LIBRARY, 6);


  // Select real-time playback mode.
  writeDRVRegister(DRV_REG_MODE, DRV_MODE_REAL_TIME);

  delay(10);


  // Start with motor off.
  writeDRVRegister(DRV_REG_RTP_INPUT, 0);

  return true;
}


// ============================================================
// SET MOTOR STRENGTH
// ============================================================

void setMotorStrength(uint8_t channel, int strength) {

  strength = constrain(strength, 0, MAX_HAPTIC);

  selectTCAChannel(channel);

  writeDRVRegister(DRV_REG_RTP_INPUT, strength);
}


// ============================================================
// IMU
// ============================================================

float getAccelerationMagnitude() {

  float x;
  float y;
  float z;

  if (!IMU.accelerationAvailable()) {
    return previousMagnitude;
  }

  IMU.readAcceleration(x, y, z);

  // Magnitude of acceleration vector.
  float magnitude = sqrt(
    x * x +
    y * y +
    z * z
  );

  return magnitude;
}


// ============================================================
// SIMPLE TREMOR FILTER
// ============================================================
//
// This is not a clinical tremor classifier.
//
// It attempts to isolate relatively fast movement from
// slower hand movement by using high-pass filtering.
//
// ============================================================

float processTremor(float accelerationMagnitude) {

  // Remove the approximate static gravity component.
  float movement = accelerationMagnitude - 1.0f;


  // First low-pass filter.
  lowPassed =
      (SMOOTHING * lowPassed)
      +
      ((1.0f - SMOOTHING) * movement);


  // High-pass component.
  highPassed = movement - lowPassed;


  // Rectify the signal.
  float tremorSignal = abs(highPassed);


  // Smooth tremor strength.
  tremorStrength =
      (SMOOTHING * tremorStrength)
      +
      ((1.0f - SMOOTHING) * tremorSignal);


  return tremorStrength;
}


// ============================================================
// CONVERT TREMOR STRENGTH TO HAPTIC STRENGTH
// ============================================================

int calculateHapticStrength(float tremor) {

  if (tremor < TREMOR_THRESHOLD) {
    return MIN_HAPTIC;
  }


  float normalized =
      (tremor - TREMOR_THRESHOLD)
      /
      (0.20f - TREMOR_THRESHOLD);


  normalized = constrain(normalized, 0.0f, 1.0f);


  int output =
      MIN_HAPTIC
      +
      (int)(normalized * MAX_HAPTIC);


  return constrain(output, 0, MAX_HAPTIC);
}


// ============================================================
// SETUP
// ============================================================

void setup() {

  Serial.begin(115200);

  while (!Serial) {
    delay(10);
  }


  Serial.println();
  Serial.println("====================================");
  Serial.println("Tremor Haptic Feedback Prototype");
  Serial.println("Nano 33 BLE + TCA9548A + 2x DRV2605L");
  Serial.println("====================================");


  // ----------------------------------------------------------
  // I2C
  // ----------------------------------------------------------

  Wire.begin();

  // 400 kHz is useful for responsive I2C communication.
  Wire.setClock(400000);

  delay(100);


  // ----------------------------------------------------------
  // Check TCA9548A
  // ----------------------------------------------------------

  Serial.println();
  Serial.println("Checking TCA9548A...");

  Wire.beginTransmission(TCA_ADDRESS);

  if (Wire.endTransmission() == 0) {

    Serial.println("TCA9548A detected!");

  } else {

    Serial.println("ERROR: TCA9548A not detected.");
    Serial.println("Check VCC, GND, SDA and SCL.");

    while (1) {
      delay(1000);
    }
  }


  // ----------------------------------------------------------
  // Initialize IMU
  // ----------------------------------------------------------

  Serial.println();
  Serial.println("Initializing Nano 33 BLE IMU...");

  if (!IMU.begin()) {

    Serial.println("ERROR: IMU failed to initialize.");

    while (1) {
      delay(1000);
    }
  }

  Serial.println("IMU initialized!");

  Serial.print("Accelerometer sample rate: ");
  Serial.print(IMU.accelerationSampleRate());
  Serial.println(" Hz");


  // ----------------------------------------------------------
  // Initialize DRV #1
  // ----------------------------------------------------------

  Serial.println();
  Serial.println("Initializing DRV2605L #1...");

  bool motor1OK =
      initializeDRV2605(TCA_CHANNEL_MOTOR_1);


  // ----------------------------------------------------------
  // Initialize DRV #2
  // ----------------------------------------------------------

  Serial.println();
  Serial.println("Initializing DRV2605L #2...");

  bool motor2OK =
      initializeDRV2605(TCA_CHANNEL_MOTOR_2);


  // ----------------------------------------------------------
  // Check motors
  // ----------------------------------------------------------

  if (!motor1OK || !motor2OK) {

    Serial.println();
    Serial.println("WARNING:");
    Serial.println("One or both DRV2605Ls were not detected.");
    Serial.println("Check the TCA channels and wiring.");

  } else {

    Serial.println();
    Serial.println("Both DRV2605Ls detected!");
  }


  // Make absolutely sure motors start OFF.
  setMotorStrength(TCA_CHANNEL_MOTOR_1, 0);
  setMotorStrength(TCA_CHANNEL_MOTOR_2, 0);


  Serial.println();
  Serial.println("System ready.");
  Serial.println();
  Serial.println("Starting tremor detection...");
  Serial.println();


  lastSampleTime = micros();
}


// ============================================================
// MAIN LOOP
// ============================================================

void loop() {

  unsigned long currentTime = micros();


  // Run at approximately 100 Hz.
  if (currentTime - lastSampleTime < SAMPLE_PERIOD_US) {
    return;
  }

  lastSampleTime += SAMPLE_PERIOD_US;


  // ----------------------------------------------------------
  // READ IMU
  // ----------------------------------------------------------

  float accelerationMagnitude =
      getAccelerationMagnitude();


  previousMagnitude = accelerationMagnitude;


  // ----------------------------------------------------------
  // PROCESS TREMOR
  // ----------------------------------------------------------

  float detectedTremor =
      processTremor(accelerationMagnitude);


  // ----------------------------------------------------------
  // CALCULATE HAPTIC OUTPUT
  // ----------------------------------------------------------

  int hapticStrength =
      calculateHapticStrength(detectedTremor);


  // ----------------------------------------------------------
  // DRIVE BOTH LRAs
  // ----------------------------------------------------------

  setMotorStrength(
      TCA_CHANNEL_MOTOR_1,
      hapticStrength
  );


  setMotorStrength(
      TCA_CHANNEL_MOTOR_2,
      hapticStrength
  );


  // ----------------------------------------------------------
  // SERIAL MONITOR
  // ----------------------------------------------------------

  static unsigned long lastPrint = 0;

  if (millis() - lastPrint >= 100) {

    lastPrint = millis();

    Serial.print("Accel: ");
    Serial.print(accelerationMagnitude, 3);

    Serial.print(" | Tremor: ");
    Serial.print(detectedTremor, 4);

    Serial.print(" | Haptic: ");
    Serial.println(hapticStrength);
  }
}