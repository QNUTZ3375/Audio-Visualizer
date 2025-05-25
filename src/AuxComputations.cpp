#include "AuxComputations.h"

void AuxComputations::fillArrayWithSamples(RingBuffer<float>& ringBuffer, std::vector<float>& outArray, size_t amtOfSamples){
    for(int i = 0; i < amtOfSamples; i++){
        if(!ringBuffer.pop(outArray[i])){
            break;
        }
    }
}

void AuxComputations::HSBtoRGB(int h, float s, float b, AuxComputations::RGBColor& target){
    h = h % 360;
    if(s < 0) s = 0;
    if(s > 1) s = 1;
    if(b < 0) b = 0;
    if(b > 1) b = 1;

    float c = b * s;
    float x = c * (1 - abs((fmod(h/60.0f, 2.0f) - 1)));
    float m = b - c;
    AuxComputations::RGBColor primes = {0, 0, 0};
    if(h >= 0 && h < 60) primes = {c, x, 0};
    if(h >= 60 && h < 120) primes = {x, c, 0};
    if(h >= 120 && h < 180) primes = {0, c, x}; 
    if(h >= 180 && h < 240) primes = {0, x, c}; 
    if(h >= 240 && h < 300) primes = {x, 0, c}; 
    if(h >= 300 && h < 360) primes = {c, 0, x};

    target = {primes.r + m, primes.g + m, primes.b + m };
}

void AuxComputations::computeRMSValueStereo(std::vector<float>& arraySamples, size_t amtOfSamples, float& leftVal, float& rightVal){
    float sumSquaresLeft = 0.0f;
    float sumSquaresRight = 0.0f;

    for(int i = 0; i < amtOfSamples; i+=2){
        if(std::isnan(arraySamples[i])) break;
        float currSampleLeft = arraySamples[i];
        sumSquaresLeft += currSampleLeft * currSampleLeft;

        if(std::isnan(arraySamples[i+1])) break;
        float currSampleRight = arraySamples[i+1];
        sumSquaresRight += currSampleRight * currSampleRight;
    }
    leftVal = sqrt(sumSquaresLeft / (float) amtOfSamples);
    rightVal = sqrt(sumSquaresRight / (float) amtOfSamples);
}

void AuxComputations::computePeakValueStereo(std::vector<float>& arraySamples, size_t amtOfSamples, float& leftVal, float& rightVal){
    float maxLeft = 0.0f;
    float maxRight = 0.0f;

    for(int i = 0; i < amtOfSamples; i+=2){
        if(std::isnan(arraySamples[i])) break;
        float currSampleLeft = arraySamples[i];
        if(maxLeft < currSampleLeft) maxLeft = currSampleLeft;

        if(std::isnan(arraySamples[i+1])) break;
        float currSampleRight = arraySamples[i+1];
        if(maxRight < currSampleRight) maxRight = currSampleRight;
    }
    leftVal = maxLeft;
    rightVal = maxRight;
}

void AuxComputations::computeDecibelLevels(float leftRMS, float rightRMS, float& leftDB, float& rightDB){
    leftDB = 20.0f * log10f(leftRMS + 1e-10f);
    rightDB = 20.0f * log10f(rightRMS + 1e-10f);
}

float AuxComputations::expSmooth(float prev, float curr, float smoothingFactor){
    return smoothingFactor * prev + (1 - smoothingFactor) * curr;
}
