#pragma once
#include <string>
#include <iostream>
extern "C"
{
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
};
#include "viewer.h"

static lua_State* s_luaState = nullptr;

namespace lua_interface
{
    void init_lua();
    void stop();
    static void init_functions();
    static int delete_entity(lua_State* L);

    lua_State* state();
    bool should_exit();
    void luathrow(lua_State* L, const std::string& error);
    void run_cmd(const std::string& line);

    template <typename T>
    T read_lua(lua_State* L, int i)
    {
        static_assert(false, "Template specialization needed.");
    };

    template <typename T>
    void push_lua(lua_State* L, const T& ref)
    {
        static_assert(false, "Template specialization needed.");
    }

    template <typename TReturn, typename... TArgs>
    struct lua_func
    {
        template <typename FunctionType>
        static int call_func(FunctionType func, const char* functionName, lua_State* L)
        {
            int nargs = lua_gettop(L);
            if (nargs != sizeof...(TArgs))
            {
                std::cerr << "The function '" << functionName << "' expects " << sizeof...(TArgs) << " arguments.\n";
                luathrow(L, "Incorrect number of arguments");
            }
            
            if constexpr (std::is_void<TReturn>::value)
            {
                call_fn_internal(func, L, std::make_integer_sequence<int, sizeof...(TArgs)>());
                return 0;
            }
            else
            {
                push_lua<TReturn>(L, call_fn_internal(func, L, std::make_integer_sequence<int, sizeof...(TArgs)>()));
                return 1;
            }
        }
    private:
        template <typename FunctionType, int... N>
        static TReturn call_fn_internal(FunctionType func, lua_State* L, std::integer_sequence<int, N...>)
        {
            return func(read_lua<TArgs>(L, N + 1)...);
        };
    };

    struct member_info
    {
        const char* type;
        const char* name;
        const char* desc;
    };

    struct func_info
    {
        const char* type;
        const char* name;
        const char* desc;
        std::vector<member_info> arguments;
        
        func_info(const char* t, const char* n, const char* d, const std::vector<member_info>& args);
    };

    static int boolean_operation(lua_State* L, op_defn op);
    int sphere(lua_State* L);
    int cylinder(lua_State* L);
    int halfspace(lua_State* L);
    int gyroid(lua_State* L);
    int schwarz(lua_State* L);
    int bunion(lua_State* L);
    int bintersect(lua_State* L);
    int bsubtract(lua_State* L);
    int offset(lua_State* L);
    int linblend(lua_State* L);
    int smoothblend(lua_State* L);

    int load(lua_State* L);

#ifdef CLDEBUG
    int viewer_debugmode(lua_State* L);
    int viewer_debugstep(lua_State* L);
#endif // CLDEBUG

    int exportframe(lua_State* L);
    int setbounds(lua_State* L);
}