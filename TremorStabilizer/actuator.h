#ifndef ACTUATOR_H
#define ACTUATOR_H

#include "DRV8833.h"

enum class Axis{
    PITCH,
    YAW
};
class TremorActuator {
    public:
        TremorActuator(Axis axis, uint8_t in1Pin, uint8_t in2Pin);
        bool begin();
        void applyCorrection(float tremorRate);
        void stop();
    private:
        Axis axis;
        DRV8833 driver;
        static constexpr float MAX_TREMOR_RATE = 200.0f; // Maximum tremor rate in degrees per second
};

#endif