#include "lua_interface.h"
#include <fstream>
#define LUA_REG_FUNC(lstate, name) lua_register(lstate, #name, name)
bool s_shouldExit = false;

void lua_interface::init_lua()
{
    if (s_luaState)
        return;
    s_luaState = luaL_newstate();
    luaL_openlibs(s_luaState);
    init_functions();
}

void lua_interface::stop()
{
    lua_State* L = state();
    lua_close(L);
}

void lua_interface::init_functions()
{
    lua_State* L = state();
    LUA_REG_FUNC(L, box);
    LUA_REG_FUNC(L, sphere);
    LUA_REG_FUNC(L, cylinder);
    LUA_REG_FUNC(L, gyroid);
    LUA_REG_FUNC(L, schwarz);
    LUA_REG_FUNC(L, bunion);
    LUA_REG_FUNC(L, bintersect);
    LUA_REG_FUNC(L, bsubtract);
    LUA_REG_FUNC(L, offset);
    LUA_REG_FUNC(L, linblend);

    LUA_REG_FUNC(L, load);

    LUA_REG_FUNC(L, show);
    LUA_REG_FUNC(L, exit);
    LUA_REG_FUNC(L, quit);
}

int lua_interface::delete_entity(lua_State* L)
{
    using namespace entities;
    //std::cout << "Deleting entity in lua ...." << std::endl;
    ent_ref* ref = (ent_ref*)lua_touserdata(L, 1);
    auto oldcount = ref->use_count();
    //std::cout << "This object has " << oldcount << " owners before\n";
    ref->~shared_ptr();
    //if (oldcount > 1) std::cout << "This object has " << ref->use_count() << " owners after\n";
    return 0;
}

void lua_interface::run_cmd(const std::string& line)
{
    lua_State* L = state();
    int ret = luaL_dostring(L, line.c_str());
    if (ret != LUA_OK)
    {
        std::cerr << "Lua Error: " << lua_tostring(L, -1) << std::endl;
    }
}

int lua_interface::box(lua_State* L)
{
    // Get num args.
    int nargs = lua_gettop(L);
    if (nargs != 6)
        luathrow(L, "Box creation requires exactly 6 arguments.");

    float bounds[6];
    for (int i = 1; i <= 6; i++)
    {
        bounds[i - 1] = read_number<float>(L, i);
    }
    using namespace entities;
    push_entity(L, entity::wrap_simple(box3(bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5])));
    return 1;
}

int lua_interface::sphere(lua_State* L)
{
    // Get num args.
    int nargs = lua_gettop(L);
    if (nargs != 4)
        luathrow(L, "Sphere creation requires exactly 4 arguments.");
    float center[3];
    for (int i = 0; i < 3; i++)
    {
        center[i] = read_number<float>(L, i + 1);
    }
    float radius = read_number<float>(L, 4);

    using namespace entities;
    push_entity(L, entity::wrap_simple(sphere3(center[0], center[1], center[2], radius)));
    return 1;
}

int lua_interface::cylinder(lua_State* L)
{
    int nargs = lua_gettop(L);
    if (nargs != 7)
        luathrow(L, "Cylinder creation requires exactly 7 arguments.");
    float p1[3], p2[3], radius;
    for (int i = 0; i < 3; i++)
        p1[i] = read_number<float>(L, i + 1);
    for (int i = 0; i < 3; i++)
        p2[i] = read_number<float>(L, i + 4);
    radius = read_number<float>(L, 7);
    push_entity(L, entities::entity::wrap_simple(entities::cylinder3(p1[0], p1[1], p1[2], p2[0], p2[1], p2[2], radius)));
    return 1;
}

int lua_interface::gyroid(lua_State* L)
{
    int nargs = lua_gettop(L);
    if (nargs != 2)
        luathrow(L, "Gyroid creation requires exactly 2 arguments.");

    float scale = read_number<float>(L, 1);
    float thickness = read_number<float>(L, 2);
    push_entity(L, entities::entity::wrap_simple(entities::gyroid(scale, thickness)));
    return 1;
}

int lua_interface::schwarz(lua_State* L)
{
    int nargs = lua_gettop(L);
    if (nargs != 2)
        luathrow(L, "Schwarz lattice creation requires exactly 2 arguments.");

    float scale = read_number<float>(L, 1);
    float thickness = read_number<float>(L, 2);
    push_entity(L, entities::entity::wrap_simple(entities::schwarz(scale, thickness)));
    return 1;
}

int lua_interface::bunion(lua_State* L)
{
    op_defn op;
    op.type = op_type::OP_UNION;
    return boolean_operation(L, op);
}

