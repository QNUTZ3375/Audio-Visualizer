#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fftw3.h>

#include <iostream>
#include <cmath>
#include <deque>
#include <vector>
#include <numeric>

#include "ErrorHandler.h"
#include "Renderer.h"
#include "Shader.h"
#include "ImgTexture.h"
#include "SampleLine.h"
#include "AudioPlayer.h"
#include "AuxComputations.h"
#include "MappedDrawObj.h"
#include "ColorThemes.h"

#include "vendor/glm/glm.hpp"
#include "vendor/glm/gtc/matrix_transform.hpp"
#include "vendor/imgui/imgui.h"
#include "vendor/imgui/imgui_impl_glfw.h"
#include "vendor/imgui/imgui_impl_opengl3.h"
#include "vendor/portable-file-dialogs/portable-file-dialogs.h"
 
#define WINDOW_WIDTH 1080.0f
#define WINDOW_HEIGHT 720.0f
#define SAMPLE_WIDTH 2
#define SAMPLE_MARGIN 1
#define NUM_FFT_SAMPLES 4096
//256 regular amplitude samples
#define NUM_GRAPH_SAMPLES 256
#define WINDOW_MARGIN (((int)WINDOW_WIDTH - NUM_GRAPH_SAMPLES*(SAMPLE_WIDTH + 2*SAMPLE_MARGIN)) / 2)
//3 values (xyz)
#define NUM_POSITION_POINTS 3
//2 triangles, 3 indices each
#define NUM_INDEX_POINTS (2*3)
//4 color components (rgba)
#define NUM_COLOR_POINTS 4
//4 position points, 4 color points, 1 SampleLine object
#define NUM_TOTAL_VERTEX_POINTS (4*NUM_POSITION_POINTS + 4*NUM_COLOR_POINTS)
#define MAX_AMPLITUDE_HEIGHT 200
#define AUDIO_SAMPLE_RATE 48000
#define DECIBEL_METER_MAX_LENGTH 400
#define MIN_FREQ 20.0f
//Nyquist frequency; can only determine up to half the sample rate frequency
#define MAX_FREQ ((float) AUDIO_SAMPLE_RATE/2)
#define BIN_WIDTH_FREQ_RANGE (((float) AUDIO_SAMPLE_RATE) / NUM_FFT_SAMPLES)
#define NUM_HALVES 4
#define SCALING_FACTOR (360/(((float)NUM_GRAPH_SAMPLES/NUM_HALVES) * 2))
#define HERTZ_PARTITIONS_INIT {MIN_FREQ, 60, 200, 1000, 2000, MAX_FREQ} //divides into important frequency ranges
#define BIN_PARTITIONS_INIT {1, 48, 100, 72, 35} //Adds to NUM_GRAPH_SAMPLES (256)

typedef enum{
    INACTIVE = -1,
    PAUSED = 0,
    PLAYING = 1
} DeviceStatus;

static DeviceStatus audioDeviceStatus = INACTIVE;
static bool toggleFileSelector = false;
static bool resetGraphs = false;
static std::string filepath;
static std::string filename;
static std::map<ColorThemes::ThemeType, ColorThemes::Theme> themeTable;

void framebuffer_size_callback(GLFWwindow* window, int width, int height){
    glViewport(0, 0, width, height);
}

bool pickFile(){
    std::vector<std::string> fpath = pfd::open_file("Select a File", ".", 
    {"Audio Files", "*.wav *.mp3 *.flac *.ogg"}, /*multiselect=*/false).result();
    if(fpath.size() < 1) return false;
    audioDeviceStatus = INACTIVE;
    filepath = fpath[0];
    const char* tempfilename = std::strrchr(filepath.c_str(), '/');
    if (!tempfilename) tempfilename = std::strrchr(filepath.c_str(), '\\');
    if (tempfilename) tempfilename++; // Move past the slash
    else tempfilename = filepath.c_str(); // No slash found
    filename = tempfilename;
    return true;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
    if (action != GLFW_PRESS) return;

    if(key == GLFW_KEY_ESCAPE){
        glfwSetWindowShouldClose(window, true);
        return;
    }

    ma_device* device = static_cast<ma_device*>(glfwGetWindowUserPointer(window));
    if(audioDeviceStatus != INACTIVE && key == GLFW_KEY_SPACE){
        if (audioDeviceStatus == PAUSED){
            startAudioCallback(*device);
            audioDeviceStatus = PLAYING;
        }
        else{
            stopAudioCallback(*device);
            audioDeviceStatus = PAUSED;
        }
        toggleFileSelector = true;
        return;
    }

    #ifdef __APPLE__
        // macOS: Cmd+O
        bool cmdPressed = (mods & GLFW_MOD_SUPER) != 0;  // GLFW_MOD_SUPER is Cmd on macOS
        if (audioDeviceStatus != PLAYING && cmdPressed && key == GLFW_KEY_O) {
            if(pickFile() == true) resetGraphs = true;
        }
    #else
        // Windows/Linux: Ctrl+O
        bool ctrlPressed = (mods & GLFW_MOD_CONTROL) != 0;
        if (audioDeviceStatus != PLAYING && ctrlPressed && key == GLFW_KEY_O) {
            if(pickFile() == true) resetGraphs = true;
        }
    #endif
}

