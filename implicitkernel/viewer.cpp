#include <iostream>
#include <thread>
#include <mutex>
#include <algorithm>
#include "kernel_sources.h"
#include "viewer.h"
#pragma warning(push)
#pragma warning(disable: 4244)
#include <boost/gil/image.hpp>
#include <boost/gil/typedefs.hpp>
#include <boost/gil/extension/io/bmp.hpp>
namespace bgil = boost::gil;
#include <boost/algorithm/string/case_conv.hpp>
#pragma warning(pop)

#include <chrono>

#define CATCH_EXIT_CL_ERR catch (cl::Error err)\
{\
std::cerr << "OpenCL Error: " << viewer::cl_err_str(err.err()) << std::endl;\
exit(err.err());\
}

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
static glm::dvec2 s_mousePos = { 0.0, 0.0 };

static uint8_t s_lowestLOD = 0;

//static constexpr uint32_t WIN_W = 960, WIN_H = 640;
static constexpr uint32_t WIN_W = 1024, WIN_H = 728;
//static constexpr uint32_t WIN_W = 1, WIN_H = 1;
static GLFWwindow* s_window;
static cl::ImageGL s_texture;
static cl::Context s_context;
static cl::CommandQueue s_queue;
static uint32_t s_pboId = 0; // Pixel buffer to be rendered to screen, controlled by OpenGL.
static cl::Program s_program;
static cl::make_kernel<
    cl::BufferGL&, cl::Buffer&, cl::Buffer&, cl::Buffer&, cl::LocalSpaceArg,
    cl::LocalSpaceArg, cl_uint, cl::Buffer&, cl_uint, cl::Buffer&, cl_uchar
#ifdef CLDEBUG
    , cl_uint2
#endif // CLDEBUG
>* s_kernel;
static cl::make_kernel<cl::BufferGL&, cl_uchar>* s_repeatPixelKernel;

static cl::BufferGL s_pBuffer; // Pixels to be rendered to the screen. Controlled by OpenCL.
static cl::Buffer s_packedBuf; // Packed bytes of simple entities.
static cl::Buffer s_typeBuf; // The types of simple entities.
static cl::Buffer s_offsetBuf; // Offsets where the simple entities start in the packedBuf.
static cl::Buffer s_opStepBuf; // Buffer containing csg operators.
static cl::Buffer s_viewerDataBuf; // Buffer contains viewer data, camera position, direction and build volume bounds.
static uint8_t s_levelOfDetail = s_lowestLOD;
static cl::LocalSpaceArg s_valueBuf; // Local buffer for storing the values of implicit functions when computing csg operations.
static cl::LocalSpaceArg s_regBuf; // Register to store intermediate csg values.
static size_t s_numCurrentEntities = 0;
static size_t s_opStepCount = 0;

static size_t s_globalMemSize = 0;
static size_t s_localMemSize = 0;
static size_t s_constMemSize = 0;
static size_t s_maxBufSize = 0;
static size_t s_maxLocalBufSize = 0;
static size_t s_maxWorkGroupSize = 0;
static size_t s_workGroupSize = 0;

static std::mutex s_mutex;
static std::condition_variable s_cv;
static bool s_pauseRender = false;
static bool s_shouldExit = false;

static glm::vec3 s_minBounds = { -20.0f, -20.0f, -20.0f };
static glm::vec3 s_maxBounds = {  20.0f,  20.0f,  20.0f };

#ifdef CLDEBUG
static bool s_debugMode = false;
static std::chrono::high_resolution_clock::time_point s_framestart;
static std::chrono::high_resolution_clock::time_point s_frameend;
#endif // CLDEBUG

bool viewer::log_gl_errors(const char* function, const char* file, uint32_t line)
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

void viewer::clear_gl_errors()
{
    // Just loop over and consume all pending errors.
    GLenum error = glGetError();
    while (error)
    {
        error = glGetError();
    }
}

void viewer::init_ogl()
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

