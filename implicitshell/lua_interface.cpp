#include <fstream>
#include "lua_interface.h"
#include "map_macro.h"
#define LUA_REG_FUNC(lstate, name) lua_register(lstate, #name, name)

// Function name macro for logging purposes.
#ifndef __FUNCTION_NAME__
#ifdef _MSC_VER //WINDOWS
#define __FUNCTION_NAME__   __FUNCTION__
#else
#error Define the function name macro for non windows platform.
#endif
#endif

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

lua_interface::func_info::func_info(const std::string& t, const std::string& n, const std::string& d, const std::vector<member_info>& args) :
    type(t),
    name(n),
    desc(d),
    arguments(args)
{
}

void lua_interface::func_info::show_help(bool detailed) const
{
    std::cout << std::endl;
    std::cout << type << " " << name << "(...)" << "\n\t" << desc << std::endl;
    if (detailed)
    {
        std::cout << "\tArguments:\n";
        for (const member_info& arg : arguments)
        {
            std::cout << "\t\t" << arg.type << " " << arg.name << ": " << arg.desc << std::endl;
        }
        std::cout << std::endl;
    }
}

static std::unordered_map<std::string, lua_interface::func_info> s_functionInfos;

#define _LUA_ARG_TYPE(type, name, desc) type
#define LUA_ARG_TYPE(arg_tuple) _LUA_ARG_TYPE##arg_tuple

#define _LUA_ARG_DECL(type, name, desc) type name
#define LUA_ARG_DECL(arg_tuple) _LUA_ARG_DECL##arg_tuple

#define _ARG_INFO_INIT(type, name, desc) {#type, #name, desc}
#define ARG_INFO_INIT(arg_tuple) _ARG_INFO_INIT##arg_tuple

#ifdef LUA_FUNC
#error "The macro is already defined."
#endif

