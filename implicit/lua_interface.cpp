#include "lua_interface.h"
#define STRINGIFY(name) #name

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
    lua_register(L, "box", make_box);
    lua_register(L, STRINGIFY(show), show);
}

void lua_interface::run_cmd(const std::string& line)
{
    int ret = luaL_dostring(s_luaState, line.c_str());
    if (ret != LUA_OK)
    {
        std::cerr << "Lua Error: " << lua_tostring(state(), -1) << std::endl;
    }
}

int lua_interface::make_box(lua_State* L)
{
    // Get num args.
    int nargs = lua_gettop(L);
    if (nargs != 6)
        luathrow(L, "Box creation requires exactly 6 arguments.");
    
    float bounds[6];
    for (int i = 1; i <= 6; i++)
    {
        if (!lua_isnumber(L, i))
            luathrow(L, "Cannot convert argument to number.");
        bounds[i - 1] = (float)lua_tonumber(L, i);
    }
    using namespace entities;
    ent_ref ref = entity::wrap_simple(box3(
        bounds[0],
        bounds[1],
        bounds[2],
        bounds[3],
        bounds[4],
        bounds[5]
    ));
    std::string refstr = ent_ref_str(ref);
    // Push the return value.
    lua_pushstring(L, refstr.c_str());
    // Return the number of return values.
    return 1;
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

lua_State* lua_interface::state()
{
    return s_luaState;
}

void lua_interface::luathrow(lua_State* L, const std::string& error)
{
    lua_pushstring(L, error.c_str());
    lua_error(L);
}
