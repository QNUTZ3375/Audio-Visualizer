#pragma once

#include <vector>
#include <glad/glad.h>
#include "ErrorHandler.h"
#include <stdexcept>

struct VertexBufferElement{
    unsigned int type;
    unsigned int count;
    unsigned char normalized;

    static unsigned int GetSizeOfType(unsigned int type){
        switch(type){
            case GL_FLOAT: return sizeof(GLfloat);
            case GL_UNSIGNED_INT: return sizeof(GLuint);
            case GL_UNSIGNED_BYTE: return sizeof(GLbyte);
        }
        ASSERT(false);
        return 0;
    }
};

class VertexBufferLayout{
    private:
        std::vector<VertexBufferElement> m_Elements;
        unsigned int m_Stride;
    public:
        VertexBufferLayout()
            : m_Stride(0) {}

        //pushes "count" items into VBL (can be vertex points, texture coords, normalizations, etc)
        template<typename T>
        void Push(unsigned int count){
            std::runtime_error("VBL Unsupported Type Pushed Error");
        }

        template<>
        void Push<float>(unsigned int count){
            VertexBufferElement vbe = {GL_FLOAT, count, GL_FALSE};
            m_Elements.push_back(vbe);
            m_Stride += count * VertexBufferElement::GetSizeOfType(GL_FLOAT);
        }

        template<>
        void Push<unsigned int>(unsigned int count){
            VertexBufferElement vbe = {GL_UNSIGNED_INT, count, GL_FALSE};
            m_Elements.push_back(vbe);
            m_Stride += count * VertexBufferElement::GetSizeOfType(GL_UNSIGNED_INT);
        }

        template<>
        void Push<unsigned char>(unsigned int count){
            VertexBufferElement vbe = {GL_UNSIGNED_BYTE, count, GL_TRUE};
            m_Elements.push_back(vbe);
            m_Stride += count * VertexBufferElement::GetSizeOfType(GL_UNSIGNED_BYTE);
        }

        inline const std::vector<VertexBufferElement> GetElements() const { return m_Elements; }
        inline unsigned int GetStride() const { return m_Stride; }
};