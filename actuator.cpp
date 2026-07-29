#include "actuator.h"
#include <cmath>

TremorActuator::TremorActuator(Axis axis, uint8_t in1Pin, uint8_t in2Pin)
    : axis(axis), driver(in1Pin, in2Pin){}

bool TremorActuator::begin() {
    driver.begin();
    stop();
    return true;
}

void TremorActuator::applyCorrection(float tremorRate){
    if(tremorRate > MAX_TREMOR_RATE)
        tremorRate = MAX_TREMOR_RATE;
    else if(tremorRate < -MAX_TREMOR_RATE)
        tremorRate = -MAX_TREMOR_RATE;
    float normalized = (tremorRate / MAX_TREMOR_RATE) * 127.0f;
    int8_t speedValue = static_cast<int8_t>(normalized);
    driver.setSpeed(speedValue);
}

void TremorActuator::stop(){
    driver.stop();
}