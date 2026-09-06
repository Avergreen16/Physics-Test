add_library(lua STATIC
    ${CMAKE_SOURCE_DIR}/external/lua/lapi.c
    ${CMAKE_SOURCE_DIR}/external/lua/lauxlib.c
    ${CMAKE_SOURCE_DIR}/external/lua/lbaselib.c
    ${CMAKE_SOURCE_DIR}/external/lua/lcode.c
    ${CMAKE_SOURCE_DIR}/external/lua/lcorolib.c
    ${CMAKE_SOURCE_DIR}/external/lua/lctype.c
    ${CMAKE_SOURCE_DIR}/external/lua/ldblib.c
    ${CMAKE_SOURCE_DIR}/external/lua/ldebug.c
    ${CMAKE_SOURCE_DIR}/external/lua/ldo.c
    ${CMAKE_SOURCE_DIR}/external/lua/ldump.c
    ${CMAKE_SOURCE_DIR}/external/lua/lfunc.c
    ${CMAKE_SOURCE_DIR}/external/lua/lgc.c
    ${CMAKE_SOURCE_DIR}/external/lua/linit.c
    ${CMAKE_SOURCE_DIR}/external/lua/liolib.c
    ${CMAKE_SOURCE_DIR}/external/lua/llex.c
    ${CMAKE_SOURCE_DIR}/external/lua/lmathlib.c
    ${CMAKE_SOURCE_DIR}/external/lua/lmem.c
    ${CMAKE_SOURCE_DIR}/external/lua/loadlib.c
    ${CMAKE_SOURCE_DIR}/external/lua/lobject.c
    ${CMAKE_SOURCE_DIR}/external/lua/lopcodes.c
    ${CMAKE_SOURCE_DIR}/external/lua/loslib.c
    ${CMAKE_SOURCE_DIR}/external/lua/lparser.c
    ${CMAKE_SOURCE_DIR}/external/lua/lstate.c
    ${CMAKE_SOURCE_DIR}/external/lua/lstring.c
    ${CMAKE_SOURCE_DIR}/external/lua/lstrlib.c
    ${CMAKE_SOURCE_DIR}/external/lua/ltable.c
    ${CMAKE_SOURCE_DIR}/external/lua/ltablib.c
    ${CMAKE_SOURCE_DIR}/external/lua/ltests.c
    ${CMAKE_SOURCE_DIR}/external/lua/ltm.c
    ${CMAKE_SOURCE_DIR}/external/lua/lua.c
    ${CMAKE_SOURCE_DIR}/external/lua/lundump.c
    ${CMAKE_SOURCE_DIR}/external/lua/lutf8lib.c
    ${CMAKE_SOURCE_DIR}/external/lua/lvm.c
    ${CMAKE_SOURCE_DIR}/external/lua/lzio.c
)

target_include_directories(lua PUBLIC ${CMAKE_SOURCE_DIR}/external)