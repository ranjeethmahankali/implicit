#include "camera.h"
#include <iostream>
static constexpr glm::vec3 unit_z = { 0.0f, 0.0f, 1.0f };

void camera::on_mouse_move(GLFWwindow* window, double xpos, double ypos)
{
    if (s_rightDown)
    {
        s_camTheta -= (float)(xpos - s_mousePos[0]) * ORBIT_ANG;
        s_camPhi += (float)(ypos - s_mousePos[1]) * ORBIT_ANG;
        s_mousePos[0] = xpos;
        s_mousePos[1] = ypos;
    }

    if (s_leftDown)
    {
        float
            st = std::sinf(s_camTheta), ct = std::cosf(s_camTheta),
            sp = std::sinf(s_camPhi), cp = std::cosf(s_camPhi);
        glm::vec3 dir = { s_camDist * cp * ct, s_camDist * cp * st, s_camDist * sp };
        glm::vec3 pos = s_camTarget + dir;
        dir = glm::normalize(-dir);
        
        glm::vec3 x = glm::normalize(glm::cross(dir, unit_z));
        glm::vec3 y = glm::normalize(glm::cross(x, dir));
        glm::vec2 diff = { (float)(s_mousePos[0] - xpos), (float)(ypos - s_mousePos[1]) };
        diff /= 20.0f;
        s_camTarget += x * diff.x + y * diff.y;

        s_mousePos[0] = xpos;
        s_mousePos[1] = ypos;
    }
}

void camera::on_mouse_button(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        if (action == GLFW_PRESS)
            s_rightDown = true;
        if (action == GLFW_RELEASE)
            s_rightDown = false;
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
            s_leftDown = true;
        if (action == GLFW_RELEASE)
            s_leftDown = false;
    }

    GL_CALL(glfwGetCursorPos(window, &s_mousePos[0], &s_mousePos[1]));
}

void camera::on_mouse_scroll(GLFWwindow* window, double xOffset, double yOffset)
{
    static constexpr float zoomUp = 1.2f;
    static constexpr float zoomDown = 1.0f / zoomUp;

    s_camDist *= yOffset > 0 ? zoomDown : zoomUp;
}

float camera::distance()
{
    return s_camDist;
}

float camera::theta()
{
    return s_camTheta;
}

float camera::phi()
{
    return s_camPhi;
}

glm::vec3 camera::target()
{
    return s_camTarget;
}

bool log_gl_errors(const char* function, const char* file, uint32_t line)
{
    static bool found_error = false;
    while (GLenum error = glGetError())
    {
        std::cout << "[OpenGL Error] (0x" << std::hex << error << std::dec << ")";
#if _DEBUG
        std::cout << " in " << function << " at " << file << ":" << line;
#endif // NDEBUG
        std::cout << std::endl;
        found_error = true;
    }
    if (found_error)
    {
        found_error = false;
        return true;
    }
    else
    {
        return false;
    }
}

void clear_gl_errors()
{
    // Just loop over and consume all pending errors.
    GLenum error = glGetError();
    while (error)
    {
        error = glGetError();
    }
}
