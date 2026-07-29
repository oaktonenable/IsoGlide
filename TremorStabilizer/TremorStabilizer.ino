#include "Arduino_BMI270_BMM150.h"

void setup() {
  Serial.begin(115200);

  if (!IMU.begin()) {
    Serial.println("IMU initialization failed");
    while (true) {
      delay(1000);
    }
  }

  Serial.println("IMU test started");
}

void loop() {
  float ax, ay, az;

  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(ax, ay, az);

    Serial.print("Accel: X=");
    Serial.print(ax, 4);
    Serial.print(" Y=");
    Serial.print(ay, 4);
    Serial.print(" Z=");
    Serial.println(az, 4);
  }

  delay(100);
}
