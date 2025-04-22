#include "VertexArray.h"
#include "ErrorHandler.h"

VertexArray::VertexArray(){
    GLCall(glGenVertexArrays(1, &m_RendererID)); 
}

VertexArray::~VertexArray(){
    GLCall(glDeleteVertexArrays(1, &m_RendererID));
}

void VertexArray::AddBuffer(const VertexBuffer& vb, const VertexBufferLayout& layout){
    Bind();
    vb.Bind();
    const std::vector<VertexBufferElement>& elements = layout.GetElements();
    unsigned int offset = 0;
    for (unsigned int i = 0; i < elements.size(); i++){
        const VertexBufferElement& element = elements[i];
        //enables attribute at specified index
        GLCall(glEnableVertexAttribArray(i));

        //set the vertex attribute; tells OpenGL the layout of the data being passed in
        //in this case, index i, with count amount of type, element's normalizing, the size per 
        //vertex (stride), where to start reading within a vertex (offset)
        GLCall(glVertexAttribPointer(i, element.count, element.type, element.normalized, 
            layout.GetStride(), (const void*) (uintptr_t) offset));
        offset += element.count * VertexBufferElement::GetSizeOfType(element.type);
    }
}

void VertexArray::Bind() const{
    GLCall(glBindVertexArray(m_RendererID));
}

void VertexArray::Unbind() const{
    GLCall(glBindVertexArray(0));
}