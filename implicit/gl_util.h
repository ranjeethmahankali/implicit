#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#ifdef _DEBUG
#define GL_CALL(fncall) {\
gl_util::clear_gl_errors();\
fncall;\
if (gl_util::log_gl_errors(#fncall, __FILE__, __LINE__)) __debugbreak();\
}
#else
#define GL_CALL(fncall) fncall
#endif

namespace gl_util
{
    bool log_gl_errors(const char* function, const char* file, uint32_t line);
    void clear_gl_errors();
}