#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cmath>
#include <deque>

#include "ErrorHandler.h"
#include "Renderer.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "VertexArray.h"
#include "Shader.h"
#include "ImgTexture.h"
#include "SampleLine.h"

#include "vendor/glm/glm.hpp"
#include "vendor/glm/gtc/matrix_transform.hpp"
#include "vendor/imgui/imgui.h"
#include "vendor/imgui/imgui_impl_glfw.h"
#include "vendor/imgui/imgui_impl_opengl3.h"
#include "AudioPlayer.h"

#define WINDOW_WIDTH 960.0f
#define WINDOW_HEIGHT 720.0f
//216 regular amplitude samples, 1 line separator
#define NUM_GRAPH_SAMPLES 216
#define TOTAL_SAMPLES (NUM_GRAPH_SAMPLES + 1)
//4 points, 3 values each (xyz)
#define NUM_VERTEX_POINTS (4*3)
//2 triangles, 3 indices each
#define NUM_INDEX_POINTS (2*3)
//4 color components (rgba)
#define NUM_COLOR_POINTS 4
#define SAMPLE_WIDTH 2
#define SAMPLE_MARGIN 1
#define MAX_AMPLITUDE_HEIGHT 200
#define AUDIO_SAMPLE_RATE 48000
#define DECIBEL_METER_MAX_LENGTH 400

void processInput(GLFWwindow *window);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

struct RGBColor{
    float r;
    float g;
    float b;
};

void HSBtoRGB(int h, float s, float b, struct RGBColor& target){
    h = h % 360;
    if(s < 0) s = 0;
    if(s > 1) s = 1;
    if(b < 0) b = 0;
    if(b > 1) b = 1;

    float c = b * s;
    float x = c * (1 - abs((fmod(h/60.0f, 2.0f) - 1)));
    float m = b - c;
    struct RGBColor primes = {0, 0, 0};
    if(h >= 0 && h < 60) primes = {c, x, 0};
    if(h >= 60 && h < 120) primes = {x, c, 0};
    if(h >= 120 && h < 180) primes = {0, c, x}; 
    if(h >= 180 && h < 240) primes = {0, x, c}; 
    if(h >= 240 && h < 300) primes = {x, 0, c}; 
    if(h >= 300 && h < 360) primes = {c, 0, x};

    target = {primes.r + m, primes.g + m, primes.b + m };
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
    struct RGBColor color = {0, 0, 0};
    iterator %= 360;
    HSBtoRGB(2*iterator, 1, 1, color);
    deque.push_back(SampleLine(deque.size(), 
                                45 + deque.size()*(SAMPLE_WIDTH + 2*SAMPLE_MARGIN), 
                                WINDOW_HEIGHT/2, 
                                2 + MAX_AMPLITUDE_HEIGHT * leftSample,
                                2 + MAX_AMPLITUDE_HEIGHT * rightSample,
                                SAMPLE_WIDTH,
                                color.r, color.g, color.b));
    //increment iterator after creating sample
    iterator = (iterator + 1);

    //update position and indices array
    for(int i = 0; i < deque.size(); i++){
        deque.at(i).fillVertices(positions, (NUM_VERTEX_POINTS + 4*NUM_COLOR_POINTS)*i);
        deque.at(i).fillIndices(indices, NUM_INDEX_POINTS*i);
    }
}

void adjustDecibelMeters(float* positions, unsigned int* indices, SampleLine& leftDBMeter, SampleLine& rightDBMeter, float prevLeftDB, float prevRightDB, float& leftDB, float& rightDB){

    float leftNormalizedDB = glm::clamp((leftDB + 60.0f) / 60.0f, 0.05f, 1.0f);
    float rightNormalizedDB = glm::clamp((rightDB + 60.0f) / 60.0f, 0.05f, 1.0f);
    
    float smFactor = 0.9f;
    float leftSmoothed = smFactor * prevLeftDB + (1 - smFactor) * leftNormalizedDB;
    float rightSmoothed = smFactor * prevRightDB + (1 - smFactor) * rightNormalizedDB;

    leftDB = leftSmoothed;
    rightDB = rightSmoothed;

    leftDBMeter.changeWidth(leftSmoothed*DECIBEL_METER_MAX_LENGTH);
    rightDBMeter.changeWidth(rightSmoothed*DECIBEL_METER_MAX_LENGTH);

    leftDBMeter.fillVertices(positions, 0);
    leftDBMeter.fillIndices(indices, 0);

    rightDBMeter.fillVertices(positions, (NUM_VERTEX_POINTS + 4*NUM_COLOR_POINTS));
    rightDBMeter.fillIndices(indices, NUM_INDEX_POINTS);
}