int lua_interface::bintersect(lua_State* L)
{
    op_defn op;
    op.type = op_type::OP_INTERSECTION;
    return boolean_operation(L, op);
}

int lua_interface::bsubtract(lua_State* L)
{
    op_defn op;
    op.type = op_type::OP_SUBTRACTION;
    return boolean_operation(L, op);
}

int lua_interface::offset(lua_State* L)
{
    using namespace entities;
    int nargs = lua_gettop(L);
    if (nargs != 2)
        luathrow(L, "Offset operation takes exactly 2 arguments.");

    ent_ref ref = read_entity(L, 1);
    float dist = read_number<float>(L, 2);
    push_entity(L, comp_entity::make_offset(ref, dist));
    return 1;
}

int lua_interface::linblend(lua_State* L)
{
    using namespace entities;
    int nargs = lua_gettop(L);
    if (nargs != 8)
        luathrow(L, "Linear blend requires exactly 1 filepath argument.");

    ent_ref e1 = read_entity(L, 1);
    ent_ref e2 = read_entity(L, 2);
    float coords[6];
    for (int i = 3; i < 9; i++)
    {
        coords[i - 3] = read_number<float>(L, i);
    }

    push_entity(L, comp_entity::make_linblend(e1, e2, { coords[0], coords[1], coords[2] }, { coords[3], coords[4], coords[5] }));
    return 1;
}

int lua_interface::load(lua_State* L)
{
    using namespace entities;
    int nargs = lua_gettop(L);
    if (nargs != 1)
        luathrow(L, "Loading operation requires exactly 1 filepath argument.");

    std::string filepath = read_string(L, 1);
    std::ifstream f;
    f.open(filepath);
    if (!f.is_open())
    {
        luathrow(L, "Cannot open file");
        return 0;
    }

    std::cout << std::endl;
    std::cout << f.rdbuf();
    std::cout << std::endl << std::endl;
    f.close();

    luaL_dofile(L, filepath.c_str());
    return 0;
}

std::string lua_interface::read_string(lua_State* L, int i)
{
    if (!lua_isstring(L, i))
        luathrow(L, "Cannot readstring");
    std::string ret = lua_tostring(L, i);
    return ret;
}

entities::ent_ref lua_interface::read_entity(lua_State* L, int i)
{
    using namespace entities;
    if (!lua_isuserdata(L, i))
        luathrow(L, "Not an entity...");
    ent_ref ref = *(ent_ref*)lua_touserdata(L, i);
    return ref;
}

int lua_interface::boolean_operation(lua_State* L, op_defn op)
{
    using namespace entities;
    int nargs = lua_gettop(L);
    if (nargs != 2 && nargs != 3)
        luathrow(L, "Boolean either 2 entity args and an optional blend radius arg.");

    ent_ref ref1 = read_entity(L, 1);
    ent_ref ref2 = read_entity(L, 2);
    if (nargs == 3)
        op.data.blend_radius = read_number<float>(L, 3);
    push_entity(L, comp_entity::make_csg(ref1, ref2, op));
    return 1;
}

void lua_interface::push_entity(lua_State* L, const entities::ent_ref& ref)
{
    using namespace entities;
    auto udata = (ent_ref*)lua_newuserdata(L, sizeof(ent_ref)); // Allocate space in lua's memory.
    new (udata) ent_ref(ref); // Copy constrct the reference in the memory allocated above.
    // Make a new table and push the functions to tell lua's GC how to delete this object.
    lua_newtable(L);
    lua_pushstring(L, "__gc");
    lua_pushcfunction(L, delete_entity);;
    lua_settable(L, -3);
    lua_setmetatable(L, -2);
    viewer::show_entity(ref);
}

int lua_interface::show(lua_State* L)
{
    int nargs = lua_gettop(L);
    if (nargs != 1)
        luathrow(L, "Show function expects 1 argument.");

    using namespace entities;
    ent_ref ref = read_entity(L, 1);
    viewer::show_entity(ref);
    return 0;
}

int lua_interface::exit(lua_State* L)
{
    std::cout << "Aborting...\n";
    s_shouldExit = true;
    return 0;
}

int lua_interface::quit(lua_State* L)
{
    return lua_interface::exit(L);
}

lua_State* lua_interface::state()
{
    return s_luaState;
}

bool lua_interface::should_exit()
{
    return s_shouldExit;
}

void lua_interface::luathrow(lua_State* L, const std::string& error)
{
    std::cout << std::endl;
    lua_pushstring(L, error.c_str());
    lua_error(L);
    std::cout << std::endl << std::endl;
}
