#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>

#ifdef _DEBUG
#define GL_CALL(fncall) {\
clear_gl_errors();\
fncall;\
if (log_gl_errors(#fncall, __FILE__, __LINE__)) __debugbreak();\
}
#else
#define GL_CALL(fncall) fncall
#endif // DEBUG

static constexpr float CAM_DIST = 10.0f;
static constexpr float CAM_THETA = 0.6f;
static constexpr float CAM_PHI = 0.77f;
static constexpr glm::vec3 CAM_TARGET = { 0.0f, 0.0f, 0.0f };
static constexpr float ORBIT_ANG = 0.005f;

static bool s_leftDown = false;
static bool s_rightDown = false;
static float s_camDist = CAM_DIST;
static float s_camTheta = CAM_THETA;
static float s_camPhi = CAM_PHI;
static glm::vec3 s_camTarget = CAM_TARGET;
static double s_mousePos[2] = { 0.0, 0.0 };

bool log_gl_errors(const char* function, const char* file, uint32_t line);
void clear_gl_errors();

namespace camera
{
    float distance();
    float theta();
    float phi();
    glm::vec3 target();

    // Handlers.
    void on_mouse_move(GLFWwindow* window, double xpos, double ypos);
    void on_mouse_button(GLFWwindow* window, int button, int action, int mods);
    void on_mouse_scroll(GLFWwindow* window, double xOffset, double yOffset);
}