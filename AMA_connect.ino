#include <Arduino_BMI270_BMM150.h>
#include "DRV8833.h"

BMI270_BMM150 imu;
DRV8833 drv(5, 6); // Replace with your DRV8833 motor control pins

float ax;
float ay;
float az;

float filteredMotion = 0.0f;
const float alpha = 0.15f;

void setup() {
  Serial.begin(115200);

  if (!imu.begin()) {
    Serial.println("IMU initialization failed");
    while (1);
  }

  Serial.println("Built-in IMU initialized");

  drv.begin();
  drv.setSpeed(0);

  Serial.println("DRV8833 Ready");
  Serial.println("System Ready");
}

void loop() {
  if (imu.accelerationAvailable()) {
    imu.readAcceleration(ax, ay, az);

    float motion = sqrt(ax * ax + ay * ay + az * az) - 1.0f;
    if (motion < 0.0f) {
      motion = 0.0f;
    }

    filteredMotion = alpha * motion + (1.0f - alpha) * filteredMotion;

    if (filteredMotion < 0.05f) {
      filteredMotion = 0.0f;
    }

    int strength = map((int)(filteredMotion * 1000.0f), 0, 4000, 0, 127);
    strength = constrain(strength, 0, 127);
    drv.setSpeed(strength);

    Serial.print("AX: ");
    Serial.print(ax);

    Serial.print(" AY: ");
    Serial.print(ay);

    Serial.print(" AZ: ");
    Serial.print(az);

    Serial.print(" MOTION: ");
    Serial.print(filteredMotion);

    Serial.print(" Strength: ");
    Serial.print(strength);
    Serial.println();
  }

  delay(5);
}

