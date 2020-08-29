#include <fstream>
#include "lua_interface.h"
#include "map_macro.h"
#define LUA_REG_FUNC(lstate, name) lua_register(lstate, #name, name)

#define _LUA_ARG_TYPE(type, name) type
#define LUA_ARG_TYPE(arg_tuple) _LUA_ARG_TYPE##arg_tuple

#define _LUA_ARG_DECL(type, name) type name
#define LUA_ARG_DECL(arg_tuple) _LUA_ARG_DECL##arg_tuple

// Function name macro for logging purposes.
#ifndef __FUNCTION_NAME__
#ifdef _MSC_VER //WINDOWS
#define __FUNCTION_NAME__   __FUNCTION__
#else
#error Define the function name macro for non windows platform.
#endif
#endif

#define CHECK_NUM_ARGS(argcount, expected, lstate) if (argcount != expected){\
std::cerr << "The function '" << __FUNCTION_NAME__ << "' expects " << expected << " arguments.\n";\
luathrow(lstate, "Wrong number of arguments for the function.");}

bool s_shouldExit = false;

template <>
float lua_interface::read_lua<float>(lua_State * L, int i)
{
    if (!lua_isnumber(L, i))
        luathrow(L, "Cannot convert to a number");
    return (float)lua_tonumber(L, i);
}

template <>
double lua_interface::read_lua<double>(lua_State * L, int i)
{
    if (!lua_isnumber(L, i))
        luathrow(L, "Cannot convert to a number");
    return (double)lua_tonumber(L, i);
}

template <>
int lua_interface::read_lua<int>(lua_State * L, int i)
{
    if (!lua_isnumber(L, i))
        luathrow(L, "Cannot convert to a number");
    return (int)lua_tonumber(L, i);
}

template <>
std::string lua_interface::read_lua<std::string>(lua_State * L, int i)
{
    if (!lua_isstring(L, i))
        luathrow(L, "Cannot readstring");
    std::string ret = lua_tostring(L, i);
    return ret;
}

template <>
entities::ent_ref lua_interface::read_lua<entities::ent_ref>(lua_State * L, int i)
{
    using namespace entities;
    if (!lua_isuserdata(L, i))
        luathrow(L, "Not an entity...");
    ent_ref ref = *(ent_ref*)lua_touserdata(L, i);
    return ref;
}

template <>
void lua_interface::push_lua<entities::ent_ref>(lua_State * L, const entities::ent_ref & ref)
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

#define LUA_FUNC(TReturn, FuncName, ...) \
TReturn lua_fn_##FuncName(MAP_LIST(LUA_ARG_TYPE, __VA_ARGS__));\
int lua_c_fn_##FuncName(lua_State* L){\
return lua_interface::lua_func<TReturn, MAP_LIST(LUA_ARG_TYPE, __VA_ARGS__)>::call_func(lua_fn_##FuncName, #FuncName, L);\
}\
TReturn lua_fn_##FuncName(MAP_LIST(LUA_ARG_DECL, __VA_ARGS__))

#define INIT_LUA_FUNC(lstate, name) lua_register(lstate, #name, lua_c_fn_##name)

using namespace entities;

LUA_FUNC(void, show, (ent_ref, ref))
{
    viewer::show_entity(ref);
}

LUA_FUNC(ent_ref, box, (float, xmin), (float, ymin), (float, zmin), (float, xmax), (float, ymax), (float, zmax))
{
    return entity::wrap_simple(box3(xmin, ymin, zmin, xmax, ymax, zmax));
}

