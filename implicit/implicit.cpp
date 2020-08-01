#define NOMINMAX
#include <vector>
#include <algorithm>
#include <iostream>
#include <algorithm>
#include <condition_variable>
#include "viewer.h"

#include <assert.h>
#include "lua_interface.h"

static entities::ent_ref test_heavy_part()
{
    using namespace entities;
    float a1 = 2.04f, a2 = 2.0f;
    ent_ref inner = comp_entity::make_csg(new box3(-a1, -a1, -a1, a1, a1, a1),
        comp_entity::make_csg(
            comp_entity::make_csg(
                comp_entity::make_csg(
                    comp_entity::make_csg(
                        comp_entity::make_csg(
                            comp_entity::make_csg(
                                comp_entity::make_csg(
                                    new sphere3(a2, a2, a2, 1.8f),
                                    new sphere3(a2, -a2, a2, 1.8f), op_type::OP_UNION),
                                new sphere3(-a2, -a2, a2, 1.8f), op_type::OP_UNION),
                            new sphere3(-a2, a2, a2, 1.8f), op_type::OP_UNION),
                        new sphere3(-a2, -a2, -a2, 1.8f), op_type::OP_UNION),
                    new sphere3(a2, -a2, -a2, 1.8f), op_type::OP_UNION),
                new sphere3(a2, a2, -a2, 1.8f), op_type::OP_UNION),
            new sphere3(-a2, a2, -a2, 1.8f), op_type::OP_UNION), OP_SUBTRACTION);

    ent_ref outer = comp_entity::make_offset<ent_ref>(inner, 0.04f);

    ent_ref pattern = comp_entity::make_csg(
        outer,
        new gyroid(10.0f, 0.2f), op_type::OP_INTERSECTION);

    //return pattern;
    return comp_entity::make_csg(inner, pattern, op_type::OP_UNION);
}

static entities::ent_ref test_offset()
{
    using namespace entities;
    float s = 1.0f;
    return ent_ref(comp_entity::make_offset<box3*>(new box3(-s, -s, -s, s, s, s), 0.2f));
};

static entities::ent_ref test_cylinder()
{
    using namespace entities;
    return entity::wrap_simple(cylinder3(0.0f, 0.0f, -3.0f, 0.0f, 0.0f, 3.0f, 3.0f));
};

static void cmd_loop()
{
    std::string input;
    std::cout << ARROWS;
    while (!viewer::window_should_close() && !lua_interface::should_exit() && std::getline(std::cin, input))
    {
        if (input.empty())
            continue;
        lua_interface::run_cmd(input);
        std::cout << ARROWS;
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