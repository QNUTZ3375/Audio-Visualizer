#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cmath>

#include "ErrorHandler.h"
#include "Renderer.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "VertexArray.h"
#include "Shader.h"
#include "ImgTexture.h"
#include "Line.h"

#include "vendor/glm/glm.hpp"
#include "vendor/glm/gtc/matrix_transform.hpp"
#include "vendor/imgui/imgui.h"
#include "vendor/imgui/imgui_impl_glfw.h"
#include "vendor/imgui/imgui_impl_opengl3.h"

#define WINDOW_WIDTH 960.0f
#define WINDOW_HEIGHT 540.0f
#define NUM_RECTS 220
//4 points, 3 values each (xyz)
#define NUM_VERTEX_POINTS (4*3)
//2 triangles, 3 indices each
#define NUM_INDEX_POINTS (2*3)
//4 color components (rgba)
#define NUM_COLOR_POINTS 4

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
    //initialize window
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "New Window", nullptr, nullptr);
    if(!window){
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSwapInterval(1);

    //initialize glad
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    {
        Line lines[NUM_RECTS];
        float positions[(NUM_VERTEX_POINTS + 4*NUM_COLOR_POINTS)*NUM_RECTS];
        VertexBufferLayout layout;
        unsigned int indices[NUM_INDEX_POINTS*NUM_RECTS];

        struct RGBColor color = {0, 0, 0};

        for(int i = 0; i < NUM_RECTS; i++){
            HSBtoRGB(i, 1, 1, color);
            lines[i] = Line(i, 45 + i*4, WINDOW_HEIGHT/2, 2 + 100 * sin(i * 180/(500*M_PI)), 2,
                            color.r, color.g, color.b);
            lines[i].fillVertices(positions, (NUM_VERTEX_POINTS + 4*NUM_COLOR_POINTS)*i);
            lines[i].fillIndices(indices, NUM_INDEX_POINTS*i);
        }

        GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
        GLCall(glEnable(GL_BLEND));
    
        VertexArray va;
        VertexBuffer vb(positions, (NUM_VERTEX_POINTS + 4*NUM_COLOR_POINTS) * NUM_RECTS * sizeof(float));
        //vertex position coords
        layout.Push<float>(3);
        //color coords
        layout.Push<float>(4);
        va.AddBuffer(vb, layout);
    
        IndexBuffer ib(indices, NUM_INDEX_POINTS*NUM_RECTS);
        glm::mat4 proj = glm::ortho(0.0f, WINDOW_WIDTH, 0.0f, WINDOW_HEIGHT, -1.0f, 1.0f);
        glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

        Shader shader("./res/shaders/shader.shader");
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

        while(!glfwWindowShouldClose(window)){
            renderer.Clear();

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            {
                // model handles the objects view
                glm::mat4 model = glm::translate(glm::mat4(1.0f), translation);
                // all three are multiplied together (in reverse order due to column ordered matrices)
                glm::mat4 mvp = proj * view * model;
                shader.Bind();
                shader.SetUniformMat4f("u_MVP", mvp);
                renderer.Draw(va, ib, shader);
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

//change implementation to from storing an array of Lines to a queue of Rects

/*
Notes:
Monday 21 April 2025: Started this project, got a sin wave of rectangles implemented
Tuesday 22 April 2025: Added colors to the rectangles

*/