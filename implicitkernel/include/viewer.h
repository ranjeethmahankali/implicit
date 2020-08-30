#pragma once
#include <glm.hpp>
#include <iostream>
#include "host_primitives.h"
#define NOMINMAX

/*glew.h, cl.hpp and glfw3.h should be included in this specific order to not get dumb warnings.*/
#include <GL/glew.h>
#define __CL_ENABLE_EXCEPTIONS
//#define __NO_STD_STRING
#define  _VARIADIC_MAX 16
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#include <CL/cl.hpp>
#include <GLFW/glfw3.h>

#ifdef _DEBUG
#define GL_CALL(fncall) {\
viewer::clear_gl_errors();\
fncall;\
if (viewer::log_gl_errors(#fncall, __FILE__, __LINE__)) __debugbreak();\
}
#else
#define GL_CALL(fncall) fncall
#endif

constexpr char ARROWS[] = ">>> ";

namespace camera
{
    /**
     * \brief The distance of the camera from the target
     * \return float The distance.
     */
    float distance();
    /**
     * \brief The polar coordinate theta of the camera with respect to the target.
     * \return float The angle.
     */
    float theta();
    /**
     * \brief The polar coordinate phi of the camera with respect to the target.
     * \return float The angle.
     */
    float phi();
    /**
     * \brief The target of the camera.
     * \return glm::vec3 The target location.
     */
    glm::vec3 target();

    // Handlers for mouse events.
    void on_mouse_move(GLFWwindow* window, double xpos, double ypos);
    void on_mouse_button(GLFWwindow* window, int button, int action, int mods);
    void on_mouse_scroll(GLFWwindow* window, double xOffset, double yOffset);
    static void capture_mouse_pos(double xpos, double ypos);
    void get_mouse_pos(uint32_t& x, uint32_t& y);
}

static bool check_format(const std::string& str, const std::string& suffix);

namespace viewer
{
    struct viewer_data
    {
        float camDistance;
        float camTheta;
        float camPhi;
        glm::vec3 camTarget;
        glm::vec3 minBounds;
        glm::vec3 maxBounds;
    };

    bool log_gl_errors(const char* function, const char* file, uint32_t line);
    void clear_gl_errors();
    /**
     * \brief Initializes the OpenGL part of the environment.
     */
    void init_ogl();
    void close_window();
    static const char* cl_err_str(cl_int err);
    bool window_should_close();
    void acquire_lock();
    uint32_t win_height();
    uint32_t win_width();

    void render_loop();
    void stop();

    /**
     * \brief Initializes the OpenCL part of the environment.
     */
    void init_ocl();
    void init_buffers();
    void set_work_group_size();
    static void pause_render_loop();
    static void resume_render_loop();
    static void add_render_data(uint8_t* bytes, size_t nBytes, uint8_t* types, uint32_t* offsets, size_t nEntities, op_step* steps, size_t nSteps);

    void show_entity(entities::ent_ref entity);

    void render();
    bool exportframe(const std::string& path);
    void setbounds(float(&bounds)[6]);

#ifdef CLDEBUG
    void setdebugmode(bool flag);
    bool getdebugmode();
    void debugstep();
#endif // CLDEBUG
};

namespace util
{
    template<typename T, typename size_t_inserter>
    void factorize(T num, size_t_inserter inserter)
    {
        T f = 2;
        while (f <= num)
        {
            if (num % f == 0)
            {
                *(inserter++) = f;
                num /= f;
                f = 2;
                continue;
            }
            f++;
        }
    };
}