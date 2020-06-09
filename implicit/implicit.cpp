#include <iostream>
#include <vector>
#include <algorithm>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <assert.h>

#define __CL_ENABLE_EXCEPTIONS
//#define __NO_STD_STRING
#define  _VARIADIC_MAX 10
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#include <CL/cl.hpp>

#include "kernel_sources.h"

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
static uint32_t s_pboId = 0;
static cl::BufferGL s_pBuffer;
static cl::Program s_program;
static cl::make_kernel<cl::BufferGL&, cl_float, cl_float, cl_float>* s_kernel;

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

static void init_ogl()
{
    /* Initialize the library */
    glfwWindowHint(GLFW_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    if (!glfwInit())
    {
        std::cout << "GlewInit failed." << std::endl;
        return;
    }

    /* Create a windowed mode window and its OpenGL context */
    s_window = glfwCreateWindow(WIN_W, WIN_H, "Viewer", NULL, NULL);
    glfwSetWindowAttrib(s_window, GLFW_RESIZABLE, GLFW_FALSE);
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
        cl::Platform platform = cl::Platform::getDefault();
        cl_context_properties props[] =
        {
            CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(),
            CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(),
            CL_CONTEXT_PLATFORM, (cl_context_properties)platform(),
            0
        };
        std::vector<cl::Device> devices;
        platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
        if (devices.empty())
        {
            std::cerr << "No devices found" << std::endl;
            exit(1);
        }
        s_context = cl::Context(devices[0], props);
        s_queue = cl::CommandQueue(s_context, devices[0]);
        s_program = cl::Program(s_context, cl_kernel_sources::cube, true);
        s_kernel = new cl::make_kernel<cl::BufferGL&, cl_float, cl_float, cl_float>(s_program, "k_traceCube");
    }
    catch (cl::Error error)
    {
        std::cerr << "OpenCL Error" << std::endl;
        exit(1);
    }
};

static void init_pbo()
{
    // Initialize the pixel buffer object.
    if (s_pboId)
    {
        GL_CALL(clReleaseMemObject(s_pBuffer()));
        GL_CALL(glDeleteBuffers(1, &s_pboId));
    }

    std::vector<uint32_t> temp(WIN_W * WIN_H);
    std::generate(temp.begin(), temp.end(), []() { return (uint32_t)std::rand(); });

    GL_CALL(glGenBuffers(1, &s_pboId));
    GL_CALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, s_pboId));
    GL_CALL(glBufferData(GL_PIXEL_UNPACK_BUFFER, WIN_W * WIN_H * sizeof(uint32_t), temp.data(), GL_STREAM_DRAW));
    GL_CALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));

    try
    {
        cl_int err = 0;
        s_pBuffer = cl::BufferGL(s_context, CL_MEM_WRITE_ONLY, s_pboId, &err);
        if (err)
        {
            std::cerr << "OpenCL Error" << std::endl;
            exit(1);
        }
    }
    catch (cl::Error error)
    {
        std::cerr << "OpenCL Error" << std::endl;
        exit(1);
    }
}

static void render(float ang)
{
    try
    {
        cl_mem mem = s_pBuffer();
        clEnqueueAcquireGLObjects(s_queue(), 1, &mem, 0, 0, 0);
        s_queue.flush();
        s_queue.finish();
        if (s_kernel)
        {
            (*s_kernel)(cl::EnqueueArgs(s_queue, cl::NDRange(WIN_W, WIN_H)), s_pBuffer, 3.0f, ang, ang);
        }
        clEnqueueReleaseGLObjects(s_queue(), 1, &mem, 0, 0, 0);
        s_queue.flush();
        s_queue.finish();
    }
    catch (cl::Error error)
    {
        std::cerr << "Open CL error" << std::endl;
        exit(1);
    }
}

int main()
{
    init_ogl();
    init_ocl();
    init_pbo();
    float ang = 0.0f;
    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(s_window))
    {
        render(ang);
        ang += 0.05f;
        GL_CALL(glClear(GL_COLOR_BUFFER_BIT));
        GL_CALL(glDisable(GL_DEPTH_TEST));

        GL_CALL(glRasterPos2i(-1, -1));
        GL_CALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, s_pboId));
        GL_CALL(glDrawPixels(WIN_W, WIN_H, GL_RGBA, GL_UNSIGNED_BYTE, nullptr));
        GL_CALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));

        /* Swap front and back buffers */
        GL_CALL(glfwSwapBuffers(s_window));

        /* Poll for and process events */
        GL_CALL(glfwPollEvents());
    }

    glfwTerminate();
    delete s_kernel;
    return 0;
}