void generateCustomBins(float* freqTable){
    int currentFreq = 0;
    std::vector<float> hertzPartitions = HERTZ_PARTITIONS_INIT;
    std::vector<int> binPartitions = BIN_PARTITIONS_INIT;

    assert(hertzPartitions.size() == binPartitions.size() + 1 
        && "Hertz partitions must be of size bin partitions + 1");
    assert(std::accumulate(binPartitions.begin(), binPartitions.end(), 0) == NUM_GRAPH_SAMPLES 
        && "Bin partitions must accumulate to the number of graph samples");

    for(int part_c = 0; part_c < binPartitions.size(); part_c++){
        float hertzPerBin = (hertzPartitions[part_c+1] - hertzPartitions[part_c]) / binPartitions[part_c];
        for(int i = 0; i < binPartitions[part_c]; i++){
            freqTable[currentFreq] = hertzPartitions[part_c] + i * (hertzPerBin);
            currentFreq++;
        }
    }
    freqTable[NUM_GRAPH_SAMPLES] = MAX_FREQ;
}

void generateGraph(std::vector<SampleLine>& graphArr, float* positions, unsigned int* indices, double* funcTable, int& iterator){
    AuxComputations::RGBAColor color = {0, 0, 0, 1.0f};
    for(iterator = 0; iterator < NUM_GRAPH_SAMPLES; iterator++){
        HSBtoRGBA(iterator, 0.6, 0.7, color);
        graphArr.push_back(SampleLine(iterator, WINDOW_MARGIN + iterator*(SAMPLE_WIDTH + 2*SAMPLE_MARGIN), WINDOW_HEIGHT/2, 
                                        4 + MAX_AMPLITUDE_HEIGHT * funcTable[(int)(SCALING_FACTOR*iterator)%360], SAMPLE_WIDTH,
                                        color.r, color.g, color.b, 1.0f));
        graphArr.back().fillVertices(positions, NUM_TOTAL_VERTEX_POINTS*iterator);
        graphArr.back().fillIndices(indices, NUM_INDEX_POINTS*iterator);
    }
}

void generateFreqGraph(std::vector<SampleLine>& freqArr, float* positions, unsigned int* indices, double* funcTable, int& iterator){
    AuxComputations::RGBAColor color = {0, 0, 0, 1.0f};
    for(iterator = 0; iterator < NUM_GRAPH_SAMPLES; iterator++){
        HSBtoRGBA((SCALING_FACTOR/(NUM_HALVES/2))*iterator, 1, 1, color);
        freqArr.push_back(SampleLine(iterator, WINDOW_MARGIN + iterator*(SAMPLE_WIDTH + 2*SAMPLE_MARGIN), WINDOW_HEIGHT/2, 
                                        4 + MAX_AMPLITUDE_HEIGHT * funcTable[(int)(SCALING_FACTOR*iterator)%360], SAMPLE_WIDTH,
                                        color.r, color.g, color.b, 0.9f));
        freqArr.back().fillVertices(positions, NUM_TOTAL_VERTEX_POINTS*iterator);
        freqArr.back().fillIndices(indices, NUM_INDEX_POINTS*iterator);
    }
}

void shiftGraphLeft(std::vector<SampleLine>& graphArr, int& iterator, float* positions, unsigned int* indices, float leftSample, float rightSample){
    //update all graph bars and shift left
    for(int i = 0; i < NUM_GRAPH_SAMPLES - 1; i++){
        const float* newColors = graphArr[i+1].getColors();
        graphArr[i].changeColor(newColors[0], newColors[1], newColors[2], newColors[3]);
        const float newHeight = graphArr[i+1].getHeight();
        graphArr[i].changeHeight(newHeight);
        const float newBaseY = graphArr[i+1].getBaseY();
        graphArr[i].changeYPos(newBaseY);
    }

    //update last element
    AuxComputations::RGBAColor color = {0, 0, 0, 1.0f};
    iterator %= 360;
    AuxComputations::HSBtoRGBA(iterator, 0.6, 0.7, color);
    graphArr.back().changeColor(color.r, color.g, color.b, 1.0f);
    graphArr.back().changeYPos(WINDOW_HEIGHT/2 - (2 + MAX_AMPLITUDE_HEIGHT * rightSample));
    graphArr.back().changeHeight(2 + MAX_AMPLITUDE_HEIGHT * rightSample + 2 + MAX_AMPLITUDE_HEIGHT * leftSample);

    //increment iterator after creating sample
    iterator = (iterator + 1);
    //update position array
    for(int i = 0; i < NUM_GRAPH_SAMPLES; i++){
        graphArr.at(i).fillVertices(positions, NUM_TOTAL_VERTEX_POINTS*i);
    }
}

