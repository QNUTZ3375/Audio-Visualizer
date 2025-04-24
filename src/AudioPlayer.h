#pragma once

#include <stdio.h>
#include <iostream>
#include "RingBuffer.hpp"

#include "vendor/miniaudio/miniaudio.h"

typedef struct{
    RingBuffer<float> *ringBuffer;
    bool trackEnded;
    ma_decoder decoder;
} TrackRingBuffer;

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

int createDevice(ma_device& device, ma_decoder& decoder, const char* filepath, TrackRingBuffer& audioBuffer);

void startAudioCallback(ma_device& device, ma_decoder& decoder);

void printRingBufferContents(RingBuffer<float>& ringBuffer);

void destroyDevice(ma_device& device, ma_decoder& decoder, TrackRingBuffer& audioBuffer);