void lua_interface::init_functions()
{
    lua_State* L = state();
    INIT_LUA_FUNC(L, show);
    INIT_LUA_FUNC(L, box);
    LUA_REG_FUNC(L, sphere);
    LUA_REG_FUNC(L, cylinder);
    LUA_REG_FUNC(L, halfspace);
    LUA_REG_FUNC(L, gyroid);
    LUA_REG_FUNC(L, schwarz);
    LUA_REG_FUNC(L, bunion);
    LUA_REG_FUNC(L, bintersect);
    LUA_REG_FUNC(L, bsubtract);
    LUA_REG_FUNC(L, offset);
    LUA_REG_FUNC(L, linblend);
    LUA_REG_FUNC(L, smoothblend);

    LUA_REG_FUNC(L, load);

    LUA_REG_FUNC(L, exit);
    LUA_REG_FUNC(L, quit);

#ifdef CLDEBUG
    LUA_REG_FUNC(L, viewer_debugmode);
    LUA_REG_FUNC(L, viewer_debugstep);
#endif // CLDEBUG

    LUA_REG_FUNC(L, exportframe);
    LUA_REG_FUNC(L, setbounds);
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

int lua_interface::sphere(lua_State* L)
{
    // Get num args.
    int nargs = lua_gettop(L);
    CHECK_NUM_ARGS(nargs, 4, L);
    
    float center[3];
    for (int i = 0; i < 3; i++)
    {
        center[i] = read_lua<float>(L, i + 1);
    }
    float radius = read_lua<float>(L, 4);

    using namespace entities;
    push_lua(L, entity::wrap_simple(sphere3(center[0], center[1], center[2], radius)));
    return 1;
}

int lua_interface::cylinder(lua_State* L)
{
    int nargs = lua_gettop(L);
    CHECK_NUM_ARGS(nargs, 7, L);

    float p1[3], p2[3], radius;
    for (int i = 0; i < 3; i++)
        p1[i] = read_lua<float>(L, i + 1);
    for (int i = 0; i < 3; i++)
        p2[i] = read_lua<float>(L, i + 4);
    radius = read_lua<float>(L, 7);
    push_lua(L, entities::entity::wrap_simple(entities::cylinder3(p1[0], p1[1], p1[2], p2[0], p2[1], p2[2], radius)));
    return 1;
}

int lua_interface::halfspace(lua_State* L)
{
    int nargs = lua_gettop(L);
    CHECK_NUM_ARGS(nargs, 6, L);

    float coords[6];
    for (int i = 0; i < 6; i++)
    {
        coords[i] = read_lua<float>(L, i + 1);
    }

    push_lua(L, entities::entity::wrap_simple(entities::halfspace({ coords[0], coords[1], coords[2] }, { coords[3], coords[4], coords[5] })));
    return 1;
}

int lua_interface::gyroid(lua_State* L)
{
    int nargs = lua_gettop(L);
    CHECK_NUM_ARGS(nargs, 2, L);

    float scale = read_lua<float>(L, 1);
    float thickness = read_lua<float>(L, 2);
    push_lua(L, entities::entity::wrap_simple(entities::gyroid(scale, thickness)));
    return 1;
}

int lua_interface::schwarz(lua_State* L)
{
    int nargs = lua_gettop(L);
    CHECK_NUM_ARGS(nargs, 2, L);

    float scale = read_lua<float>(L, 1);
    float thickness = read_lua<float>(L, 2);
    push_lua(L, entities::entity::wrap_simple(entities::schwarz(scale, thickness)));
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
    CHECK_NUM_ARGS(nargs, 2, L);

    ent_ref ref = read_lua<ent_ref>(L, 1);
    float dist = read_lua<float>(L, 2);
    push_lua(L, comp_entity::make_offset(ref, dist));
    return 1;
}

int lua_interface::linblend(lua_State* L)
{
    using namespace entities;
    int nargs = lua_gettop(L);
    CHECK_NUM_ARGS(nargs, 8, L);

    ent_ref e1 = read_lua<ent_ref>(L, 1);
    ent_ref e2 = read_lua<ent_ref>(L, 2);
    float coords[6];
    for (int i = 3; i < 9; i++)
    {
        coords[i - 3] = read_lua<float>(L, i);
    }

    push_lua(L, comp_entity::make_linblend(e1, e2, { coords[0], coords[1], coords[2] }, { coords[3], coords[4], coords[5] }));
    return 1;
}

int lua_interface::smoothblend(lua_State* L)
{
    using namespace entities;
    int nargs = lua_gettop(L);
    CHECK_NUM_ARGS(nargs, 8, L);

    ent_ref e1 = read_lua<ent_ref>(L, 1);
    ent_ref e2 = read_lua<ent_ref>(L, 2);
    float coords[6];
    for (int i = 3; i < 9; i++)
    {
        coords[i - 3] = read_lua<float>(L, i);
    }

    push_lua(L, comp_entity::make_smoothblend(e1, e2, { coords[0], coords[1], coords[2] }, { coords[3], coords[4], coords[5] }));
    return 1;
}

int lua_interface::load(lua_State* L)
{
    using namespace entities;
    int nargs = lua_gettop(L);
    CHECK_NUM_ARGS(nargs, 1, L);

    std::string filepath = read_lua<std::string>(L, 1);
    std::ifstream f;
    f.open(filepath);
    if (!f.is_open())
    {
        luathrow(L, "Cannot open file");
        return 0;
    }

    std::cout << std::endl;
    std::cout << "Parsing file: " << filepath << std::endl;
    std::cout << std::endl;
    std::cout << f.rdbuf();
    std::cout << std::endl << std::endl;
    f.close();

    luaL_dofile(L, filepath.c_str());
    return 0;
}

int lua_interface::boolean_operation(lua_State* L, op_defn op)
{
    using namespace entities;
    int nargs = lua_gettop(L);
    if (nargs != 2 && nargs != 3)
        luathrow(L, "Boolean either 2 entity args and an optional blend radius arg.");

    ent_ref ref1 = read_lua<ent_ref>(L, 1);
    ent_ref ref2 = read_lua<ent_ref>(L, 2);
    if (nargs == 3)
        op.data.blend_radius = read_lua<float>(L, 3);
    push_lua(L, comp_entity::make_csg(ref1, ref2, op));
    return 1;
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

#ifdef CLDEBUG
int lua_interface::viewer_debugmode(lua_State* L)
{
    int nargs = lua_gettop(L);
    CHECK_NUM_ARGS(nargs, 1, L);
    int arg = read_lua<int>(L, 1);
    if (arg != 0 && arg != 1)
        luathrow(L, "Argument must be either 0 or 1.");
    viewer::setdebugmode(arg == 1 ? true : false);
    return 0;
}

int lua_interface::viewer_debugstep(lua_State* L)
{
    int nargs = lua_gettop(L);
    CHECK_NUM_ARGS(nargs, 0, L);
    viewer::debugstep();
    return 0;
}
#endif // CLDEBUG

int lua_interface::exportframe(lua_State* L)
{
    int nargs = lua_gettop(L);
    CHECK_NUM_ARGS(nargs, 1, L);
    std::string filepath = read_lua<std::string>(L, 1);
    if (!viewer::exportframe(filepath))
        luathrow(L, "Failed to export the frame.");
    std::cout << "Frame was exported.\n";
    return 0;
}

int lua_interface::setbounds(lua_State* L)
{
    int nargs = lua_gettop(L);
    CHECK_NUM_ARGS(nargs, 6, L);
    float bounds[6];
    for (int i = 0; i < 6; i++)
    {
        bounds[i] = read_lua<float>(L, i + 1);
    }
    viewer::setbounds(bounds);
    return 0;
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
