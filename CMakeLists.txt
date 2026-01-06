cmake_minimum_required(VERSION 3.15)
project(OnnxLua)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LUAJIT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/LuaJIT-2.1.0-beta3")
include_directories("${LUAJIT_DIR}/src")

set(SOL2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/sol2")
include_directories("${SOL2_DIR}/include")

set(ONNXRUNTIME_VERSION "1.17.1")
set(ONNXRUNTIME_DIR "${CMAKE_CURRENT_SOURCE_DIR}/onnxruntime")

include_directories("${ONNXRUNTIME_DIR}/include")

find_library(ONNXRUNTIME_LIB
    NAMES onnxruntime
    PATHS "${ONNXRUNTIME_DIR}/lib"
    NO_DEFAULT_PATH
)

add_library(onnx_lua STATIC
    src/onnx_lua.cpp
)

target_include_directories(onnx_lua PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${LUAJIT_DIR}/src"
    "${SOL2_DIR}/include"
    "${ONNXRUNTIME_DIR}/include"
)

target_link_libraries(onnx_lua
    ${ONNXRUNTIME_LIB}
)

if(WIN32)
    target_link_libraries(onnx_lua
        winmm
        ws2_32
    )
endif()

add_executable(onnx_lua_test
    src/main.cpp
    src/onnx_lua.cpp
)

target_include_directories(onnx_lua_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${LUAJIT_DIR}/src"
    "${SOL2_DIR}/include"
    "${ONNXRUNTIME_DIR}/include"
)

target_link_libraries(onnx_lua_test
    ${ONNXRUNTIME_LIB}
    "${LUAJIT_DIR}/src/lua51.lib"
)

if(WIN32)
    target_link_libraries(onnx_lua_test
        winmm
        ws2_32
    )
endif()
