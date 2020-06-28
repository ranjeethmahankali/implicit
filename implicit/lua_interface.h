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

constexpr char ARROWS[] = ">>> ";

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

    template <typename T> T read_number(lua_State* L, int i)
    {
        if (!lua_isnumber(L, i))
            luathrow(L, "Cannot convert to a number");
        return (T)lua_tonumber(L, i);
    };
    static std::string read_string(lua_State* L, int i);
    static entities::ent_ref read_entity(lua_State* L, int i);
    static int boolean_operation(lua_State* L, op_defn op);

    void push_entity(lua_State* L, const entities::ent_ref& ref);

    int show(lua_State* L);
    int box(lua_State* L);
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

    int exit(lua_State* L);
    int quit(lua_State* L);
}