#define LUA_FUNC(TReturn, FuncName, HasArgs, FuncDesc, ...) \
namespace lua_interface{\
/*Declare the function without definition.*/\
TReturn lua_fn_##FuncName(MAP_LIST_COND(HasArgs, LUA_ARG_TYPE, __VA_ARGS__));\
/*Define the C function that calls the above function with the correct parameters read from the lua state.*/\
int lua_c_fn_##FuncName(lua_State* L){\
return lua_interface::lua_func<TReturn COND_COMMA(HasArgs) MAP_LIST_COND(HasArgs, LUA_ARG_TYPE, __VA_ARGS__)>::call_func(lua_fn_##FuncName, #FuncName, L);\
}\
/*Define the function that registers this the above lua C function and all the help information.*/\
void lua_init_fn_##FuncName(lua_State* L){\
std::vector<member_info> argsVec = {MAP_LIST_COND(HasArgs, ARG_INFO_INIT, __VA_ARGS__)};\
s_functionInfos.emplace(#FuncName, func_info(#TReturn, #FuncName, FuncDesc, argsVec));\
lua_register(L, #FuncName, lua_c_fn_##FuncName);}\
}\
/*The sigature of the function declared at the beginning, so that the user of the macro can write the definition.*/\
TReturn lua_interface::lua_fn_##FuncName(MAP_LIST_COND(HasArgs, LUA_ARG_DECL, __VA_ARGS__))

#define INIT_LUA_FUNC(lstate, name) lua_init_fn_##name(lstate);

using namespace entities;

LUA_FUNC(void, quit, false, "Aborts the application.")
{
    std::cout << "Aborting...\n";
    s_shouldExit = true;
}

LUA_FUNC(void, show, true, "Shows the given entity in the viewer",
    (ent_ref, ent, "The entity to be displayed"))
{
    viewer::show_entity(ent);
}

LUA_FUNC(ent_ref, box, true, "Creates and returns a box entity",
    (float, xcenter, "The x coordinate of the center of the box."),
    (float, ycenter, "The y coordinate of the center of the box."),
    (float, zcenter, "The z coordinate of the center of the box."),
    (float, xhalf, "Half-size of the box in the x direction"),
    (float, yhalf, "Half-size of the box in the y direction"),
    (float, zhalf, "Half-size of the box in the z direction"))
{
    return entity::wrap_simple(box3(xcenter, ycenter, zcenter, xhalf, yhalf, zhalf));
}

LUA_FUNC(ent_ref, sphere, true, "Creates a sphere",
    (float, xcenter, "The x coordinate of the center"),
    (float, ycenter, "The y coordinate of the center"),
    (float, zcenter, "The z coordinate of the center"),
    (float, radius, "The radius of the sphere"))
{
    return entity::wrap_simple(sphere3(xcenter, ycenter, zcenter, radius));
}

LUA_FUNC(ent_ref, cylinder, true, "Creates a cylinder",
    (float, xstart, "The x coordinate of the start of the cylinder"),
    (float, ystart, "The y coordinate of the start of the cylinder"),
    (float, zstart, "The z coordinate of the start of the cylinder"),
    (float, xend, "The x coordinate of the end of the cylinder"),
    (float, yend, "The y coordinate of the end of the cylinder"),
    (float, zend, "The z coordinate of the end of the cylinder"),
    (float, radius, "The radius of the cylinder"))
{
    return entities::entity::wrap_simple(entities::cylinder3(xstart, ystart, zstart, xend, yend, zend, radius));
}

LUA_FUNC(ent_ref, halfspace, true, "Creates a halfspace defined by a plane",
    (float, xorigin, "The x coordinate of the origin of the plane"),
    (float, yorigin, "The y coordinate of the origin of the plane"),
    (float, zorigin, "The z coordinate of the origin of the plane"),
    (float, xnormal, "The x coordinate of the normal of the plane"),
    (float, ynormal, "The y coordinate of the normal of the plane"),
    (float, znormal, "The z coordinate of the normal of the plane"))
{
    return entities::entity::wrap_simple(entities::halfspace({ xorigin, yorigin, zorigin }, { xnormal, ynormal, znormal }));
}

LUA_FUNC(ent_ref, gyroid, true, "Creates a gyroid lattice",
    (float, scale, "The scale of the lattice"),
    (float, thickness, "The wall thickness"))
{
    return entities::entity::wrap_simple(entities::gyroid(scale, thickness));
}

LUA_FUNC(ent_ref, schwarz, true, "Creates a schwarz lattice",
    (float, scale, "The scale of the lattice"),
    (float, thickness, "The wall thickness"))
{
    return entities::entity::wrap_simple(entities::schwarz(scale, thickness));
}

LUA_FUNC(ent_ref, bunion, true, "Creates a boolean union of the given entities",
    (ent_ref, first, "First entity"),
    (ent_ref, second, "Second entity"))
{
    op_defn op;
    op.data.blend_radius = 0.0f;
    op.type = op_type::OP_UNION;
    return comp_entity::make_csg(first, second, op);
}

LUA_FUNC(ent_ref, bintersect, true, "Creates a boolean intersection of the given entities",
    (ent_ref, first, "First entity"),
    (ent_ref, second, "Second entity"))
{
    op_defn op;
    op.data.blend_radius = 0.0f;
    op.type = op_type::OP_INTERSECTION;
    return comp_entity::make_csg(first, second, op);
}

LUA_FUNC(ent_ref, bsubtract, true, "Creates a boolean difference of the given entities",
    (ent_ref, first, "First entity, to be subtracted from"),
    (ent_ref, second, "Second entity, to be subtracted"))
{
    op_defn op;
    op.data.blend_radius = 0.0f;
    op.type = op_type::OP_SUBTRACTION;
    return comp_entity::make_csg(first, second, op);
}

LUA_FUNC(ent_ref, offset, true, "Creates an entity that is offset from the given entity",
    (ent_ref, ent, "Entity to be offset"),
    (float, dist, "Offset distance"))
{
    return comp_entity::make_offset(ent, dist);
}

LUA_FUNC(ent_ref, linblend, true, "Creates a linear blend by interpolating the two entities between the two points",
    (ent_ref, first, "First entity for the blend"),
    (ent_ref, second, "Second entity for the blend"),
    (float, xfirst, "The x coordinate of the first point for interpolation"),
    (float, yfirst, "The y coordinate of the first point for interpolation"),
    (float, zfirst, "The z coordinate of the first point for interpolation"),
    (float, xsecond, "The x coordinate of the first point for interpolation"),
    (float, ysecond, "The y coordinate of the first point for interpolation"),
    (float, zsecond, "The z coordinate of the first point for interpolation"))
{
    return comp_entity::make_linblend(first, second, { xfirst, yfirst, zfirst }, { xsecond, ysecond, zsecond });
}

LUA_FUNC(ent_ref, smoothblend, true, "Creates a smooth blend by interpolating (with an s-function) the two entities between the two points",
    (ent_ref, first, "First entity for the blend"),
    (ent_ref, second, "Second entity for the blend"),
    (float, xfirst, "The x coordinate of the first point for interpolation"),
    (float, yfirst, "The y coordinate of the first point for interpolation"),
    (float, zfirst, "The z coordinate of the first point for interpolation"),
    (float, xsecond, "The x coordinate of the first point for interpolation"),
    (float, ysecond, "The y coordinate of the first point for interpolation"),
    (float, zsecond, "The z coordinate of the first point for interpolation"))
{
    return comp_entity::make_smoothblend(first, second, { xfirst, yfirst, zfirst }, { xsecond, ysecond, zsecond });
}

LUA_FUNC(void, load, true, "Runs a lua script into the current environment",
    (std::string, filepath, "The path to the script file"))
{
    std::ifstream f;
    f.open(filepath);
    if (!f.is_open())
    {
        throw "Cannot open file";
    }

    std::cout << std::endl;
    std::cout << "Parsing file: " << filepath << std::endl;
    std::cout << std::endl;
    std::cout << f.rdbuf();
    std::cout << std::endl << std::endl;
    f.close();

    luaL_dofile(state(), filepath.c_str());
}

#ifdef CLDEBUG
LUA_FUNC(void, viewer_debugmode, true, "Sets the viewer debug mode flag",
    (int, flag, "The flag to be set"))
{
    if (flag != 0 && flag != 1)
        throw "Argument must be either 0 or 1.";
    viewer::setdebugmode(flag == 1 ? true : false);
}

LUA_FUNC(void, viewer_debugstep, false, "Advances the viewer by one frame in the debug mode")
{
    viewer::debugstep();
}
#endif

LUA_FUNC(void, exportframe, true, "Exports the current view as a BMP image",
    (std::string, filepath, "Path of the BMP file to be written"))
{
    if (!viewer::exportframe(filepath))
        throw "Failed to export the frame.";
    std::cout << "Frame was exported.\n";
}

LUA_FUNC(void, setbounds, true, "Sets the bounds, or the build volume for the current environment",
    (float, xmin, "The minimum coordinate of the bounds in the x direction"),
    (float, ymin, "The minimum coordinate of the bounds in the y direction"),
    (float, zmin, "The minimum coordinate of the bounds in the z direction"),
    (float, xmax, "The maximum coordinate of the bounds in the x direction"),
    (float, ymax, "The maximum coordinate of the bounds in the y direction"),
    (float, zmax, "The maximum coordinate of the bounds in the z direction"))
{
    float bounds[6] = { xmin, ymin, zmin, xmax, ymax, zmax };
    viewer::setbounds(bounds);
}

LUA_FUNC(void, help_all, false, "Shows a list of all functions and their descriptions")
{
    for (const auto& info : s_functionInfos)
    {
        info.second.show_help(false);
    }
}

LUA_FUNC(void, help, true, "Shows the detailed help of a single function",
    (std::string, functionName, "The name of the function whose help information is to be shown"))
{
    auto match = s_functionInfos.find(functionName);
    if (match != s_functionInfos.end())
    {
        match->second.show_help(true);
    }
    else
    {
        throw "Cannot find function with that name";
    }
}

LUA_FUNC(ent_ref, filleted_union, true, "Creates a boolean union of the given entities with the meeting edges filleted",
    (ent_ref, first, "The first entity"),
    (ent_ref, second, "The second entity"),
    (float, filletRadius, "The fillet radius"))
{
    op_defn op;
    op.type = op_type::OP_UNION;
    op.data.blend_radius = filletRadius;
    return comp_entity::make_csg(first, second, op);
}

LUA_FUNC(ent_ref, filleted_intersection, true, "Creates a boolean intersection of the given entities with the meeting edges filleted",
    (ent_ref, first, "The first entity"),
    (ent_ref, second, "The second entity"),
    (float, filletRadius, "The fillet radius"))
{
    op_defn op;
    op.type = op_type::OP_INTERSECTION;
    op.data.blend_radius = filletRadius;
    return comp_entity::make_csg(first, second, op);
}

LUA_FUNC(ent_ref, filleted_subtraction, true, "Creates a boolean subtraction of the given entities with the meeting edges filleted",
    (ent_ref, first, "The first entity"),
    (ent_ref, second, "The second entity"),
    (float, filletRadius, "The fillet radius"))
{
    op_defn op;
    op.type = op_type::OP_SUBTRACTION;
    op.data.blend_radius = filletRadius;
    return comp_entity::make_csg(first, second, op);
}

LUA_FUNC(void, adaptive_rendermode, true, "Sets the level of detail for the adaptive rendering mode",
    (int, lod, "Level of detail, must be between 0 and 8"))
{
    if (lod < 0)
    {
        std::cout << "Level of detail cannot be negative. Setting LOD to zero.\n";
        lod = 0;
    }
    else if (lod > 8)
    {
        std::cout << "Value too high. Using 8 as the LOD.\n";
        lod = 8;
    }
    viewer::adaptive_rendermode((uint8_t)lod);
}

void lua_interface::init_functions()
{
    lua_State* L = state();
    INIT_LUA_FUNC(L, quit);
    INIT_LUA_FUNC(L, show);
    INIT_LUA_FUNC(L, box);
    INIT_LUA_FUNC(L, sphere);
    INIT_LUA_FUNC(L, cylinder);
    INIT_LUA_FUNC(L, halfspace);
    INIT_LUA_FUNC(L, gyroid);
    INIT_LUA_FUNC(L, schwarz);
    INIT_LUA_FUNC(L, bunion);
    INIT_LUA_FUNC(L, bintersect);
    INIT_LUA_FUNC(L, bsubtract);
    INIT_LUA_FUNC(L, offset);
    INIT_LUA_FUNC(L, linblend);
    INIT_LUA_FUNC(L, smoothblend);
    INIT_LUA_FUNC(L, load);

#ifdef CLDEBUG
    INIT_LUA_FUNC(L, viewer_debugmode);
    INIT_LUA_FUNC(L, viewer_debugstep);
#endif // CLDEBUG

    INIT_LUA_FUNC(L, exportframe);
    INIT_LUA_FUNC(L, setbounds);
    INIT_LUA_FUNC(L, help_all);
    INIT_LUA_FUNC(L, help);
    INIT_LUA_FUNC(L, filleted_union);
    INIT_LUA_FUNC(L, filleted_intersection);
    INIT_LUA_FUNC(L, filleted_subtraction);
    INIT_LUA_FUNC(L, adaptive_rendermode);
}
