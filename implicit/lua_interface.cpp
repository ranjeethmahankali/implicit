#include "lua_interface.h"
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

void lua_interface::init_functions()
{
    lua_State* L = state();
    LUA_REG_FUNC(L, box);
    LUA_REG_FUNC(L, sphere);
    LUA_REG_FUNC(L, bunion);
    LUA_REG_FUNC(L, bintersection);
    LUA_REG_FUNC(L, bsubtract);

    LUA_REG_FUNC(L, show);
    LUA_REG_FUNC(L, exit);
    LUA_REG_FUNC(L, quit);
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

int lua_interface::bunion(lua_State* L)
{
    op_defn op;
    op.type = op_type::OP_UNION;
    return boolean_operation(L, op);
}

int lua_interface::bintersection(lua_State* L)
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

std::string lua_interface::read_string(lua_State* L, int i)
{
    if (!lua_isstring(L, i))
        luathrow(L, "Cannot readstring");
    std::string ret = lua_tostring(L, i);
    return ret;
}

entities::ent_ref lua_interface::read_entity(lua_State* L, int i)
{
    std::string refstr = read_string(L, i);
    return entities::get_ent_ref(refstr);
}

int lua_interface::boolean_operation(lua_State* L, op_defn op)
{
    using namespace entities;
    int nargs = lua_gettop(L);
    if (nargs != 2)
        luathrow(L, "Boolean operation requires 2 arguments.");

    ent_ref ref1 = read_entity(L, 1);
    ent_ref ref2 = read_entity(L, 2);
    push_entity(L, comp_entity::make_csg(ref1, ref2, op));
    return 1;
}

void lua_interface::push_entity(lua_State* L, entities::ent_ref ref)
{
    std::string refstr = entities::map_ent(ref);
    lua_pushstring(L, refstr.c_str());
}

int lua_interface::show(lua_State* L)
{
    int nargs = lua_gettop(L);
    if (nargs != 1)
        luathrow(L, "Show function expects 1 argument.");
    
    std::string refstr = lua_tostring(L, 1);
    using namespace entities;
    ent_ref ref = get_ent_ref(refstr);
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
    lua_pushstring(L, error.c_str());
    lua_error(L);
}
