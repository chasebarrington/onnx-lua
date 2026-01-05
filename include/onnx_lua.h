#pragma once

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include <onnxruntime_cxx_api.h>
#include <memory>
#include <vector>
#include <string>

namespace OnnxLua {

class OnnxSession {
public:
    OnnxSession(const std::string& model_path);
    ~OnnxSession();

    std::vector<std::vector<float>> run(
        const std::vector<std::vector<float>>& inputs,
        const std::vector<std::vector<int64_t>>& input_shapes
    );

    std::vector<std::string> getInputNames() const;
    std::vector<std::string> getOutputNames() const;
    std::vector<std::vector<int64_t>> getInputShapes() const;
    std::vector<std::vector<int64_t>> getOutputShapes() const;

private:
    std::unique_ptr<Ort::Env> env_;
    std::unique_ptr<Ort::Session> session_;
    std::unique_ptr<Ort::SessionOptions> session_options_;
    Ort::AllocatorWithDefaultOptions allocator_;

    std::vector<std::string> input_names_;
    std::vector<std::string> output_names_;
    std::vector<std::vector<int64_t>> input_shapes_;
    std::vector<std::vector<int64_t>> output_shapes_;
};

extern "C" {
    int luaopen_onnx(lua_State* L);
    int lua_onnx_load_model(lua_State* L);
    int lua_onnx_run_inference(lua_State* L);
    int lua_onnx_get_input_names(lua_State* L);
    int lua_onnx_get_output_names(lua_State* L);
    int lua_onnx_get_input_shapes(lua_State* L);
    int lua_onnx_get_output_shapes(lua_State* L);
    int lua_onnx_free_model(lua_State* L);
}

std::vector<float> lua_table_to_float_vector(lua_State* L, int idx);
std::vector<int64_t> lua_table_to_int64_vector(lua_State* L, int idx);
void push_float_vector_to_lua(lua_State* L, const std::vector<float>& vec);
void push_int64_vector_to_lua(lua_State* L, const std::vector<int64_t>& vec);

}
