#include "MappedDrawObj.h"

MappedDrawObj::MappedDrawObj(float* positions, unsigned int* indices, size_t posLength, size_t indLength, VertexBufferLayout layout)
    : va(), vb(positions, posLength * sizeof(float)), ib(indices, indLength){

    va.AddBuffer(vb, layout);
    va.Unbind();
    vb.Bind();
    ib.Bind();
    GLCall(mappedPositions = (float*) glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE));
    GLCall(mappedIndices = (unsigned int*) glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_READ_WRITE));
    vb.Unbind();
    ib.Unbind();
}

MappedDrawObj::~MappedDrawObj(){

}