void addSeparatorLine(size_t offset, float* positions, unsigned int* indices){
    SampleLine separator(offset, 45, WINDOW_HEIGHT/2 - 1, 2, WINDOW_WIDTH - 90, 0.5f, 0.5f, 0.5f);

    separator.fillVertices(positions, (NUM_VERTEX_POINTS + 4*NUM_COLOR_POINTS)*(offset));
    separator.fillIndices(indices, NUM_INDEX_POINTS*(offset));
}

void computeRMSValueStereo(RingBuffer<float>& ringBuffer, size_t amtOfSamples, float& leftVal, float& rightVal){
    float sumSquaresLeft = 0.0f;
    float sumSquaresRight = 0.0f;

    for(int i = 0; i < amtOfSamples; i+=2){
        float currSampleLeft = 0.0f;
        if(!ringBuffer.pop(currSampleLeft)) break;
        sumSquaresLeft += currSampleLeft * currSampleLeft;

        float currSampleRight = 0.0f;
        if(!ringBuffer.pop(currSampleRight)) break;
        sumSquaresRight += currSampleRight * currSampleRight;
    }
    leftVal = sqrt(sumSquaresLeft / (float) amtOfSamples);
    rightVal = sqrt(sumSquaresRight / (float) amtOfSamples);
}

void computePeakValueStereo(RingBuffer<float>& ringBuffer, size_t amtOfSamples, float& leftVal, float& rightVal){
    float maxLeft = 0.0f;
    float maxRight = 0.0f;

    for(int i = 0; i < amtOfSamples; i+=2){
        float currSampleLeft = 0.0f;
        if(!ringBuffer.pop(currSampleLeft)) break;
        if(maxLeft < currSampleLeft) maxLeft = currSampleLeft;

        float currSampleRight = 0.0f;
        if(!ringBuffer.pop(currSampleRight)) break;
        if(maxRight < currSampleRight) maxRight = currSampleRight;
    }
    leftVal = maxLeft;
    rightVal = maxRight;
}

