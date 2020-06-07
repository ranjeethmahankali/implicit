#include <iostream>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <assert.h>

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

static constexpr uint32_t WIN_W = 640, WIN_H = 480;
static GLFWwindow* s_window;
static cl::ImageGL s_texture;
static cl::Context s_context;
static cl::CommandQueue s_queue;
static uint32_t s_texId;

static bool log_gl_errors(const char* function, const char* file, uint32_t line)
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

static void clear_gl_errors()
{
    // Just loop over and consume all pending errors.
    GLenum error = glGetError();
    while (error)
    {
        error = glGetError();
    }
}

static void viewer_init()
{
    /* Initialize the library */
    /*glfwWindowHint(GLFW_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_VERSION_MINOR, 6);*/
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    if (!glfwInit())
    {
        std::cout << "GlewInit failed." << std::endl;
        return;
    }

    /* Create a windowed mode window and its OpenGL context */
    s_window = glfwCreateWindow(WIN_W, WIN_H, "Viewer", NULL, NULL);
    if (!s_window)
    {
        glfwTerminate();
        return;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(s_window);

    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        std::cout << "GlewInit failed." << std::endl;
        return;
    }

    std::cout << "Using OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
}

static void init_ocl()
{
    try
    {
        cl_context_properties props[] =
        {
            CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(),
            CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(),
            CL_CONTEXT_PLATFORM, (cl_context_properties)(cl::Platform::getDefault())(),
            0
        };
        std::vector<cl::Device> devices;
        cl::Platform::getDefault().getDevices(CL_DEVICE_TYPE_GPU, &devices);
        if (devices.empty())
        {
            std::cerr << "No devices found" << std::endl;
            exit(1);
        }
        s_context = cl::Context(devices[0], props);
        s_queue = cl::CommandQueue(s_context, devices[0]);
    }
    catch (cl::Error error)
    {
        std::cerr << "OpenCL Error" << std::endl;
        exit(1);
    }
};

int main()
{
    init_ocl();
    viewer_init();
    size_t i = 0;
    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(s_window))
    {
        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT);

        /* Swap front and back buffers */
        glfwSwapBuffers(s_window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}