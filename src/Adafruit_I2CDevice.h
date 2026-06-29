#ifndef ADAFRUIT_I2CDEVICE_H
#define ADAFRUIT_I2CDEVICE_H

#include <Arduino.h>
#include <Wire.h>

class Adafruit_I2CDevice {
public:
  Adafruit_I2CDevice(uint8_t address, TwoWire *wire = &Wire);
  bool begin();
  bool write(uint8_t *buffer, size_t len, bool stop = true);
  bool read(uint8_t *buffer, size_t len, bool stop = true);
  bool write8(uint8_t reg, uint8_t value);
  bool read8(uint8_t reg, uint8_t *value);
  uint8_t address(void) const;

private:
  uint8_t address_;
  TwoWire *wire_;
};

#endif
