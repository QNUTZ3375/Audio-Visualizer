#pragma once

#include <string>
#include <unordered_map>
#include "vendor/glm/glm.hpp"

struct ShaderProgramSource{
    std::string VertexSource;
    std::string FragmentSource;
};

enum ShaderType{
    NONE = -1, VERTEX = 0, FRAGMENT = 1
};

class Shader{
    private:
        std::string m_Filepath;
        unsigned int m_RendererID;
        //caching for uniforms
        std::unordered_map<std::string, int> m_UniformLocationCache;
    public:
        Shader(const std::string& filepath);
        ~Shader();

        void Bind() const;
        void Unbind() const;

        //set uniforms
        void SetUniform1i(const std::string& name, int value);
        void SetUniform4f(const std::string& name, float v0, float v1, float v2, float v3);
        void SetUniformMat4f(const std::string& name, const glm::mat4& matrix);
    private:
        int GetUniformLocation(const std::string& name);
        unsigned int CompileShader(unsigned int type, const std::string& source);
        int CreateShader(const std::string& vertexShader, const std::string& fragmentShader);
        struct ShaderProgramSource ParseShader(const std::string& filepath);
};