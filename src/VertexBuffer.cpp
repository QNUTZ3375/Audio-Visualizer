#include "VertexBuffer.h"
#include "Renderer.h"

VertexBuffer::VertexBuffer(const void* data, unsigned int size){
    GLCall(glGenBuffers(1, &m_RendererID));
    //bind to buffer (select buffer), current buffer is an array (GL_ARRAY_BUFFER)
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_RendererID));
    //set buffer data
    GLCall(glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW));
}

VertexBuffer::~VertexBuffer(){
    GLCall(glDeleteBuffers(1, &m_RendererID));
}

void VertexBuffer::Bind() const{
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_RendererID));
}

void VertexBuffer::Unbind() const{
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
}