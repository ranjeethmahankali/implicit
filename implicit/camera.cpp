#include "camera.h"
#include <iostream>
#include <algorithm>
static constexpr glm::vec3 unit_z = { 0.0f, 0.0f, 1.0f };
static constexpr float MAX_PHI = 1.5607963267948965f;

void camera::on_mouse_move(GLFWwindow* window, double xpos, double ypos)
{
    if (s_rightDown)
    {
        s_camTheta -= (float)(xpos - s_mousePos[0]) * ORBIT_ANG;
        s_camPhi += (float)(ypos - s_mousePos[1]) * ORBIT_ANG;
        s_camPhi = std::min(MAX_PHI, std::max(-MAX_PHI, s_camPhi));
        capture_mouse_pos(xpos, ypos);
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

        capture_mouse_pos(xpos, ypos);
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

    GL_CALL(glfwGetCursorPos(window, &s_mousePos.x, &s_mousePos.y));
}

void camera::on_mouse_scroll(GLFWwindow* window, double xOffset, double yOffset)
{
    static constexpr float zoomUp = 1.2f;
    static constexpr float zoomDown = 1.0f / zoomUp;

    s_camDist *= yOffset > 0 ? zoomDown : zoomUp;
}

void camera::capture_mouse_pos(double xpos, double ypos)
{
    s_mousePos.x = xpos;
    s_mousePos.y = ypos;
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
