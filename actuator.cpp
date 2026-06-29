#include "actuator.h"
#include <Wire.h>
#include <cmath>

TremorActuator::TremorActuator(Axis axis, uint8_t i2cAddress)
    : axis(axis), i2cAddress(i2cAddress){}

bool TremorActuator::begin() {
    if(!driver.begin()){
        return false;
    }
    stop();
    return true;
}

void TremorActuator::applyCorrection(float tremorRate){
    if(tremorRate > MAX_TREMOR_RATE)
        tremorRate = MAX_TREMOR_RATE;
    else if(tremorRate < -MAX_TREMOR_RATE)
        tremorRate = -MAX_TREMOR_RATE;
    float normalized = (tremorRate / MAX_TREMOR_RATE) * 127.0f;
    int8_t rtpValue = static_cast<int8_t>(normalized);
    driver.setRealtimeValue(rtpValue);
}

void TremorActuator::stop(){
    driver.setRealtimeValue(0);
}