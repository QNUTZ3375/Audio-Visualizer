#pragma once

class SampleLine{
    private:
        unsigned int m_ID;
        float m_Coords[12];
        float m_Colors[4];
        unsigned int m_Indices[6];

    public:
        SampleLine();
        SampleLine(unsigned int id, float basex, float basey, float height, float width, float r, float g, float b);
        ~SampleLine();
        const void fillVertices(float* target, int offset);
        const void fillIndices(unsigned int* target, int offset);
        const void changeID(unsigned int id);
        const void changeXPos(float distance);
        const void changeYPos(float distance);
        const void changeHeight(float difference);
        const void changeWidth(float difference);
        const void changeIndicesPosition(unsigned int id);

        inline const unsigned int getID(){ return m_ID; }
        inline const float* getCoords(){ return m_Coords; }
        inline const float* getColors(){ return m_Colors; }
        inline const unsigned int* getIndices(){ return m_Indices; }
};