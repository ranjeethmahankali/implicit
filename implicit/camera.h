#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define __CL_ENABLE_EXCEPTIONS
//#define __NO_STD_STRING
#define  _VARIADIC_MAX 10
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#include <CL/cl.hpp>

#ifdef _DEBUG
#define GL_CALL(fncall) {\
clear_gl_errors();\
fncall;\
if (log_gl_errors(#fncall, __FILE__, __LINE__)) __debugbreak();\
}
#else
#define GL_CALL(fncall) fncall
#endif // DEBUG

static bool s_leftDown = false;
static bool s_rightDown = false;
static float s_camDist = 10.0f;
static float s_camTheta = 0.6f;
static float s_camPhi = 0.77f;
static cl_float3 s_camTarget = { 0.0f, 0.0f, 0.0f };
static double s_mousePos[2] = { 0.0, 0.0 };
static constexpr float ORBIT_ANG = 0.005f;

bool log_gl_errors(const char* function, const char* file, uint32_t line);
void clear_gl_errors();

namespace camera
{
    float distance();
    float theta();
    float phi();
    cl_float3 target();

    // Handlers.
    void on_mouse_move(GLFWwindow* window, double xpos, double ypos);
    void on_mouse_button(GLFWwindow* window, int button, int action, int mods);
    void on_mouse_scroll(GLFWwindow* window, double xOffset, double yOffset);
}