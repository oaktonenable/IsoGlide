#ifndef ACTUATOR_H
#define ACTUATOR_H

#include "Adafruit_DRV2605.h"

enum class Axis{
    PITCH,
    YAW
};
class TremorActuator {
    public:
        TremorActuator(Axis axis, uint8_t i2cAddress);
        bool begin();
        void applyCorrection(float tremorRate);
        void stop();
    private:
        Axis axis;
        uint8_t i2cAddress;
        Adafruit_DRV2605 driver;
        static constexpr float MAX_TREMOR_RATE = 200.0f; // Maximum tremor rate in degrees per second
};

#endif