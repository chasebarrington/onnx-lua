#include "onnx_lua.h"
#include <iostream>

int main(int argc, char* argv[]) {
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    lua_pushcfunction(L, OnnxLua::luaopen_onnx);
    lua_call(L, 0, 1);
    lua_setglobal(L, "onnx");

    if (argc > 1) {
        if (luaL_dofile(L, argv[1]) != LUA_OK) {
            std::cerr << "Error: " << lua_tostring(L, -1) << std::endl;
            lua_close(L);
            return 1;
        }
    } else {
        std::cout << "usage: " << argv[0] << " <lua_script.lua>" << std::endl;
        std::cout << "onnx lib loaded." << std::endl;
    }

    lua_close(L);
    return 0;
}
