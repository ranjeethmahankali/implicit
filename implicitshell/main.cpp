#define NOMINMAX
#include <vector>
#include <algorithm>
#include <iostream>
#include <algorithm>
#include <condition_variable>

#include <assert.h>
#include "lua_interface.h"
#include "viewer.h"

static void cmd_loop()
{
    std::string input;
    bool running = true;
    while (running)
    {
        std::cout << ARROWS;
        running = !viewer::window_should_close() && !lua_interface::should_exit() && std::getline(std::cin, input);
        if (input.empty())
            continue;
        lua_interface::run_cmd(input);
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
    lua_interface::init_lua();
    std::cout << "=====================================\n\n";

    if (argc == 2)
    {
        std::string path(argv[1]);
        std::string command = "load(\"" + path + "\")";
        lua_interface::run_cmd(command);
    }
    std::thread cmdThread(cmd_loop);
    viewer::render_loop();

    cmdThread.join();
    viewer::stop();
    lua_interface::stop();
    return 0;
}