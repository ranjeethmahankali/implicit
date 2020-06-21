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
    void luathrow(lua_State* L, const std::string& error);
    void run_cmd(const std::string& line);

    int make_box(lua_State* L);
    int show(lua_State* L);
}