void updateFreqValues(std::vector<SampleLine>& freqGraphArr, int& iterator, float* positions, unsigned int* indices, fftw_complex* leftSamples, fftw_complex* rightSamples, float* freqSeparationTable){
    for(int visBin = 0; visBin < NUM_GRAPH_SAMPLES; visBin++){
        float startFreq = freqSeparationTable[visBin];
        float endFreq = freqSeparationTable[visBin+1];
        //compute starting and ending position of bins
        int startBin = (int)(startFreq / BIN_WIDTH_FREQ_RANGE);
        int endBin = (int)(endFreq / BIN_WIDTH_FREQ_RANGE);

        if (endBin <= startBin){
            endBin = startBin + 1;
        }

        if (endBin > NUM_FFT_SAMPLES / 2){
            endBin = NUM_FFT_SAMPLES / 2;
            startBin = endBin - 1;
        }

        float centerFreq = (startFreq + endFreq) / 2.0f;
        //0.5f = tight central emphasis, 1.0f = default, 2.0f = less weight focus
        float weightingFactor = 1.7f;
        float sigma = (endFreq - startFreq) / 2.0f * weightingFactor;

        float leftWeightedSum = 0.0f, rightWeightedSum = 0.0f;
        float weightTotal = 0.0f;

        //compute magnitudes of both left and right channels within the given frequency range
        float re, im;
        for (int i = startBin; i < endBin && i < (NUM_FFT_SAMPLES/2 + 1); i++) {
            float binFreq = i * BIN_WIDTH_FREQ_RANGE;
            float dist = binFreq - centerFreq;

            // Gaussian weighting, e^-0.5(((x-u)/s)^2), smaller sigma means stricter fall-off
            float weight = expf(-0.5f * (dist * dist) / (sigma * sigma + 1e-6f));
            re = leftSamples[i][0], im = leftSamples[i][1];
            float leftMag = sqrtf(fmaxf(0.0f, re * re + im * im));

            re = rightSamples[i][0], im = rightSamples[i][1];
            float rightMag = sqrtf(fmaxf(0.0f, re * re + im * im));

            leftWeightedSum += leftMag * weight;
            rightWeightedSum += rightMag * weight;
            weightTotal += weight;
        }
        int gain = 25;
        //compute overall height of left and right sample
        float leftHeight = (leftWeightedSum / (weightTotal + 1e-6f)) / NUM_FFT_SAMPLES * MAX_AMPLITUDE_HEIGHT * gain;
        float rightHeight = (rightWeightedSum / (weightTotal + 1e-6f)) / NUM_FFT_SAMPLES * MAX_AMPLITUDE_HEIGHT * gain;

        //set default value to 0 if value is invalid
        if(std::isnan(leftHeight) || std::isinf(leftHeight) || leftHeight < 0) leftHeight = 0;
        if(std::isnan(rightHeight) || std::isinf(rightHeight) || rightHeight < 0) rightHeight = 0;

        float prevRightHeight = WINDOW_HEIGHT/2 - freqGraphArr[visBin].getBaseY();
        float prevLeftHeight = freqGraphArr[visBin].getHeight() - prevRightHeight;
        //calculate smoothed values (exponential smoothing)
        float newLeftHeight = AuxComputations::expSmooth(prevLeftHeight, leftHeight, 0.85f);
        float newRightHeight = AuxComputations::expSmooth(abs(prevRightHeight), rightHeight, 0.85f);

        freqGraphArr[visBin].changeHeight(fmaxf(4.0f, newRightHeight + newLeftHeight));
        freqGraphArr[visBin].changeYPos(WINDOW_HEIGHT/2 - fmaxf(2.0f, newRightHeight));
    }
    //update position array
    for(int i = 0; i < freqGraphArr.size(); i++){
        freqGraphArr.at(i).fillVertices(positions, NUM_TOTAL_VERTEX_POINTS*i);
    }
}

void adjustDecibelMeters(float* positions, unsigned int* indices, SampleLine& leftDBMeter, SampleLine& rightDBMeter, float prevLeftDB, float prevRightDB, float& leftDB, float& rightDB){
    leftDB = AuxComputations::expSmooth(prevLeftDB, glm::clamp((leftDB + 60.0f) / 60.0f, 0.01f, 1.0f), 0.93f);
    leftDBMeter.changeColor(0.85f, 0.85f, 0.85f, 0.7f);
    if(leftDB >= 0.05f) leftDBMeter.changeColor(0.0f, 0.9f, 0.0f, 0.8f);
    if(leftDB >= 0.5f) leftDBMeter.changeColor(0.9f, 0.9f, 0.0f, 0.8f);
    if(leftDB >= 0.75f) leftDBMeter.changeColor(0.9f, 0.0f, 0.0f, 0.8f);
    if(leftDB >= 0.99f) leftDBMeter.changeColor(0.9f, 0.9f, 0.9f, 0.8f);
    leftDBMeter.changeWidth(leftDB*DECIBEL_METER_MAX_LENGTH);
    leftDBMeter.fillVertices(positions, NUM_TOTAL_VERTEX_POINTS*4);
    leftDBMeter.fillIndices(indices, NUM_INDEX_POINTS*4);

    rightDB = AuxComputations::expSmooth(prevRightDB, glm::clamp((rightDB + 60.0f) / 60.0f, 0.01f, 1.0f), 0.93f);
    rightDBMeter.changeColor(0.85f, 0.85f, 0.85f, 0.7f);
    if(rightDB >= 0.05f) rightDBMeter.changeColor(0.0f, 0.9f, 0.0f, 0.8f);
    if(rightDB >= 0.5f) rightDBMeter.changeColor(0.9f, 0.9f, 0.0f, 0.8f);
    if(rightDB >= 0.75f) rightDBMeter.changeColor(0.9f, 0.0f, 0.0f, 0.8f);
    if(rightDB >= 0.99f) rightDBMeter.changeColor(0.9f, 0.9f, 0.9f, 0.8f);
    rightDBMeter.changeWidth(rightDB*DECIBEL_METER_MAX_LENGTH);
    rightDBMeter.fillVertices(positions, NUM_TOTAL_VERTEX_POINTS*5);
    rightDBMeter.fillIndices(indices, NUM_INDEX_POINTS*5);
}

void addSeparatorLine(size_t offset, float* positions, unsigned int* indices){
    SampleLine separator(offset, WINDOW_MARGIN, WINDOW_HEIGHT/2 - 1, 2, 
        WINDOW_WIDTH - 2*WINDOW_MARGIN - SAMPLE_MARGIN, 0.5f, 0.5f, 0.5f, 1.0f);

    separator.fillVertices(positions, NUM_TOTAL_VERTEX_POINTS*(offset));
    separator.fillIndices(indices, NUM_INDEX_POINTS*(offset));
}

bool checkMousePos(GLFWwindow* window, SampleLine targetObj){
    if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_PRESS){
        return false;
    }
    double mouseX;
    double mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    mouseY = WINDOW_HEIGHT - mouseY;
    if(mouseX >= targetObj.getBaseX() && mouseX <= targetObj.getBaseX() + targetObj.getWidth()
        && mouseY >= targetObj.getBaseY() && mouseY <= targetObj.getBaseY() + targetObj.getHeight()){
            return true;
    }
    return false;
}

