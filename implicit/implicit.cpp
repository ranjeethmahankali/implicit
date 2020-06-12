#include <vector>
#include <algorithm>
#include <iostream>
#include "camera.h"

#include <assert.h>
#include "kernel_sources.h"

#define __CL_ENABLE_EXCEPTIONS
//#define __NO_STD_STRING
#define  _VARIADIC_MAX 10
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#include <CL/cl.hpp>

#define CATCH_EXIT_CL_ERR catch (cl::Error err)\
{\
std::cerr << "OpenCL Error: " << cl_err_str(err.err()) << std::endl;\
exit(err.err());\
}

static constexpr uint32_t WIN_W = 960, WIN_H = 640;
static GLFWwindow* s_window;
static cl::ImageGL s_texture;
static cl::Context s_context;
static cl::CommandQueue s_queue;
static uint32_t s_pboId = 0;
static cl::BufferGL s_pBuffer;
static cl::Program s_program;
static cl::make_kernel<cl::BufferGL&, cl_float, cl_float, cl_float, cl_float3>* s_kernel;

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

    // Register mouse event handlers.
    GL_CALL(glfwSetCursorPosCallback(s_window, camera::on_mouse_move));
    GL_CALL(glfwSetMouseButtonCallback(s_window, camera::on_mouse_button));
    GL_CALL(glfwSetScrollCallback(s_window, camera::on_mouse_scroll));
}

static const char* cl_err_str(cl_int err)
{
#define CASE_RET(val) case val: return #val
    switch (err)
    {
    CASE_RET(CL_SUCCESS);
    CASE_RET(CL_DEVICE_NOT_FOUND);
    CASE_RET(CL_DEVICE_NOT_AVAILABLE);
    CASE_RET(CL_COMPILER_NOT_AVAILABLE);
    CASE_RET(CL_MEM_OBJECT_ALLOCATION_FAILURE);
    CASE_RET(CL_OUT_OF_RESOURCES);
    CASE_RET(CL_OUT_OF_HOST_MEMORY);
    CASE_RET(CL_PROFILING_INFO_NOT_AVAILABLE);
    CASE_RET(CL_MEM_COPY_OVERLAP);
    CASE_RET(CL_IMAGE_FORMAT_MISMATCH);
    CASE_RET(CL_IMAGE_FORMAT_NOT_SUPPORTED);
    CASE_RET(CL_BUILD_PROGRAM_FAILURE);
    CASE_RET(CL_MAP_FAILURE);
    CASE_RET(CL_MISALIGNED_SUB_BUFFER_OFFSET);
    CASE_RET(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
    CASE_RET(CL_COMPILE_PROGRAM_FAILURE);
    CASE_RET(CL_LINKER_NOT_AVAILABLE);
    CASE_RET(CL_LINK_PROGRAM_FAILURE);
    CASE_RET(CL_DEVICE_PARTITION_FAILED);
    CASE_RET(CL_KERNEL_ARG_INFO_NOT_AVAILABLE);

    // compile-time errors
    CASE_RET(CL_INVALID_VALUE);
    CASE_RET(CL_INVALID_DEVICE_TYPE);
    CASE_RET(CL_INVALID_PLATFORM);
    CASE_RET(CL_INVALID_DEVICE);
    CASE_RET(CL_INVALID_CONTEXT);
    CASE_RET(CL_INVALID_QUEUE_PROPERTIES);
    CASE_RET(CL_INVALID_COMMAND_QUEUE);
    CASE_RET(CL_INVALID_HOST_PTR);
    CASE_RET(CL_INVALID_MEM_OBJECT);
    CASE_RET(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR);
    CASE_RET(CL_INVALID_IMAGE_SIZE);
    CASE_RET(CL_INVALID_SAMPLER);
    CASE_RET(CL_INVALID_BINARY);
    CASE_RET(CL_INVALID_BUILD_OPTIONS);
    CASE_RET(CL_INVALID_PROGRAM);
    CASE_RET(CL_INVALID_PROGRAM_EXECUTABLE);
    CASE_RET(CL_INVALID_KERNEL_NAME);
    CASE_RET(CL_INVALID_KERNEL_DEFINITION);
    CASE_RET(CL_INVALID_KERNEL);
    CASE_RET(CL_INVALID_ARG_INDEX);
    CASE_RET(CL_INVALID_ARG_VALUE);
    CASE_RET(CL_INVALID_ARG_SIZE);
    CASE_RET(CL_INVALID_KERNEL_ARGS);
    CASE_RET(CL_INVALID_WORK_DIMENSION);
    CASE_RET(CL_INVALID_WORK_GROUP_SIZE);
    CASE_RET(CL_INVALID_WORK_ITEM_SIZE);
    CASE_RET(CL_INVALID_GLOBAL_OFFSET);
    CASE_RET(CL_INVALID_EVENT_WAIT_LIST);
    CASE_RET(CL_INVALID_EVENT);
    CASE_RET(CL_INVALID_OPERATION);
    CASE_RET(CL_INVALID_GL_OBJECT);
    CASE_RET(CL_INVALID_BUFFER_SIZE);
    CASE_RET(CL_INVALID_MIP_LEVEL);
    CASE_RET(CL_INVALID_GLOBAL_WORK_SIZE);
    CASE_RET(CL_INVALID_PROPERTY);
    CASE_RET(CL_INVALID_IMAGE_DESCRIPTOR);
    CASE_RET(CL_INVALID_COMPILER_OPTIONS);
    CASE_RET(CL_INVALID_LINKER_OPTIONS);
    CASE_RET(CL_INVALID_DEVICE_PARTITION_COUNT);

    // extension errors
    CASE_RET(CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR);
    CASE_RET(CL_PLATFORM_NOT_FOUND_KHR);
    default: return "Unknown OpenCL error";
    }
#undef CASE_RET
};

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
        s_program = cl::Program(s_context, cl_kernel_sources::render, true);
        s_kernel = new cl::make_kernel<cl::BufferGL&, cl_float, cl_float, cl_float, cl_float3>(s_program, "k_trace");
    }
    CATCH_EXIT_CL_ERR;
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
    CATCH_EXIT_CL_ERR;
}

static void render()
{
    try
    {
        cl_mem mem = s_pBuffer();
        clEnqueueAcquireGLObjects(s_queue(), 1, &mem, 0, 0, 0);
        s_queue.flush();
        s_queue.finish();
        if (s_kernel)
        {
            glm::vec3 ctarget = camera::target();
            (*s_kernel)(cl::EnqueueArgs(s_queue, cl::NDRange(WIN_W, WIN_H)),
                s_pBuffer,
                camera::distance(),
                camera::theta(),
                camera::phi(),
                { ctarget.x, ctarget.y, ctarget.z });
        }
        clEnqueueReleaseGLObjects(s_queue(), 1, &mem, 0, 0, 0);
        s_queue.flush();
        s_queue.finish();
    }
    CATCH_EXIT_CL_ERR;
}

int main()
{
    init_ogl();
    init_ocl();
    init_pbo();
    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(s_window))
    {
        render();
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