void computeDecibelLevels(float leftRMS, float rightRMS, float& leftDB, float& rightDB){
    leftDB = 20.0f * log10f(leftRMS + 1e-10f);
    rightDB = 20.0f * log10f(rightRMS + 1e-10f);
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
        std::deque<SampleLine> lineDeque;
        VertexBufferLayout layout;
        //vertex position coords
        layout.Push<float>(3);
        //color coords
        layout.Push<float>(4);

        float positions[(NUM_VERTEX_POINTS + 4*NUM_COLOR_POINTS)*TOTAL_SAMPLES];
        unsigned int indices[NUM_INDEX_POINTS*TOTAL_SAMPLES];

        struct RGBColor color = {0, 0, 0};
        int iterator = 0;

        for(iterator = 0; iterator < NUM_GRAPH_SAMPLES; iterator++){
            HSBtoRGB(2*iterator, 1, 1, color);
            lineDeque.push_back(SampleLine(iterator, 
                                            45 + iterator*(SAMPLE_WIDTH + 2*SAMPLE_MARGIN), 
                                            WINDOW_HEIGHT/2, 
                                            4 + MAX_AMPLITUDE_HEIGHT * funcTable[(5*iterator)%360], 
                                            SAMPLE_WIDTH,
                                            color.r, color.g, color.b));
            lineDeque.back().fillVertices(positions, (NUM_VERTEX_POINTS + 4*NUM_COLOR_POINTS)*iterator);
            lineDeque.back().fillIndices(indices, NUM_INDEX_POINTS*iterator);
        }

        addSeparatorLine(NUM_GRAPH_SAMPLES, positions, indices);

        GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
        GLCall(glEnable(GL_BLEND));

        VertexArray va;
        VertexBuffer vb(positions, (NUM_VERTEX_POINTS + 4*NUM_COLOR_POINTS) * TOTAL_SAMPLES * sizeof(float));
        va.AddBuffer(vb, layout);
        IndexBuffer ib(indices, NUM_INDEX_POINTS*TOTAL_SAMPLES);

        glm::mat4 proj = glm::ortho(0.0f, WINDOW_WIDTH, 0.0f, WINDOW_HEIGHT, -1.0f, 1.0f);
        glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

        Shader shader("./res/shaders/shader.glsl");
        shader.Bind();
    
        va.Unbind();
        shader.Unbind();
        vb.Unbind();
        ib.Unbind();

        Renderer renderer;

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.Fonts->AddFontDefault();
        io.Fonts->Build();

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330");
        ImGui::StyleColorsDark();

        glm::vec3 translation(0.0f, 0.0f, 0);

        vb.Bind();
        ib.Bind();
        GLCall(float* pos = (float*) glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE));
        GLCall(unsigned int* ind = (unsigned int*) glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_READ_WRITE));
        vb.Unbind();
        ib.Unbind();

        //start setting up decibel bar chart
        float dbPositions[(NUM_VERTEX_POINTS + 4*NUM_COLOR_POINTS)*2];
        unsigned int dbIndices[NUM_INDEX_POINTS*2];

        SampleLine leftDecibelMeter(0, 50, 100, 20, DECIBEL_METER_MAX_LENGTH, 0.0f, 0.2f, 1.0f);
        leftDecibelMeter.fillVertices(dbPositions, 0);
        leftDecibelMeter.fillIndices(dbIndices, 0);

        SampleLine rightDecibelMeter(1, 50, 70, 20, DECIBEL_METER_MAX_LENGTH, 0.0f, 0.2f, 1.0f);
        rightDecibelMeter.fillVertices(dbPositions, (NUM_VERTEX_POINTS + 4*NUM_COLOR_POINTS));
        rightDecibelMeter.fillIndices(dbIndices, NUM_INDEX_POINTS);

        VertexArray vaDecibel;
        VertexBuffer vbDecibel(dbPositions, (NUM_VERTEX_POINTS + 4*NUM_COLOR_POINTS)*2*sizeof(float));
        vaDecibel.AddBuffer(vbDecibel, layout);
        IndexBuffer ibDecibel(dbIndices, NUM_INDEX_POINTS*2);

        vaDecibel.Unbind();
        vbDecibel.Bind();
        ibDecibel.Bind();
        GLCall(float* posDB = (float*) glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE));
        GLCall(unsigned int* indDB = (unsigned int*) glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_READ_WRITE));
        vbDecibel.Unbind();
        ibDecibel.Unbind();

        //end setting up decibel bar chart

        ma_device device;
        ma_decoder decoder;
        //file_example_WAV_1MG
        //Moon_River_Audio_File
        const char* filepath = "Moon_River_Audio_File.wav";
        TrackRingBuffer audioBuffer;
        createDevice(device, decoder, filepath, audioBuffer);

        bool hasPlayed = false;
        int samplesPerDrawCall = AUDIO_SAMPLE_RATE / mode->refreshRate;
        float prevLeftSample = 0;
        float prevRightSample = 0;
        float prevLeftDB = 0;
        float prevRightDB = 0;

        while(!glfwWindowShouldClose(window)){
            processInput(window);

            renderer.Clear();

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            {
                if(hasPlayed && !(*audioBuffer.ringBuffer).isEmpty()){
                    float leftSample = prevLeftSample;
                    float rightSample = prevRightSample;
                    computePeakValueStereo((*audioBuffer.ringBuffer), samplesPerDrawCall, leftSample, rightSample);
                    rotateDeque(lineDeque, iterator, funcTable, pos, ind, leftSample, rightSample);
                    prevLeftSample = leftSample;
                    prevRightSample = rightSample;

                    float leftDB = prevLeftDB;
                    float rightDB = prevRightDB;
                    computeDecibelLevels(leftSample, rightSample, leftDB, rightDB);
                    adjustDecibelMeters(posDB, indDB, leftDecibelMeter, rightDecibelMeter, prevLeftDB, prevRightDB, leftDB, rightDB);
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
                renderer.Draw(va, ib, shader);
                renderer.Draw(vaDecibel, ibDecibel, shader);
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

void processInput(GLFWwindow *window){
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height){
    glViewport(0, 0, width, height);
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
*/