void viewer::close_window()
{
    viewer::pause_render_loop();
    s_shouldExit = true;
    viewer::resume_render_loop();
}

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
        viewer::reset_LOD();
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
        viewer::reset_LOD();
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
    viewer::reset_LOD();
}

void camera::capture_mouse_pos(double xpos, double ypos)
{
    s_mousePos.x = xpos;
    s_mousePos.y = ypos;
}

void camera::get_mouse_pos(uint32_t& x, uint32_t& y)
{
    double xpos, ypos;
    glfwGetCursorPos(s_window, &xpos, &ypos);
    capture_mouse_pos(xpos, ypos);
    x = (uint32_t)xpos;
    y = (uint32_t)ypos;
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

static const char* viewer::cl_err_str(cl_int err)
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
}

bool viewer::window_should_close()
{
    return glfwWindowShouldClose(s_window);
}

void viewer::acquire_lock()
{
    while (s_pauseRender)
    {
        std::unique_lock<std::mutex> lock(s_mutex);
        s_cv.wait(lock);
        lock.unlock();
    }
}

uint32_t viewer::win_height()
{
    return WIN_H;
}

uint32_t viewer::win_width()
{
    return WIN_W;
}

void viewer::render_loop()
{
    /* Loop until the user closes the window */
    while (!viewer::window_should_close() && !s_shouldExit)
    {
        viewer::acquire_lock();
#ifdef CLDEBUG
        if (s_debugMode) s_framestart = std::chrono::high_resolution_clock::now();
#endif
        viewer::render();
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

#ifdef CLDEBUG
        if (s_debugMode)
        {
            s_frameend = std::chrono::high_resolution_clock::now();
            std::cout << "\n\n";
            std::cout << "Frame time: "
                << std::chrono::duration_cast<std::chrono::milliseconds>(s_frameend - s_framestart).count() << "ms\n";
            std::cout << "Viewer paused in debug mode...\n" << ARROWS;
            pause_render_loop();
        }
#endif // CLDEBUG
    }
}

void viewer::stop()
{
    GL_CALL(glfwSetWindowShouldClose(s_window, GL_TRUE));
    glfwTerminate();
    delete s_kernel;
    delete s_repeatPixelKernel;
}

void viewer::render()
{
    try
    {
        cl_mem mem = s_pBuffer();
        clEnqueueAcquireGLObjects(s_queue(), 1, &mem, 0, 0, 0);
        s_queue.flush();
        s_queue.finish();
        if (s_kernel)
        {
#ifdef CLDEBUG
            cl_uint2 mousePos = { UINT32_MAX, UINT32_MAX };
            if (viewer::getdebugmode())
            {
                uint32_t x, y;
                camera::get_mouse_pos(x, y);
                mousePos = { x, WIN_H - y };
            }
#endif // CLDEBUG
            cl::EnqueueArgs args = cl::EnqueueArgs(s_queue, cl::NDRange(WIN_W, WIN_H), cl::NDRange(s_workGroupSize, 1ui64));
            viewer_data vdata
            {
                camera::distance(), camera::theta(), camera::phi(),
                camera::target(),
                s_minBounds,
                s_maxBounds
            };
            s_queue.enqueueWriteBuffer(s_viewerDataBuf, CL_TRUE, 0, sizeof(vdata), &vdata);
            (*s_kernel)(
                args,
                s_pBuffer,
                s_packedBuf,
                s_typeBuf,
                s_offsetBuf,
                s_valueBuf,
                s_regBuf,
                (cl_uint)s_numCurrentEntities,
                s_opStepBuf,
                (cl_uint)s_opStepCount,
                s_viewerDataBuf,
                (cl_uchar)s_levelOfDetail
#ifdef CLDEBUG
                , mousePos
#endif // CLDEBUG
            );
            if (s_repeatPixelKernel && s_levelOfDetail > 0)
            {
                (*s_repeatPixelKernel)(args, s_pBuffer, (cl_uchar)s_levelOfDetail);
            }
            update_LOD();
        }
        clEnqueueReleaseGLObjects(s_queue(), 1, &mem, 0, 0, 0);
        s_queue.flush();
        s_queue.finish();
    }
    CATCH_EXIT_CL_ERR;
}

