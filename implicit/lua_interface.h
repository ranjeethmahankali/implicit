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
    static void init_functions();

    lua_State* state();
    bool should_exit();
    void luathrow(lua_State* L, const std::string& error);
    void run_cmd(const std::string& line);

    template <typename T> T read_number(lua_State* L, int i)
    {
        if (!lua_isnumber(L, i))
            luathrow(L, "Cannot convert to a number");
        return (T)lua_tonumber(L, i);
    };
    static std::string read_string(lua_State* L, int i);
    static entities::ent_ref read_entity(lua_State* L, int i);
    static int boolean_operation(lua_State* L, op_defn op);

    void push_entity(lua_State* L, entities::ent_ref ref);

    int show(lua_State* L);
    int box(lua_State* L);
    int sphere(lua_State* L);
    int cylinder(lua_State* L);
    int gyroid(lua_State* L);
    int bunion(lua_State* L);
    int bintersection(lua_State* L);
    int bsubtract(lua_State* L);

    int exit(lua_State* L);
    int quit(lua_State* L);
}