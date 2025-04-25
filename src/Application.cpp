#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <cmath>
#include <deque>

#include "ErrorHandler.h"
#include "Renderer.h"
#include "Shader.h"
#include "ImgTexture.h"
#include "SampleLine.h"
#include "AudioPlayer.h"
#include "AuxComputations.h"
#include "MappedDrawObj.h"

#include "vendor/glm/glm.hpp"
#include "vendor/glm/gtc/matrix_transform.hpp"
#include "vendor/imgui/imgui.h"
#include "vendor/imgui/imgui_impl_glfw.h"
#include "vendor/imgui/imgui_impl_opengl3.h"

#define WINDOW_WIDTH 960.0f
#define WINDOW_HEIGHT 720.0f
//216 regular amplitude samples
#define NUM_GRAPH_SAMPLES 216
//3 values (xyz)
#define NUM_POSITION_POINTS 3
//2 triangles, 3 indices each
#define NUM_INDEX_POINTS (2*3)
//4 color components (rgba)
#define NUM_COLOR_POINTS 4
//4 position points, 4 color points, 1 SampleLine object
#define NUM_TOTAL_VERTEX_POINTS (4*NUM_POSITION_POINTS + 4*NUM_COLOR_POINTS)
#define SAMPLE_WIDTH 2
#define SAMPLE_MARGIN 1
#define MAX_AMPLITUDE_HEIGHT 200
#define AUDIO_SAMPLE_RATE 48000
#define DECIBEL_METER_MAX_LENGTH 400

void processInput(GLFWwindow *window){
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height){
    glViewport(0, 0, width, height);
}

void generateGraph(std::deque<SampleLine>& deque, float* positions, unsigned int* indices, double* funcTable, int& iterator){
    AuxComputations::RGBColor color = {0, 0, 0};
    for(iterator = 0; iterator < NUM_GRAPH_SAMPLES; iterator++){
        HSBtoRGB(2*iterator, 1, 1, color);
        deque.push_back(SampleLine(iterator, 45 + iterator*(SAMPLE_WIDTH + 2*SAMPLE_MARGIN), WINDOW_HEIGHT/2, 
                                        4 + MAX_AMPLITUDE_HEIGHT * funcTable[(5*iterator)%360], SAMPLE_WIDTH,
                                        color.r, color.g, color.b, 1.0f));
        deque.back().fillVertices(positions, NUM_TOTAL_VERTEX_POINTS*iterator);
        deque.back().fillIndices(indices, NUM_INDEX_POINTS*iterator);
    }
}

