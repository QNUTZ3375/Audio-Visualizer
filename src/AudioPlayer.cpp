#include "AudioPlayer.h"

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount){
    TrackRingBuffer* audioBuffer = (TrackRingBuffer*)pDevice->pUserData;
    if (audioBuffer == NULL) return;
    //prevent unnecessary reads from being written into the buffer
    if(audioBuffer->trackEnded) return;

    ma_decoder *pDecoder = &(audioBuffer->decoder);
    if (pDecoder == NULL) return;

    ma_uint64 framesRead = 0;
    int result = ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount, &framesRead);

    //track whether or not track has ended
    audioBuffer->trackEnded = (result == MA_AT_END) || (framesRead < frameCount);

    float* samples = (float*)pOutput;
    for (ma_uint32 i = 0; i < framesRead * pDecoder->outputChannels; ++i) {
        bool pushStatus = (*audioBuffer->ringBuffer).push(samples[i]);
        if(!pushStatus){
            perror("RING BUFFER FULL");
        }
        // printf("Sample[%d]: %f\n", i, samples[i]);  // <-- You can process here
    }

    (void)pInput; // Avoid unused warning
}

int createDevice(ma_device& device, ma_decoder& decoder, const char* filepath, TrackRingBuffer& audioBuffer){
    ma_result result;
    ma_device_config deviceConfig;
    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 2, 48000);

    result = ma_decoder_init_file(filepath, &config, &decoder);
    if (result != MA_SUCCESS) {
        printf("Failed to open audio file.\n");
        return -1;
    }

    long long unsigned int lengthTrack = 0;
    //length of track in pcm frames = subchunk2size / (num bytes per sample * num channels)
    ma_decoder_get_length_in_pcm_frames(&decoder, &lengthTrack);

    int safetyFactor = 2;
    RingBuffer<float>* ringBuffer = new RingBuffer<float>(lengthTrack * decoder.outputChannels * safetyFactor);

    audioBuffer = {ringBuffer, false, decoder};

    deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format   = decoder.outputFormat;
    deviceConfig.playback.channels = decoder.outputChannels;
    deviceConfig.sampleRate        = decoder.outputSampleRate;
    deviceConfig.dataCallback      = data_callback;
    deviceConfig.pUserData         = &audioBuffer;

    result = ma_device_init(NULL, &deviceConfig, &device);
    if (result != MA_SUCCESS) {
        printf("Failed to initialize playback device.\n");
        ma_decoder_uninit(&decoder);
        return -2;
    }
    return 0;
}

void startAudioCallback(ma_device& device){
    ma_device_start(&device);
    return;
}

void stopAudioCallback(ma_device& device){
    ma_device_stop(&device);
    return;
}

void printRingBufferContents(RingBuffer<float>& ringBuffer){
    size_t sizeBuffer = 0;
    while(!ringBuffer.isEmpty()){
        float arg1;
        float arg2;
        if(!ringBuffer.pop(arg1)) break;
        sizeBuffer++;

        std::cout << arg1;
        if(!ringBuffer.pop(arg2)){
            std::cout << std::endl; 
            break;
        }
        sizeBuffer++;

        std::cout << " " << arg2 << std::endl;
    }

    std::cout << "Size of Buffer: " << sizeBuffer << std::endl;
    return;
}

void destroyDevice(ma_device& device, ma_decoder& decoder, TrackRingBuffer& audioBuffer){
    //make sure to do this after the buffer values have been parsed completely
    //the audio playback stops immediately when these lines are called
    ma_device_uninit(&device);
    ma_decoder_uninit(&decoder);
    delete audioBuffer.ringBuffer;
    return;
}
