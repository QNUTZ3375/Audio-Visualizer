#include "ErrorHandler.h"

void GLClearError(){
    while(glGetError() != GL_NO_ERROR);
}

bool GLLogCall(const char* function, const char* file, int line){
    while(GLenum error = glGetError()){
        std::cout << "[OpenGL Error] (0x" << std::hex << error << ") " << function << " " << file << ": line " << std::dec << line << std::endl;
        return false;
    }
    return true;
}