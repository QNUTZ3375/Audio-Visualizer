#pragma once

#include "ErrorHandler.h"
#include "VertexArray.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "VertexBufferLayout.h"

class MappedDrawObj{
    public:
        VertexArray va;
        VertexBuffer vb;
        IndexBuffer ib;
        float* mappedPositions;
        unsigned int* mappedIndices;
    public:
        MappedDrawObj(float* positions, unsigned int* indices, size_t posLength, size_t indLength, VertexBufferLayout layout);
        ~MappedDrawObj();
};