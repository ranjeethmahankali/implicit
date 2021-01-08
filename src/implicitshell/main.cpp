#include <vector>
#include <algorithm>
#include <iostream>
#include <algorithm>
#include <condition_variable>

#include <assert.h>
#include <implicitlua/luabindings.h>

static void cmd_loop()
{
    std::string input;
    bool running = true;
    while (running)
    {
        std::cout << ARROWS;
        running = !viewer::window_should_close() && !implicit_lua::should_exit() && std::getline(std::cin, input);
        if (input.empty())
            continue;
        implicit_lua::run_cmd(input);
    }
    viewer::close_window();
};

int main(int argc, char** argv)
{
    std::cout << "Initializing OpenGL...\n";
    viewer::init_ogl();
    std::cout << "Initializing OpenCL...\n";
    viewer::init_ocl();
    std::cout << "\tAllocating device buffers\n";
    viewer::init_buffers();
    std::cout << "Initializing Lua bindings...\n";
    implicit_lua::init_lua();
    std::cout << "=====================================\n\n";

    if (argc == 2)
    {
        std::string path(argv[1]);
        std::replace(path.begin(), path.end(), '\\', '/');
        std::string command = "load(\"" + path + "\")";
        implicit_lua::run_cmd(command);
    }
    std::thread cmdThread(cmd_loop);
    viewer::render_loop();

    cmdThread.join();
    viewer::stop();
    implicit_lua::stop();
    return 0;
}