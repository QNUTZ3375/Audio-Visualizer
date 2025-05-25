#include "SampleLine.h"

#include <iostream>

SampleLine::SampleLine(unsigned int id, float basex, float basey, float heightLeft, float heightRight, float width, float r, float g, float b, float a){
    m_ID = id;

    m_Coords[0] = basex;
    m_Coords[1] = basey - heightRight;
    m_Coords[2] = 1.0f;
    m_Coords[3] = basex + width;
    m_Coords[4] = basey - heightRight;
    m_Coords[5] = 1.0f;
    m_Coords[6] = basex + width;
    m_Coords[7] = basey + heightLeft;
    m_Coords[8] = 1.0f;
    m_Coords[9] = basex;
    m_Coords[10] = basey + heightLeft;
    m_Coords[11] = 1.0f;

    m_Colors[0] = r;
    m_Colors[1] = g;
    m_Colors[2] = b;
    m_Colors[3] = a;

    m_Indices[0] = 0 + id*4;
    m_Indices[1] = 1 + id*4;
    m_Indices[2] = 2 + id*4;
    m_Indices[3] = 2 + id*4;
    m_Indices[4] = 3 + id*4;
    m_Indices[5] = 0 + id*4;
}

SampleLine::SampleLine(unsigned int id, float basex, float basey, float height, float width, float r, float g, float b, float a){
    m_ID = id;

    m_Coords[0] = basex;
    m_Coords[1] = basey;
    m_Coords[2] = 1.0f;
    m_Coords[3] = basex + width;
    m_Coords[4] = basey;
    m_Coords[5] = 1.0f;
    m_Coords[6] = basex + width;
    m_Coords[7] = basey + height;
    m_Coords[8] = 1.0f;
    m_Coords[9] = basex;
    m_Coords[10] = basey + height;
    m_Coords[11] = 1.0f;

    m_Colors[0] = r;
    m_Colors[1] = g;
    m_Colors[2] = b;
    m_Colors[3] = a;

    m_Indices[0] = 0 + id*4;
    m_Indices[1] = 1 + id*4;
    m_Indices[2] = 2 + id*4;
    m_Indices[3] = 2 + id*4;
    m_Indices[4] = 3 + id*4;
    m_Indices[5] = 0 + id*4;
}

SampleLine::~SampleLine(){

}

void SampleLine::changeColor(float r, float g, float b, float a){
    m_Colors[0] = r;
    m_Colors[1] = g;
    m_Colors[2] = b;
    m_Colors[3] = a;
}

const void SampleLine::fillVertices(float* target, int offset){
    const float vertices[28] = {
        m_Coords[0], m_Coords[1], m_Coords[2], m_Colors[0], m_Colors[1], m_Colors[2], m_Colors[3],
        m_Coords[3], m_Coords[4], m_Coords[5], m_Colors[0], m_Colors[1], m_Colors[2], m_Colors[3],
        m_Coords[6], m_Coords[7], m_Coords[8], m_Colors[0], m_Colors[1], m_Colors[2], m_Colors[3],
        m_Coords[9], m_Coords[10], m_Coords[11], m_Colors[0], m_Colors[1], m_Colors[2], m_Colors[3]
    };
    std::memcpy(target + offset, vertices, 28 * sizeof(float));
}

const void SampleLine::fillIndices(unsigned int* target, int offset){
    std::memcpy(target + offset, m_Indices, 6 * sizeof(unsigned int));
}

void SampleLine::changeIndicesPosition(unsigned int id){
    m_Indices[0] = 0 + id*4;
    m_Indices[1] = 1 + id*4;
    m_Indices[2] = 2 + id*4;
    m_Indices[3] = 2 + id*4;
    m_Indices[4] = 3 + id*4;
    m_Indices[5] = 0 + id*4;
}

void SampleLine::changeID(unsigned int id){
    m_ID = id;
}

void SampleLine::changeXPos(float newBaseX){
    const float width = getWidth();
    m_Coords[0] = newBaseX;
    m_Coords[3] = newBaseX + width;
    m_Coords[6] = newBaseX + width;
    m_Coords[9] = newBaseX;
}

void SampleLine::changeYPos(float newBaseY){
    const float height = getHeight();
    m_Coords[1] = newBaseY;
    m_Coords[4] = newBaseY;
    m_Coords[7] = newBaseY + height;
    m_Coords[10] = newBaseY + height;
}

void SampleLine::changeWidth(float newWidth){
    m_Coords[3] = m_Coords[0] + newWidth;
    m_Coords[6] = m_Coords[0] + newWidth;
}

void SampleLine::changeHeight(float newHeight){
    m_Coords[7] = m_Coords[1] + newHeight;
    m_Coords[10] = m_Coords[1] + newHeight;
}

void SampleLine::changeColors(float newR, float newG, float newB, float newA){
    m_Colors[0] = newR;
    m_Colors[0] = newG;
    m_Colors[0] = newB;
    m_Colors[0] = newA;
}