void viewer::update_LOD()
{
    if (s_levelOfDetail)
        s_levelOfDetail--;
}

void viewer::reset_LOD()
{
    s_levelOfDetail = s_lowestLOD;
}

bool viewer::exportframe(const std::string& path)
{
    try
    {
        size_t nPixels = WIN_W * WIN_H;
        std::vector<uint8_t> pdata(nPixels * 4); // 4 channels per pixel.
        {
            cl_mem mem = s_pBuffer();
            pause_render_loop();
            clEnqueueAcquireGLObjects(s_queue(), 1, &mem, 0, 0, 0);
            s_queue.enqueueReadBuffer(s_pBuffer, true, 0, nPixels * sizeof(uint32_t), pdata.data());
            clEnqueueReleaseGLObjects(s_queue(), 1, &mem, 0, 0, 0);
            resume_render_loop();
        }
        bgil::rgba8_image_t img(WIN_W, WIN_H);
        auto dataIt = pdata.cbegin();
        // We need the flipped view because the y-axis in boost goes from bottom to top.
        auto flippedView = bgil::flipped_up_down_view(bgil::view(img));
        auto imgIt = flippedView.begin();
        auto imgEnd = flippedView.end();
        while (dataIt != pdata.cend() && imgIt != imgEnd)
        {
            uint8_t r = *(dataIt++);
            uint8_t g = *(dataIt++);
            uint8_t b = *(dataIt++);
            uint8_t a = *(dataIt++);
            *(imgIt++) = bgil::rgba8_pixel_t(r, g, b, a);
        }
        if (check_format(path, ".bmp"))
        {
            bgil::write_view(path, bgil::view(img), bgil::bmp_tag{});
        }
        else
        {
            std::cerr << "Cannot export this format." << std::endl;
            return false;
        }

        return true;
    }
    CATCH_EXIT_CL_ERR;
}

void viewer::setbounds(float(&bounds)[6])
{
    s_minBounds.x = bounds[0];
    s_minBounds.y = bounds[1];
    s_minBounds.z = bounds[2];
    s_maxBounds.x = bounds[3];
    s_maxBounds.y = bounds[4];
    s_maxBounds.z = bounds[5];
}

void viewer::adaptive_rendermode(uint8_t lod)
{
    if (lod > 8) lod = 8;
    s_lowestLOD = lod;
}

#ifdef CLDEBUG
void viewer::setdebugmode(bool flag)
{
    s_debugMode = flag;
    if (!s_debugMode) debugstep();
}

bool viewer::getdebugmode()
{
    return s_debugMode;
}

void viewer::debugstep()
{
    resume_render_loop();
}
#endif // CLDEBUG

void viewer::init_ocl()
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
        s_program = cl::Program(s_context, cl_kernel_sources::render_kernel(), false);
        std::string optionStr = "-I \"" + cl_kernel_sources::abs_path() + "\"";
#ifdef CLDEBUG
        optionStr += " -D CLDEBUG";