int main(){
    //intialize glfw and configure
    if(!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    //initialize window
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Audio Visualizer", nullptr, nullptr);
    if(!window){
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    
    const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

    //initialize glad
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    GLCall(glEnable(GL_BLEND));

    double sinTable[360] = {};
    for(int i = 0; i < 360; i++){
        sinTable[i] = sin(i * M_PI/180);
    }
    double cosTable[360] = {};
    for(int i = 0; i < 360; i++){
        cosTable[i] = cos(i * M_PI/180);
    }
    double tanTable[360] = {};
    for(int i = 0; i < 360; i++){
        tanTable[i] = tan(i * M_PI/180);
    }
    //IMPORTANT: sets the function for drawing heights
    double* funcTable = sinTable;
    {
        VertexBufferLayout layout;
        //vertex position coords
        layout.Push<float>(3);
        //color coords
        layout.Push<float>(4);

        //start setting up main graph
        std::vector<SampleLine> ampGraph;
        float ampPositions[NUM_TOTAL_VERTEX_POINTS*(NUM_GRAPH_SAMPLES + 1)];
        unsigned int ampIndices[NUM_INDEX_POINTS*(NUM_GRAPH_SAMPLES + 1)];
        int ampIterator = 0;
        generateGraph(ampGraph, ampPositions, ampIndices, funcTable, ampIterator);
        addSeparatorLine(NUM_GRAPH_SAMPLES, ampPositions, ampIndices);

        MappedDrawObj ampGraphObj(ampPositions, ampIndices, 
            NUM_TOTAL_VERTEX_POINTS * (NUM_GRAPH_SAMPLES + 1), 
            NUM_INDEX_POINTS*(NUM_GRAPH_SAMPLES + 1), layout);
        //end setting up main graph

        //start setting up decibel meter
        float dbPositions[NUM_TOTAL_VERTEX_POINTS*6];
        unsigned int dbIndices[NUM_INDEX_POINTS*6];
        int dbIterator = 0;
        float dbXPos = WINDOW_MARGIN;
        float dbYPos = WINDOW_MARGIN + 10.0f;

        SampleLine blackOutline(dbIterator, dbXPos - 5, dbYPos - 5, 
            70, DECIBEL_METER_MAX_LENGTH + 10 + 10, 
            0.0f, 0.0f, 0.0f, 1.0f);
        blackOutline.fillVertices(dbPositions, NUM_TOTAL_VERTEX_POINTS*dbIterator);
        blackOutline.fillIndices(dbIndices, NUM_INDEX_POINTS*dbIterator);
        dbIterator++;

        SampleLine greenSegment(dbIterator, dbXPos, dbYPos, 
            60, DECIBEL_METER_MAX_LENGTH/2 + 10, 
            0.0f, 0.7f, 0.0f, 1.0f);
        greenSegment.fillVertices(dbPositions, NUM_TOTAL_VERTEX_POINTS*dbIterator);
        greenSegment.fillIndices(dbIndices, NUM_INDEX_POINTS*dbIterator);
        dbIterator++;

        SampleLine yellowSegment(dbIterator, dbXPos + DECIBEL_METER_MAX_LENGTH/2, dbYPos, 
            60, DECIBEL_METER_MAX_LENGTH/4 + 10, 
            0.7f, 0.7f, 0.0f, 1.0f);
        yellowSegment.fillVertices(dbPositions, NUM_TOTAL_VERTEX_POINTS*dbIterator);
        yellowSegment.fillIndices(dbIndices, NUM_INDEX_POINTS*dbIterator);
        dbIterator++;

        SampleLine redSegment(dbIterator, dbXPos + ((DECIBEL_METER_MAX_LENGTH * 3) / 4), dbYPos, 
            60, DECIBEL_METER_MAX_LENGTH/4 + 10, 
            0.7f, 0.0f, 0.0f, 1.0f);
        redSegment.fillVertices(dbPositions, NUM_TOTAL_VERTEX_POINTS*dbIterator);
        redSegment.fillIndices(dbIndices, NUM_INDEX_POINTS*dbIterator);
        dbIterator++;

        SampleLine leftDecibelMeter(dbIterator, dbXPos + 5, dbYPos + 5 + 30, 
            20, DECIBEL_METER_MAX_LENGTH, 
            0.85f, 0.85f, 0.85f, 0.7f);
        leftDecibelMeter.fillVertices(dbPositions, NUM_TOTAL_VERTEX_POINTS*dbIterator);
        leftDecibelMeter.fillIndices(dbIndices, NUM_INDEX_POINTS*dbIterator);
        dbIterator++;

        SampleLine rightDecibelMeter(dbIterator, dbXPos + 5, dbYPos + 5, 
            20, DECIBEL_METER_MAX_LENGTH, 
            0.85f, 0.85f, 0.85f, 0.7f);
        rightDecibelMeter.fillVertices(dbPositions, NUM_TOTAL_VERTEX_POINTS*dbIterator);
        rightDecibelMeter.fillIndices(dbIndices, NUM_INDEX_POINTS*dbIterator);
        dbIterator++;

        MappedDrawObj dbMeterObj(dbPositions, dbIndices, 
            NUM_TOTAL_VERTEX_POINTS*6, NUM_INDEX_POINTS*6, layout);
        //end setting up decibel meter

        //start setting up frequency graph
        std::vector<SampleLine> freqGraph;
        float freqPositions[NUM_TOTAL_VERTEX_POINTS*(NUM_GRAPH_SAMPLES + 1)];
        unsigned int freqIndices[NUM_INDEX_POINTS*(NUM_GRAPH_SAMPLES + 1)];
        int freqIterator = 0;
        generateFreqGraph(freqGraph, freqPositions, freqIndices, funcTable, freqIterator);

        MappedDrawObj freqGraphObj(freqPositions, freqIndices, 
            NUM_TOTAL_VERTEX_POINTS * (NUM_GRAPH_SAMPLES + 1), 
            NUM_INDEX_POINTS*(NUM_GRAPH_SAMPLES + 1), layout);
        
        double* leftIn = (double*) fftw_malloc(NUM_FFT_SAMPLES * sizeof(double));
        fftw_complex* leftOut = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (NUM_FFT_SAMPLES/2 + 1));
        double* rightIn = (double*) fftw_malloc(NUM_FFT_SAMPLES * sizeof(double));
        fftw_complex* rightOut = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (NUM_FFT_SAMPLES/2 + 1));
        std::fill(leftIn, leftIn + NUM_FFT_SAMPLES, 0.0);
        std::fill(rightIn, rightIn + NUM_FFT_SAMPLES, 0.0);
        for(int i = 0; i < (NUM_FFT_SAMPLES/2 + 1); i++){
            leftOut[i][0] = 0;
            leftOut[i][1] = 0;
            rightOut[i][0] = 0;
            rightOut[i][1] = 0;
        }

        fftw_plan freqGraphPlanLeft = fftw_plan_dft_r2c_1d(NUM_FFT_SAMPLES, leftIn, leftOut, FFTW_ESTIMATE);
        fftw_plan freqGraphPlanRight = fftw_plan_dft_r2c_1d(NUM_FFT_SAMPLES, rightIn, rightOut, FFTW_ESTIMATE);

        float logFreqSpacingTable[NUM_GRAPH_SAMPLES+1] = {0};
        float linearFreqSpacingTable[NUM_GRAPH_SAMPLES+1] = {0};
        float customFreqSpacingTable[NUM_GRAPH_SAMPLES+1] = {0};

        float logMinFreq = logf(MIN_FREQ);
        float binSpacing = logf(MAX_FREQ / MIN_FREQ);
        //precompute logarithmic spacing for frequency graph
        for(int i = 0; i < NUM_GRAPH_SAMPLES+1; i++) {
            logFreqSpacingTable[i] = expf(logMinFreq + (float)i / NUM_GRAPH_SAMPLES * binSpacing);
        }
        for(int i = 0; i < NUM_GRAPH_SAMPLES+1; i++){
            linearFreqSpacingTable[i] = MIN_FREQ + (float)i/(NUM_GRAPH_SAMPLES+1) * (MAX_FREQ - MIN_FREQ);
        }
        generateCustomBins(customFreqSpacingTable);
        std::deque<float> rollingFreqBuffer(NUM_FFT_SAMPLES, 0.0f);
        //end setting up frequency graph

        //start setting up miscellaneous icons
        float icon_size = 60.0f;
        float icon_offset = 20.0f;
        float miscIconXpos = WINDOW_MARGIN + icon_offset;
        float miscIconYpos = WINDOW_HEIGHT - WINDOW_MARGIN - icon_offset - icon_size;
        //2 for pause icon, 0.5 for play icon, 2 for file icon
        float pauseButtonPositions[(int)(NUM_TOTAL_VERTEX_POINTS*2.0f)] = {
            miscIconXpos + icon_size*0.2f, miscIconYpos            , 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            miscIconXpos + icon_size*0.4f, miscIconYpos            , 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            miscIconXpos + icon_size*0.4f, miscIconYpos + icon_size, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            miscIconXpos + icon_size*0.2f, miscIconYpos + icon_size, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            miscIconXpos + icon_size*0.6f, miscIconYpos            , 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            miscIconXpos + icon_size*0.8f, miscIconYpos            , 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            miscIconXpos + icon_size*0.8f, miscIconYpos + icon_size, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            miscIconXpos + icon_size*0.6f, miscIconYpos + icon_size, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        };
        unsigned int pauseButtonIndices[(int)(NUM_INDEX_POINTS*2.0f)] = {
            0, 1, 2, 2, 3, 0,
            4, 5, 6, 6, 7, 4,
        };
        MappedDrawObj pauseButtonObj(pauseButtonPositions, pauseButtonIndices, 
            (size_t)(NUM_TOTAL_VERTEX_POINTS * 2.0f), 
            (size_t)(NUM_INDEX_POINTS * 2.0f), layout);

        float startButtonPositions[(int)(NUM_TOTAL_VERTEX_POINTS*0.75f)] = {
            miscIconXpos            , miscIconYpos                 , 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            miscIconXpos            , miscIconYpos + icon_size     , 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            miscIconXpos + icon_size, miscIconYpos + icon_size*0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        };
        unsigned int startButtonIndices[(int)(NUM_INDEX_POINTS*0.5f)] = {
            0, 1, 2,
        };
        MappedDrawObj startButtonObj(startButtonPositions, startButtonIndices, 
            (size_t)(NUM_TOTAL_VERTEX_POINTS * 2.0f), 
            (size_t)(NUM_INDEX_POINTS * 2.0f), layout);

        float fileOpenButtonXpos = WINDOW_WIDTH - WINDOW_MARGIN - icon_offset - icon_size;
        float fileOpenButtonYpos = WINDOW_HEIGHT - WINDOW_MARGIN - icon_size - icon_offset;
        float fileButtonPositions[(int)(NUM_TOTAL_VERTEX_POINTS*3.0f)] = {
            fileOpenButtonXpos                 , fileOpenButtonYpos + icon_size*0.1f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
            fileOpenButtonXpos                 , fileOpenButtonYpos + icon_size*0.65f, 1.0f, 0.48f, 0.48f, 0.5f, 1.0f,
            fileOpenButtonXpos + icon_size     , fileOpenButtonYpos + icon_size*0.65f, 1.0f, 0.48f, 0.48f, 0.5f, 1.0f,
            fileOpenButtonXpos + icon_size     , fileOpenButtonYpos + icon_size*0.1f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,

            fileOpenButtonXpos                 , fileOpenButtonYpos + icon_size*0.65f, 1.0f, 0.5f, 0.5f, 0.5f, 1.0f,
            fileOpenButtonXpos                 , fileOpenButtonYpos + icon_size*0.8f, 1.0f, 0.5f, 0.5f, 0.5f, 1.0f,
            fileOpenButtonXpos + icon_size     , fileOpenButtonYpos + icon_size*0.8f, 1.0f, 0.5f, 0.5f, 0.5f, 1.0f,
            fileOpenButtonXpos + icon_size     , fileOpenButtonYpos + icon_size*0.65f, 1.0f, 0.5f, 0.5f, 0.5f, 1.0f,

            fileOpenButtonXpos                 , fileOpenButtonYpos + icon_size*0.8f, 1.0f, 0.5f, 0.5f, 0.5f, 1.0f,
            fileOpenButtonXpos                 , fileOpenButtonYpos + icon_size*0.9f, 1.0f, 0.5f, 0.5f, 0.5f, 1.0f,
            fileOpenButtonXpos + icon_size*0.5f, fileOpenButtonYpos + icon_size*0.9f, 1.0f, 0.5f, 0.5f, 0.5f, 1.0f,
            fileOpenButtonXpos + icon_size*0.5f, fileOpenButtonYpos + icon_size*0.8f, 1.0f, 0.5f, 0.5f, 0.5f, 1.0f,
        };
        unsigned int fileButtonIndices[(int)(NUM_INDEX_POINTS*3.0f)] = {
            0, 1, 2, 2, 3, 0,
            4, 5, 6, 6, 7, 4,
            8, 9, 10, 10, 11, 8,
        };
        MappedDrawObj fileButtonObj(fileButtonPositions, fileButtonIndices, 
            (size_t)(NUM_TOTAL_VERTEX_POINTS * 3.0f), 
            (size_t)(NUM_INDEX_POINTS * 3.0f), layout);

        float border_icon_size = 100.0f;
        float border_margin = 5.0f;
        float borderXpos = WINDOW_MARGIN;
        float borderYpos = WINDOW_HEIGHT - WINDOW_MARGIN - border_icon_size;
        float fileBorderXpos = WINDOW_WIDTH - WINDOW_MARGIN - border_icon_size;
        float borderButtonPositions[(int)(NUM_TOTAL_VERTEX_POINTS*4.0f)];
        unsigned int borderButtonIndices[(int)(NUM_INDEX_POINTS*4.0f)];

        float borderIterator = 0;
        SampleLine border(borderIterator, borderXpos, borderYpos, 
            border_icon_size, border_icon_size, 0.0f, 0.0f, 0.0f, 1.0f);
        border.fillVertices(borderButtonPositions, NUM_TOTAL_VERTEX_POINTS*borderIterator);
        border.fillIndices(borderButtonIndices, NUM_INDEX_POINTS*borderIterator);
        borderIterator++;

        SampleLine borderFile(borderIterator, fileBorderXpos, borderYpos, 
            border_icon_size, border_icon_size, 0.0f, 0.0f, 0.0f, 1.0f);
        borderFile.fillVertices(borderButtonPositions, NUM_TOTAL_VERTEX_POINTS*borderIterator);
        borderFile.fillIndices(borderButtonIndices, NUM_INDEX_POINTS*borderIterator);
        borderIterator++;

        SampleLine borderInterior(borderIterator, borderXpos + border_margin, borderYpos + border_margin, 
            border_icon_size - 2*border_margin, border_icon_size - 2*border_margin, 0.88f, 0.76f, 0.64f, 1.0f);
        borderInterior.fillVertices(borderButtonPositions, NUM_TOTAL_VERTEX_POINTS*borderIterator);
        borderInterior.fillIndices(borderButtonIndices, NUM_INDEX_POINTS*borderIterator);
        borderIterator++;

        SampleLine borderFileInterior(borderIterator, fileBorderXpos + border_margin, borderYpos + border_margin, 
            border_icon_size - 2*border_margin, border_icon_size - 2*border_margin, 0.88f, 0.76f, 0.64f, 1.0f);
        borderFileInterior.fillVertices(borderButtonPositions, NUM_TOTAL_VERTEX_POINTS*borderIterator);
        borderFileInterior.fillIndices(borderButtonIndices, NUM_INDEX_POINTS*borderIterator);
        borderIterator++;

        MappedDrawObj borderButtonObj(borderButtonPositions, borderButtonIndices, 
            (size_t)(NUM_TOTAL_VERTEX_POINTS * 4.0f), 
            (size_t)(NUM_INDEX_POINTS * 4.0f), layout);
        //end setting up miscellaneous icons

        Shader shader("./res/shaders/shader.glsl");
        Renderer renderer;

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.Fonts->AddFontDefault();
        io.Fonts->Build();

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330");
        ImGui::StyleColorsDark();

        glm::mat4 proj = glm::ortho(0.0f, WINDOW_WIDTH, 0.0f, WINDOW_HEIGHT, -1.0f, 1.0f);
        glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
        glm::vec3 translation(0.0f, 0.0f, 0);

        std::string home_dir;
        #ifdef _WIN32
        home_dir = std::getenv("USERPROFILE");
        #else
        home_dir = std::getenv("HOME");
        #endif

        //file_example_WAV_1MG
        //Moon_River_Audio_File
        if(pickFile() == false){
            std::cout << "No File Selected" << std::endl;
            fftw_free(leftIn);
            fftw_free(leftOut);
            fftw_free(rightIn);
            fftw_free(rightOut);
            fftw_destroy_plan(freqGraphPlanLeft);
            fftw_destroy_plan(freqGraphPlanRight);
            return 0;
        }

        std::cout << filepath << std::endl;

        ma_device device;
        ma_decoder decoder;

        glfwSetWindowUserPointer(window, &device);

        TrackRingBuffer audioBuffer;
        createDevice(device, decoder, filepath.c_str(), audioBuffer);

        //number of samples to be read per frame = sampling rate / refresh rate
        int samplesPerDrawCall = AUDIO_SAMPLE_RATE / mode->refreshRate;
        float prevLeftSample = 0;
        float prevRightSample = 0;
        float prevLeftDB = 1.0f;
        float prevRightDB = 1.0f;

        audioDeviceStatus = PAUSED;
        bool cursorPressedBefore = false;
        bool canSelectFile = true;
        while(!glfwWindowShouldClose(window)){
            renderer.Clear();
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            ImGui::SetNextWindowPos(ImVec2(dbXPos + DECIBEL_METER_MAX_LENGTH + 50, 
                WINDOW_HEIGHT - dbYPos - 80), ImGuiCond_Once);
            ImGui::SetNextWindowSize(ImVec2(WINDOW_WIDTH/2 + 20, 100), ImGuiCond_Once);
            {
                if(audioDeviceStatus == PLAYING && !(*audioBuffer.ringBuffer).isEmpty()){
                    if((*audioBuffer.ringBuffer).getSize() >= 2*samplesPerDrawCall){
                        for(int i = 0; i < samplesPerDrawCall; i++){
                            float temp;
                            (*audioBuffer.ringBuffer).pop(temp);
                        }
                    }
                    float leftSample = prevLeftSample;
                    float rightSample = prevRightSample;
                    std::vector<float> arraySamples(samplesPerDrawCall, 0.0f);

                    AuxComputations::fillArrayWithSamples((*audioBuffer.ringBuffer), arraySamples, samplesPerDrawCall);
                    AuxComputations::computePeakValueStereo(arraySamples, samplesPerDrawCall, leftSample, rightSample);
                    leftSample = AuxComputations::expSmooth(prevLeftSample, leftSample, 0.3f);
                    rightSample = AuxComputations::expSmooth(prevRightSample, rightSample, 0.3f);
                    shiftGraphLeft(ampGraph, ampIterator, 
                                ampGraphObj.mappedPositions, ampGraphObj.mappedIndices, 
                                leftSample, rightSample);
                    prevLeftSample = leftSample;
                    prevRightSample = rightSample;

                    float leftDB = prevLeftDB;
                    float rightDB = prevRightDB;
                    AuxComputations::computeDecibelLevels(leftSample, rightSample, leftDB, rightDB);
                    adjustDecibelMeters(dbMeterObj.mappedPositions, dbMeterObj.mappedIndices, 
                        leftDecibelMeter, rightDecibelMeter, prevLeftDB, prevRightDB, leftDB, rightDB);
                    prevLeftDB = leftDB;
                    prevRightDB = rightDB;

                    for(int i = 0; i < arraySamples.size(); i++){
                        rollingFreqBuffer.push_back(arraySamples[i]);
                        //hop with size of samples per draw call;
                        //if buffer is full, compute FFT and remove samples
                        if(rollingFreqBuffer.size() >= samplesPerDrawCall){
                            //fill in input array and compute FFT
                            std::fill(leftIn, leftIn + NUM_FFT_SAMPLES, 0.0);
                            std::fill(rightIn, rightIn + NUM_FFT_SAMPLES, 0.0);
                            int idx = 0;
                            for(int i = 0; i < rollingFreqBuffer.size(); i+=2){
                                leftIn[idx] = (double) rollingFreqBuffer[i];
                                rightIn[idx] = (double) rollingFreqBuffer[i+1];
                                idx++;
                            }
                            fftw_execute(freqGraphPlanLeft);
                            fftw_execute(freqGraphPlanRight);
                            //update frequency bars
                            updateFreqValues(freqGraph, freqIterator, 
                                        freqGraphObj.mappedPositions, freqGraphObj.mappedIndices, 
                                        leftOut, rightOut, customFreqSpacingTable);
                            //clear output values after frequency change
                            for(int i = 0; i < (NUM_FFT_SAMPLES/2 + 1); i++){
                                leftOut[i][0] = 0;
                                leftOut[i][1] = 0;
                                rightOut[i][0] = 0;
                                rightOut[i][1] = 0;
                            }
                            //remove samples after fft computation
                            for(int j = 0; j < samplesPerDrawCall; j++){
                                rollingFreqBuffer.pop_front();
                            }
                        }
                    }
                }
                if(audioBuffer.trackEnded == true){
                    audioDeviceStatus = INACTIVE;
                    toggleFileSelector = true;
                }
                // model handles the objects view
                glm::mat4 model = glm::translate(glm::mat4(1.0f), translation);
                // all three are multiplied together (in reverse order due to column ordered matrices)
                glm::mat4 mvp = proj * view * model;
                shader.Bind();
                shader.SetUniformMat4f("u_MVP", mvp);
                renderer.Draw(ampGraphObj.va, ampGraphObj.ib, shader);
                renderer.Draw(dbMeterObj.va, dbMeterObj.ib, shader);
                renderer.Draw(freqGraphObj.va, freqGraphObj.ib, shader);
                renderer.Draw(borderButtonObj.va, borderButtonObj.ib, shader);
                if(audioDeviceStatus == PLAYING){
                    renderer.Draw(pauseButtonObj.va, pauseButtonObj.ib, shader);
                }
                if(audioDeviceStatus <= PAUSED){
                    renderer.Draw(startButtonObj.va, startButtonObj.ib, shader);
                }
                renderer.Draw(fileButtonObj.va, fileButtonObj.ib, shader);
                if(!cursorPressedBefore && canSelectFile && checkMousePos(window, borderFile) == true){
                    if(pickFile() == true) resetGraphs = true;
                    cursorPressedBefore = true;
                }
                if(!cursorPressedBefore && checkMousePos(window, border) == true){
                    key_callback(window, GLFW_KEY_SPACE, 0, GLFW_PRESS, 0);
                    cursorPressedBefore = true;
                }
                if(cursorPressedBefore && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE){
                    cursorPressedBefore = false;
                }
                if(toggleFileSelector == true){
                    if(audioDeviceStatus == PLAYING){
                        canSelectFile = false;
                        borderFileInterior.changeColor(0.8f, 0.8f, 0.8f, 1.0f);
                    }
                    else{
                        canSelectFile = true;
                        borderFileInterior.changeColor(0.88f, 0.76f, 0.64f, 1.0f);
                    }
                    borderFileInterior.fillVertices(borderButtonObj.mappedPositions, 
                        NUM_TOTAL_VERTEX_POINTS*(borderIterator - 1));
                    toggleFileSelector = false;
                }
                if(resetGraphs == true){
                    //reset all graphs
                    ampGraph.clear();
                    ampIterator = 0;
                    freqGraph.clear();
                    freqIterator = 0;
                    generateGraph(ampGraph, ampGraphObj.mappedPositions, 
                        ampGraphObj.mappedIndices, funcTable, ampIterator);
                    generateFreqGraph(freqGraph, freqGraphObj.mappedPositions, 
                        freqGraphObj.mappedIndices, funcTable, freqIterator);
                    //reset decibel meters
                    leftDecibelMeter.changeWidth(DECIBEL_METER_MAX_LENGTH);
                    leftDecibelMeter.changeColor(0.85f, 0.85f, 0.85f, 0.7f);
                    leftDecibelMeter.fillVertices(dbMeterObj.mappedPositions, 
                        NUM_TOTAL_VERTEX_POINTS*(dbIterator - 2));
                    rightDecibelMeter.changeWidth(DECIBEL_METER_MAX_LENGTH);
                    rightDecibelMeter.changeColor(0.85f, 0.85f, 0.85f, 0.7f);
                    rightDecibelMeter.fillVertices(dbMeterObj.mappedPositions, 
                        NUM_TOTAL_VERTEX_POINTS*(dbIterator - 1));
                    //destroy current device (old filepath)
                    destroyDevice(device, decoder, audioBuffer);
                    //create new device with new filepath
                    createDevice(device, decoder, filepath.c_str(), audioBuffer);
                    audioBuffer.trackEnded = false;
                    audioDeviceStatus = PAUSED;
                    resetGraphs = false;
                }
            }

            {
                ImGui::Begin("Information");
                ImGui::TextWrapped("Now Playing: %s", filename.c_str());

                // ImGui::SliderFloat3("Translation", &translation.x, -WINDOW_WIDTH, WINDOW_WIDTH);
                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 
                    1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

                ImGui::End();
            }
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
            glfwSwapBuffers(window);
            glfwPollEvents();
        }
        // printRingBufferContents(*audioBuffer.ringBuffer);
        destroyDevice(device, decoder, audioBuffer);
        fftw_free(leftIn);
        fftw_free(leftOut);
        fftw_free(rightIn);
        fftw_free(rightOut);
        fftw_destroy_plan(freqGraphPlanLeft);
        fftw_destroy_plan(freqGraphPlanRight);
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}


/*
    1. pick file
    2. parse into raw data
    3. vizualise using opengl
*/

/*
Notes:
Monday 21 April 2025: 
    - Started this project (imported files from a previous project)
    - Got a sin wave of rectangles projected onto the screen
Tuesday 22 April 2025: 
    - Added variable colors to the rectangles
    - Added the ability to shift graph to the left
    - Added customisability to the graph's features
Wednesday 23 April 2025:
    - Did some research on the miniaudio library
    - Added AudioPlayer and RingBuffer files
    - Added a memory leak checker into the makefile
Thursday 24 April 2025:
    - Did some research on how audio samples are converted into visuals
    - Integrated AudioPlayer into the Application
    - Implemented RMS and Peak Graph functions
    - Modified the graph so now both the left and right track are displayed (left on top, right on bottom)
Friday 25 April 2025:
    - Implement dBFS decibel meter graphic and function (uses RMS) including exponential smoothing
    - Adjusted the horizontal separator of the graph
    - Abstracted object creation process, removed unnecessary includes
    - Change colour of decibel meter depending on level
    (-60db to -20db: green, -20db to -6db: yellow, -6db to 0db: red)
Sunday 27 April 2025:
    - Add file picker option

(Assignment Break)

Friday 23 May 2025:
    - changed AuxComputations to accept an array of values instead of popping from the queue
Saturday 24 May 2025:
    - added frequency graph and FFT computations
Sunday 25 May 2025:
    - tweaked FFT settings to reduce errors
    - added weighting heuristic
    - added customizable bin separation (called distance-weighted binning)
    - increased FFT bin size to create smoother visuals
Monday 26 May 2025:
    - Added customizability on graph appearance using macros
    - Added customizability on bin spacing
    - Added hopping to increase smoothness of frequency graph
Tuesday 27 May 2025:
    - Added the option to stop and resume a currently playing track
    - Added play and pause icons
Wednesday 28 May 2025:
    - Prevented file selection while a track is playing
    - Added the option to pick a new file after playback finished
Thursday 29 May 2025:
    - Added a text box to display the filepath of the track currently being played
*/