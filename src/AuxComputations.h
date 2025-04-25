#pragma once

#include "AudioPlayer.h"

namespace AuxComputations{
    typedef struct{
        float r;
        float g;
        float b;
    } RGBColor;
    
    void HSBtoRGB(int h, float s, float b, AuxComputations::RGBColor& target);
    
    void computeRMSValueStereo(RingBuffer<float>& ringBuffer, size_t amtOfSamples, float& leftVal, float& rightVal);
    
    void computePeakValueStereo(RingBuffer<float>& ringBuffer, size_t amtOfSamples, float& leftVal, float& rightVal);
    
    void computeDecibelLevels(float leftRMS, float rightRMS, float& leftDB, float& rightDB);

    float expSmooth(float prev, float curr, float smoothingFactor);
}
