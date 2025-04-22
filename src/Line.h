#pragma once

class Line{
    private:
        float m_Coords[12];
        float m_Colors[4];
        unsigned int m_Indices[6];

    public:
        Line();
        Line(unsigned int id, float basex, float basey, float height, float width, float r, float g, float b);
        ~Line();
        const void fillVertices(float* target, int offset);
        const void fillIndices(unsigned int* target, int offset);

        inline const float* getCoords(){ return m_Coords; }
        inline const float* getColors(){ return m_Colors; }
        inline const unsigned int* getIndices(){ return m_Indices; }
};