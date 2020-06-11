#include "gl_util.h"
#include <glm.hpp>

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