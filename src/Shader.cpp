#include <glad/glad.h>
#include "Shader.h"
#include "ErrorHandler.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

Shader::Shader(const std::string& filepath)
    : m_Filepath(filepath), m_RendererID(0) 
{
    //generate shader by reading from filepaths
    ShaderProgramSource source = ParseShader(filepath);
    m_RendererID = CreateShader(source.VertexSource, source.FragmentSource);
}

Shader::~Shader(){
    GLCall(glDeleteProgram(m_RendererID));
}

struct ShaderProgramSource Shader::ParseShader(const std::string& filepath){
    std::ifstream stream(filepath);

    std::string line;
    std::stringstream ss[2];
    ShaderType type = NONE;
    while(getline(stream, line)){
        if(line.find("#shader") != std::string::npos){
            if(line.find("vertex") != std::string::npos){
                type = VERTEX;
            }else if(line.find("fragment") != std::string::npos){
                type = FRAGMENT;
            }
        }
        else if(type != NONE){
            ss[(int)type] << line << '\n';
        }
    }
    struct ShaderProgramSource returnVal = { ss[0].str() , ss[1].str() };
    return returnVal;
}

unsigned int Shader::CompileShader(unsigned int type, const std::string& source){
    GLCall(unsigned int id = glCreateShader(type));
    const char* src = source.c_str(); //equivalent to &source[0];
    if(src == nullptr) std::cout << "NULL POINTER" << std::endl;
    //set source code for shader, nullptr means that it assumes the string is null-terminated, 
    //go through the whole length of the string
    GLCall(glShaderSource(id, 1, &src, nullptr));
    GLCall(glCompileShader(id));
    
    int result;
    GLCall(glGetShaderiv(id, GL_COMPILE_STATUS, &result));
    //shader couldn't compile, return error message and return 0
    if(result == GL_FALSE){
        int length;
        GLCall(glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length));
        //allocate on the stack dynamically
        char* message = (char*) alloca(length * sizeof(char));
        GLCall(glGetShaderInfoLog(id, length, &length, message));
        std::cout << "Failed to compile " << 
            (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << "shader" << std::endl;
        std::cout << message << std::endl;
        GLCall(glDeleteShader(id));
        return 0;
    }
    return id;
}

int Shader::CreateShader(const std::string& vertexShader, const std::string& fragmentShader){
    GLCall(unsigned int program = glCreateProgram());
    unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
    unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);
    //attach shaders to program
    GLCall(glAttachShader(program, vs));
    GLCall(glAttachShader(program, fs));
    GLCall(glLinkProgram(program));
    GLCall(glValidateProgram(program));
    //remove intermediate shader objects
    GLCall(glDeleteShader(vs));
    GLCall(glDeleteShader(fs));
    //return program id
    return program;
}

void Shader::Bind() const{
    GLCall(glUseProgram(m_RendererID));
}

void Shader::Unbind() const{
    GLCall(glUseProgram(0));
}

void Shader::SetUniform1i(const std::string& name, int value){
    GLCall(glUniform1i(GetUniformLocation(name), value));
}

void Shader::SetUniform4f(const std::string& name, float v0, float v1, float v2, float v3){
    GLCall(glUniform4f(GetUniformLocation(name), v0, v1, v2, v3));
}

void Shader::SetUniformMat4f(const std::string& name, const glm::mat4& matrix){
    GLCall(glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &matrix[0][0]));
}

int Shader::GetUniformLocation(const std::string& name){
    if(m_UniformLocationCache.find(name) != m_UniformLocationCache.end()){
        return m_UniformLocationCache[name];
    }

    GLCall(int location = glGetUniformLocation(m_RendererID, name.c_str()));
    if(location == -1){
        std::cout << "Warning: uniform " << name << " doesn't exist!" << std::endl;
    }
    m_UniformLocationCache[name] = location;
    return location;
}