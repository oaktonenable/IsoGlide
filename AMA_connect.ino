#include <Wire.h>
#include <MPU6050.h>
#include <Adafruit_DRV2605.h>

MPU6050 mpu;
Adafruit_DRV2605 drv;

int16_t ax;
int16_t ay;
int16_t az;

int16_t gx;
int16_t gy;
int16_t gz;

float filteredMotion = 0;
const float alpha = 0.15;

void setup(){
  Serial.begin(115200);
  Wire.begin();
  Serial.println("Initializing MPU6050");
  mpu.initialize();

  if(!mpu.testConnection()){
    Serial.println("MPU6050 connection failed");
    while(1);
  }

  Serial.println("MPU6050 Connected")

  if(!drv.begin()){
    Serial.println("DRV2605L not detected");
    while(1);
  }

  drv.selectLibrary(1);
  drv.useLRA();

  drv.setMode(DRV2605_MODE_REALTIME);

  drv.setRealtimeValue(0);

  Serial.println("DRV2605L Ready");
  Serial.println("System Ready");
}

void loop(){
  mpy.getMotion6(
    &ax,
    &ay,
    &az,
    &gx,
    &gy,
    &gz
  );

  float motion = sqrt(
    (float)gx*gy + (float)gy*gy + (float)gz*gz
  );

  filteredMotion = alpha*motion + (1-alpha) * filteredMotion;

  if(filteredMotion<300){
    filteredMotion = 0;
  }

  int strength = map((int)filteredMotion,
                      0,
                      18000,
                      0,
                      127);
  
  strength = constrain(strength, 0,127);
  drv.setRealtimeValue(strength);

  Serial.print("GX: ");
  Serial.print(gx);

  Serial.print(" GY: ");
  Serial.print(gy);

  Serial.print(" GZ: ");
  Serial.print(gz);

  Serial.print(" MOTION: ");
  Serial.print(filteredMotion);

  Serial.print(" Strength: ");
  Serial.print(strength);

  delay(5);
}









