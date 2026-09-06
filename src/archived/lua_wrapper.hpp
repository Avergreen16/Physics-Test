#include <string>
#include <tuple>
#include <iostream>
#include <functional>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

namespace axiom::lua {

template<typename type>
type get_arg(lua_State* L, int index);

template<>
int get_arg<int>(lua_State* L, int index) {
    return (int)lua_tonumber(L, index);
}

template<>
float get_arg<float>(lua_State* L, int index) {
    return (float)lua_tonumber(L, index);
}

template<>
double get_arg<double>(lua_State* L, int index) {
    return (double)lua_tonumber(L, index);
}

template<>
bool get_arg<bool>(lua_State* L, int index) {
    return (bool)lua_toboolean(L, index);
}

template<>
std::string get_arg<std::string>(lua_State* L, int index) {
    return (std::string)lua_tostring(L, index);
}

//

template<typename type>
void push_arg(lua_State* L, type arg);

template<>
void push_arg<int>(lua_State* L, int arg) {
    lua_pushnumber(L, arg);
}

template<>
void push_arg<float>(lua_State* L, float arg) {
    lua_pushnumber(L, arg);
}

template<>
void push_arg<double>(lua_State* L, double arg) {
    lua_pushnumber(L, arg);
}

template<>
void push_arg<bool>(lua_State* L, bool arg) {
    lua_pushnumber(L, arg);
}

template<>
void push_arg<std::string>(lua_State* L, std::string arg) {
    lua_pushstring(L, arg.c_str());
}


//

template<typename... args, size_t... i>
std::tuple<args...> get_args(lua_State* L, std::index_sequence<i...>) {
    return std::tuple{
        get_arg<args>(L, int(i + 1))...
    };
}

//

template<typename type>
void push_value(lua_State* L, type v);


template<>
void push_value(lua_State* L, int value) {
    lua_pushinteger(L, value);
}

template<>
void push_value(lua_State* L, float value) {
    lua_pushnumber(L, value);
}

template<>
void push_value(lua_State* L, double value) {
    lua_pushnumber(L, value);
}

template<>
void push_value(lua_State* L, bool value) {
    lua_pushboolean(L, value);
}

template<>
void push_value(lua_State* L, std::string value) {
    lua_pushstring(L, value.c_str());
}

//

template<typename ret, typename... args>
void register_function(lua_State* L, std::string name, std::function<ret(args...)>* func) {
    constexpr size_t count = sizeof...(args);

    lua_pushlightuserdata(L, (void*)func);

    lua_CFunction f = [](lua_State* L) {
        std::function<ret(args...)>* function = static_cast<std::function<ret(args...)>*>(lua_touserdata(L, lua_upvalueindex(1)));

        std::tuple<args...> arguments = get_args<args...>(L, std::index_sequence_for<args...>{});

        ret rv = std::apply(*function, arguments);

        push_arg<ret>(L, rv);

        return 1;
    };

    lua_pushcclosure(L, f, 1);
    lua_setglobal(L, name.c_str());
}

bool check_lua(lua_State* L, int r) {
    if(r != LUA_OK) {
        std::string errormsg = lua_tostring(L, -1);
        std::cout << "ERROR: " << errormsg << std::endl;
    }

    return r == LUA_OK;
}

lua_State* lua_init() {
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    return L;
}

void do_file(lua_State* L, std::string filepath) {
    int r = luaL_dofile(L, filepath.c_str());

    check_lua(L, r);
}

template<typename type, typename... type_args>
type call_function(lua_State* L, std::string function, type_args... args) {
    push_value<type_args...>(L, args...);

    int r = lua_pcall(L, sizeof...(args), 1, 0);

    if(r != LUA_OK) {
        std::cout << "LUA ERROR - FUNCTION (" << function << ") : " << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1);
        return;
    }

    type ret = get_arg<type>(L, -1);
    lua_pop(L, 1);

    return ret;
}

}