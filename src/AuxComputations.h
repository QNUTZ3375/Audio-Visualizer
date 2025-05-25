#pragma once

#include <vector>
#include <fftw3-mpi.h>
#include "AudioPlayer.h"

namespace AuxComputations{
    typedef struct{
        float r;
        float g;
        float b;
    } RGBColor;

    void fillArrayWithSamples(RingBuffer<float>& ringBuffer, std::vector<float>& outArray, size_t amtOfSamples);
    
    void HSBtoRGB(int h, float s, float b, AuxComputations::RGBColor& target);
    
    void computeRMSValueStereo(std::vector<float>& arraySamples, size_t amtOfSamples, float& leftVal, float& rightVal);
    
    void computePeakValueStereo(std::vector<float>& arraySamples, size_t amtOfSamples, float& leftVal, float& rightVal);
    
    void computeDecibelLevels(float leftRMS, float rightRMS, float& leftDB, float& rightDB);

    float expSmooth(float prev, float curr, float smoothingFactor);
}
