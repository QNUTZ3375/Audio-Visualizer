#pragma once

class SampleLine{
    private:
        unsigned int m_ID;
        float m_Coords[12];
        float m_Colors[4];
        unsigned int m_Indices[6];

    public:
        SampleLine(unsigned int id, float basex, float basey, float heightLeft, float heightRight, float width, float r, float g, float b, float a);
        SampleLine(unsigned int id, float basex, float basey, float height, float width, float r, float g, float b, float a);
        ~SampleLine();
        void changeColor(float r, float g, float b, float a);
        const void fillVertices(float* target, int offset);
        const void fillIndices(unsigned int* target, int offset);
        void changeID(unsigned int id);
        void changeXPos(float distance);
        void changeYPos(float distance);
        void changeHeight(float newHeight);
        void changeWidth(float newWidth);
        void changeIndicesPosition(unsigned int id);

        inline const unsigned int getID(){ return m_ID; }
        inline const float getWidth(){ return m_Coords[0] - m_Coords[3]; }
        inline const float* getCoords(){ return m_Coords; }
        inline const float* getColors(){ return m_Colors; }
        inline const unsigned int* getIndices(){ return m_Indices; }
};