void rotateDeque(std::deque<SampleLine>& deque, int& iterator, double* trigTable, float* positions, unsigned int* indices, float leftSample, float rightSample){
    SampleLine toDelete = deque.at(0);
    //remove front element
    deque.pop_front();
    //update all indices and shift left
    for(int i = 0; i < deque.size(); i++){
        deque.at(i).changeID(deque.at(i).getID() - 1);
        deque.at(i).changeIndicesPosition(deque.at(i).getID());
        deque.at(i).changeXPos(-1*(SAMPLE_WIDTH + 2*SAMPLE_MARGIN));
    }

    //generate new SampleLine
    AuxComputations::RGBColor color = {0, 0, 0};
    iterator %= 360;
    AuxComputations::HSBtoRGB(2*iterator, 1, 1, color);
    deque.push_back(SampleLine(deque.size(), 45 + deque.size()*(SAMPLE_WIDTH + 2*SAMPLE_MARGIN), WINDOW_HEIGHT/2, 
                                2 + MAX_AMPLITUDE_HEIGHT * leftSample, 2 + MAX_AMPLITUDE_HEIGHT * rightSample, SAMPLE_WIDTH,
                                color.r, color.g, color.b, 1.0f));
    //increment iterator after creating sample
    iterator = (iterator + 1);

    //update position and indices array
    for(int i = 0; i < deque.size(); i++){
        deque.at(i).fillVertices(positions, NUM_TOTAL_VERTEX_POINTS*i);
        deque.at(i).fillIndices(indices, NUM_INDEX_POINTS*i);
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
    SampleLine separator(offset, 45, WINDOW_HEIGHT/2 - 1, 2, WINDOW_WIDTH - 98, 0.5f, 0.5f, 0.5f, 1.0f);

    separator.fillVertices(positions, NUM_TOTAL_VERTEX_POINTS*(offset));
    separator.fillIndices(indices, NUM_INDEX_POINTS*(offset));
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
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "New Window", nullptr, nullptr);
    if(!window){
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    
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
        std::deque<SampleLine> lineDeque;
        float positions[NUM_TOTAL_VERTEX_POINTS*(NUM_GRAPH_SAMPLES + 1)];
        unsigned int indices[NUM_INDEX_POINTS*(NUM_GRAPH_SAMPLES + 1)];
        int iterator = 0;
        generateGraph(lineDeque, positions, indices, funcTable, iterator);
        addSeparatorLine(NUM_GRAPH_SAMPLES, positions, indices);

        MappedDrawObj graphObj(positions, indices, 
            NUM_TOTAL_VERTEX_POINTS * (NUM_GRAPH_SAMPLES + 1), 
            NUM_INDEX_POINTS*(NUM_GRAPH_SAMPLES + 1), layout);
        //end setting up main graph

        //start setting up decibel meter
        float dbPositions[NUM_TOTAL_VERTEX_POINTS*6];
        unsigned int dbIndices[NUM_INDEX_POINTS*6];
        int dbIterator = 0;
        float dbXPos = 45;
        float dbYPos = 45;

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

        MappedDrawObj dbMeterObj(dbPositions, dbIndices, 
            NUM_TOTAL_VERTEX_POINTS*6, NUM_INDEX_POINTS*6, layout);
        //end setting up decibel meter

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

        ma_device device;
        ma_decoder decoder;
        //file_example_WAV_1MG
        //Moon_River_Audio_File
        const char* filepath = "file_example_WAV_1MG.wav";
        TrackRingBuffer audioBuffer;
        createDevice(device, decoder, filepath, audioBuffer);

        bool hasPlayed = false;
        //set it a tiny bit below the actual samples needed
        //to reduce frame drops
        int samplesPerDrawCall = AUDIO_SAMPLE_RATE / mode->refreshRate - 2;
        float prevLeftSample = 0;
        float prevRightSample = 0;
        float prevLeftDB = 1.0f;
        float prevRightDB = 1.0f;

        while(!glfwWindowShouldClose(window)){
            processInput(window);

            renderer.Clear();

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            {
                if(hasPlayed && !(*audioBuffer.ringBuffer).isEmpty()){
                    if((*audioBuffer.ringBuffer).getSize() >= 2*samplesPerDrawCall){
                        for(int i = 0; i < samplesPerDrawCall; i++){
                            float temp;
                            (*audioBuffer.ringBuffer).pop(temp);
                        }
                    }
                    float leftSample = prevLeftSample;
                    float rightSample = prevRightSample;
                    AuxComputations::computePeakValueStereo((*audioBuffer.ringBuffer), samplesPerDrawCall, leftSample, rightSample);
                    leftSample = AuxComputations::expSmooth(prevLeftSample, leftSample, 0.3f);
                    rightSample = AuxComputations::expSmooth(prevRightSample, rightSample, 0.3f);
                    rotateDeque(lineDeque, iterator, funcTable, 
                                graphObj.mappedPositions, graphObj.mappedIndices, 
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
                }
                if(!hasPlayed && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS){
                    startAudioCallback(device, decoder);
                    hasPlayed = true;
                }
                // model handles the objects view
                glm::mat4 model = glm::translate(glm::mat4(1.0f), translation);
                // all three are multiplied together (in reverse order due to column ordered matrices)
                glm::mat4 mvp = proj * view * model;
                shader.Bind();
                shader.SetUniformMat4f("u_MVP", mvp);
                renderer.Draw(graphObj.va, graphObj.ib, shader);
                renderer.Draw(dbMeterObj.va, dbMeterObj.ib, shader);
            }

            {
                ImGui::SliderFloat3("Translation", &translation.x, -WINDOW_WIDTH, WINDOW_WIDTH);
                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            }

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
            glfwSwapBuffers(window);
            glfwPollEvents();
        }
        // printRingBufferContents(*audioBuffer.ringBuffer);
        destroyDevice(device, decoder, audioBuffer);
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
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
*/