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
            
            try
            {
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
            catch (const char* msg)
            {
                luathrow(L, msg);
                return 0;
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
        std::string type;
        std::string name;
        std::string desc;
    };

    struct func_info
    {
        std::string type;
        std::string name;
        std::string desc;
        std::vector<member_info> arguments;
        
        func_info(const std::string& t, const std::string& n, const std::string& d, const std::vector<member_info>& args);

        void show_help(bool detailed) const;
    };

    static void init_functions();
}