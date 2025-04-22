#include "ImgTexture.h"
#include "ErrorHandler.h"
#include "Renderer.h"
#include "vendor/stb_image/stb_image.h"
#include <iostream>

Texture::Texture(const std::string& path)
    : m_RendererID(0), m_FilePath(path), m_LocalBuffer(nullptr), 
    m_Width(0), m_Height(0), m_BPP(0){

    //mess around with this flag if the texture renders upside down
    stbi_set_flip_vertically_on_load(1);
    //load the image into the buffer
    m_LocalBuffer = stbi_load(path.c_str(), &m_Width, &m_Height, &m_BPP, 4);
    
    //generate and bind a new texture
    GLCall(glGenTextures(1, &m_RendererID));
    GLCall(glBindTexture(GL_TEXTURE_2D, m_RendererID));

    //set all the parameters of the texture (required)
    //GL_TEXTURE_MIN_FILTER means minification (if image is larger than size to draw)
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    //GL_TEXTURE_MAG_FILTER means magnification (if image is skaller than size to draw)
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    //GL_TEXTURE_WRAP_S means horizontal wrap, clamp means not to wrap
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    //GL_TEXTURE_WRAP_T means tiling (vertical wrap), clamp means not to wrap
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

    //check if buffer has been loaded correctly
    if (m_LocalBuffer){
        //send image data to OpenGL
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_LocalBuffer);
        //unbind the texture
        glBindTexture(GL_TEXTURE_2D, 0);
        //free the image buffer
        stbi_image_free(m_LocalBuffer);
    }
    else{
        std::cout << "\nError: Failed to load texture" << std::endl;
        std::cout << stbi_failure_reason() << std::endl;
        exit(1);
    }
}

Texture::~Texture(){
    GLCall(glDeleteTextures(1, &m_RendererID));
}

void Texture::Bind(unsigned int slot) const{
    //bind the current texture to the specified slot (argument 1)
    //double check how many slots your system has 
    //(modern typically has 32, mobile typically has 8)
    GLCall(glActiveTexture(GL_TEXTURE0 + slot));
    GLCall(glBindTexture(GL_TEXTURE_2D, m_RendererID));
}

void Texture::Unbind() const{
    glBindTexture(GL_TEXTURE_2D, 0);
}