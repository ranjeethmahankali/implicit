#define NOMINMAX
#include <vector>
#include <algorithm>
#include <iostream>
#include <algorithm>
#include "camera.h"

#include <assert.h>
#include "kernel_sources.h"
#include "host_primitives.h"

#define __CL_ENABLE_EXCEPTIONS
//#define __NO_STD_STRING
#define  _VARIADIC_MAX 16
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#include <CL/cl.hpp>

#define CATCH_EXIT_CL_ERR catch (cl::Error err)\
{\
std::cerr << "OpenCL Error: " << cl_err_str(err.err()) << std::endl;\
exit(err.err());\
}

static constexpr uint32_t WIN_W = 960, WIN_H = 640;
//static constexpr uint32_t WIN_W = 1, WIN_H = 1;
static GLFWwindow* s_window;
static cl::ImageGL s_texture;
static cl::Context s_context;
static cl::CommandQueue s_queue;
static uint32_t s_pboId = 0;
static cl::Program s_program;
static cl::make_kernel<cl::BufferGL&, cl::Buffer&, cl::Buffer&, cl::Buffer&, cl::LocalSpaceArg, cl_uint, cl::Buffer&, cl_uint, cl_float3, cl_float3>* s_kernel;

static cl::BufferGL s_pBuffer; // Pixels to be rendered to the screen.
static cl::Buffer s_packedBuf; // Packed bytes of simple entities.
static cl::Buffer s_typeBuf; // The types of simple entities.
static cl::Buffer s_offsetBuf; // Offsets where the simple entities start in the packedBuf.
static cl::Buffer s_opStepBuf; // Buffer containing csg operators.
static cl::LocalSpaceArg s_valueBuf; // Local buffer for storing the values of implicit functions when computing csg operations.
static size_t s_numCurrentEntities = 0;
static size_t s_opStepCount = 0;

static size_t s_globalMemSize = 0;
static size_t s_localMemSize = 0;
static size_t s_maxBufSize = 0;
static size_t s_maxLocalBufSize = 0;
static size_t s_workGroupSize = 0;

constexpr static size_t MAX_ENTITY_COUNT = 64;

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

        s_kernel = new cl::make_kernel<
            cl::BufferGL&, cl::Buffer&, cl::Buffer&, cl::Buffer&, cl::LocalSpaceArg,
            cl_uint, cl::Buffer&, cl_uint, cl_float3, cl_float3
        >(s_program, "k_trace");

        s_globalMemSize = devices[0].getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>();
        s_maxBufSize = s_globalMemSize / 32;
        s_localMemSize = devices[0].getInfo<CL_DEVICE_LOCAL_MEM_SIZE>();
        s_maxLocalBufSize = s_localMemSize / 2;
        s_valueBuf = cl::Local(s_maxLocalBufSize);
        s_workGroupSize =
            std::min((size_t)WIN_W, std::min(
                devices[0].getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>(),
                (size_t)std::ceil(s_maxLocalBufSize / (sizeof(float) * MAX_ENTITY_COUNT))));
    }
    CATCH_EXIT_CL_ERR;
};

static void init_buffers()
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

        s_packedBuf = cl::Buffer(s_context, CL_MEM_HOST_WRITE_ONLY | CL_MEM_READ_ONLY, s_maxBufSize);
        s_typeBuf = cl::Buffer(s_context, CL_MEM_HOST_WRITE_ONLY | CL_MEM_READ_ONLY, s_maxBufSize);
        s_offsetBuf = cl::Buffer(s_context, CL_MEM_HOST_WRITE_ONLY | CL_MEM_READ_ONLY, s_maxBufSize);
        s_opStepBuf = cl::Buffer(s_context, CL_MEM_HOST_WRITE_ONLY | CL_MEM_READ_ONLY, s_maxBufSize);
    }
    CATCH_EXIT_CL_ERR;
}

template <typename T>
void write_buf(cl::Buffer& buffer, T* data, size_t size)
{
    size_t nBytes = size * sizeof(T);
    if (nBytes > s_maxBufSize)
    {
        std::cerr << "Device buffer overflow... terminating application" << std::endl;
        exit(1);
    }
    if (nBytes == 0) return;

    s_queue.enqueueWriteBuffer(buffer, CL_TRUE, 0, size * sizeof(T), data);
};

void show_entity(const entities::entity& entity)
{
    try
    {
        size_t nBytes = 0, nEntities = 0, nSteps = 0;
        entity.render_data_size(nBytes, nEntities, nSteps);
        std::vector<uint8_t> bytes(nBytes);
        std::vector<uint32_t> offsets(nEntities);
        std::vector<uint8_t> types(nEntities);
        std::vector<op_step> steps(nSteps);

        // Copy the render data into these buffers.
        {
            uint8_t* bptr = bytes.data();
            uint32_t* optr = offsets.data();
            uint8_t* tptr = types.data();
            op_step* sptr = steps.data();
            size_t ei = 0, co = 0;
            entity.copy_render_data(bptr, optr, tptr, sptr, ei, co);
        }
        
        write_buf(s_packedBuf, bytes.data(), nBytes);
        write_buf(s_typeBuf, types.data(), nEntities);
        write_buf(s_offsetBuf, offsets.data(), nEntities);
        write_buf(s_opStepBuf, steps.data(), nSteps);
        s_numCurrentEntities = nEntities;
        s_opStepCount = nSteps;
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
            (*s_kernel)(cl::EnqueueArgs(s_queue, cl::NDRange(WIN_W, WIN_H), cl::NDRange(s_workGroupSize, 1ui64)),
                s_pBuffer,
                s_packedBuf,
                s_typeBuf,
                s_offsetBuf,
                s_valueBuf,
                (cl_uint)s_numCurrentEntities,
                s_opStepBuf,
                (cl_uint)s_opStepCount,
                { camera::distance(), camera::theta(), camera::phi() },
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
    init_buffers();

    //entities::box3 e(-5.0f, -5.0f, -5.0f, 5.0f, 5.0f, 5.0f);
    entities::box3 e1(-5.0f, -5.0f, -5.0f, 5.0f, 5.0f, 5.0f);
    entities::gyroid e2(2.0f, 0.2f);
    entities::sphere3 e3(-5.0f, 5.0f, 0.0f, 3.0f);
    entities::sphere3 e4(5.0f, -5.0f, 0.0f, 3.0f);
    entities::csg_entity ec(&e1, &e3, op_type::OP_UNION);
    entities::csg_entity ec2(&ec, &e2, op_type::OP_INTERSECTION);
    entities::csg_entity ent(&ec2, &e4, op_type::OP_SUBTRACTION);
    //entities::sphere3 e(0.0f, 0.0f, 0.0f, 5.0f);
    show_entity(ent);

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