#endif // CLDEBUG
        try
        {
            s_program.build(optionStr.c_str());

            s_kernel = new cl::make_kernel<
                cl::BufferGL&, cl::Buffer&, cl::Buffer&, cl::Buffer&, cl::LocalSpaceArg,
                cl::LocalSpaceArg, cl_uint, cl::Buffer&, cl_uint, cl::Buffer&, cl_uchar
#ifdef CLDEBUG
                , cl_uint2
#endif // CLDEBUG
            >(s_program, "k_trace");

            s_repeatPixelKernel = new cl::make_kernel<cl::BufferGL&, cl_uchar>(s_program, "k_repeatPixels");
        }
        catch (cl::Error error)
        {
            std::string log = s_program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(cl::Device::getDefault());
            std::cerr << "Error - " << error.err() << " when building the kernel program. Error log: " << std::endl;
            std::cerr << log << std::endl;
        }
        s_globalMemSize = devices[0].getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>();
        s_maxBufSize = s_globalMemSize / 32;
        s_localMemSize = devices[0].getInfo<CL_DEVICE_LOCAL_MEM_SIZE>();
        s_constMemSize = devices[0].getInfo< CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE>();
        s_maxLocalBufSize = s_localMemSize / 4;
        s_valueBuf = cl::Local(s_maxLocalBufSize);
        s_regBuf = cl::Local(s_maxLocalBufSize);
        s_maxWorkGroupSize = devices[0].getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();
        viewer::set_work_group_size();
    }
    CATCH_EXIT_CL_ERR;
}

void viewer::init_buffers()
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
        s_viewerDataBuf = cl::Buffer(s_context, CL_MEM_HOST_WRITE_ONLY | CL_MEM_READ_ONLY, 12 * sizeof(float));
    }
    CATCH_EXIT_CL_ERR;
}

void viewer::set_work_group_size()
{
    if (s_numCurrentEntities > MAX_ENTITY_COUNT)
    {
        throw "too many entities";
    }
    size_t nEntities = std::max(1ui64, s_numCurrentEntities);
    std::vector<size_t> factors;
    auto fIter = std::back_inserter(factors);
    size_t width = (size_t)WIN_W;
    util::factorize(width, fIter);
    std::sort(factors.begin(), factors.end());
    s_workGroupSize =
        std::min(width, std::min(
            s_maxWorkGroupSize,
            (size_t)std::ceil(s_maxLocalBufSize / (sizeof(float) * nEntities))));
    if (width % s_workGroupSize)
    {
        size_t newSize = width;
        for (size_t f : factors)
        {
            newSize /= f;
            if (newSize <= s_workGroupSize) break;
        }
        s_workGroupSize = newSize;
    }
}

void viewer::pause_render_loop()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    s_pauseRender = true;
}

void viewer::resume_render_loop()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    s_pauseRender = false;
    s_cv.notify_one();
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

void viewer::add_render_data(uint8_t* bytes, size_t nBytes, uint32_t* types, uint32_t* offsets, size_t nEntities, op_step* steps, size_t nSteps)
{
    try
    {
        pause_render_loop();
        write_buf(s_packedBuf, bytes, nBytes);
        write_buf(s_typeBuf, types, nEntities);
        write_buf(s_offsetBuf, offsets, nEntities);
        write_buf(s_opStepBuf, steps, nSteps);
        s_numCurrentEntities = nEntities;
        s_opStepCount = nSteps;
        set_work_group_size();

        // Resume the render loop.
        resume_render_loop();
    }
    CATCH_EXIT_CL_ERR;
}

void viewer::show_entity(entities::ent_ref entity)
{
    size_t nBytes = 0, nEntities = 0, nSteps = 0;
    entity->render_data_size(nBytes, nEntities, nSteps);
    std::vector<uint8_t> bytes(nBytes);
    std::vector<uint32_t> offsets(nEntities);
    std::vector<uint32_t> types(nEntities);
    std::vector<op_step> steps(nSteps);

    // Copy the render data into these buffers.
    {
        uint8_t* bptr = bytes.data();
        uint32_t* optr = offsets.data();
        uint32_t* tptr = types.data();
        op_step* sptr = steps.data();
        entity->copy_render_data(bptr, optr, tptr, sptr);
    }

    viewer::add_render_data(bytes.data(), nBytes, types.data(), offsets.data(), nEntities, steps.data(), nSteps);
}

bool check_format(const std::string& path, const std::string& ext)
{
    std::string pathLower(path);
    boost::to_lower(pathLower);
    return pathLower.size() >= ext.size() && pathLower.compare(path.size() - ext.size(), ext.size(), ext) == 0;
}
