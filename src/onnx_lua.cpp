#include "onnx_lua.h"
#include <stdexcept>
#include <cstring>

namespace OnnxLua {

OnnxSession::OnnxSession(const std::string& model_path) {
    env_ = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "OnnxLuaEnv");
    session_options_ = std::make_unique<Ort::SessionOptions>();

    session_options_->SetIntraOpNumThreads(1);
    session_options_->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

#ifdef _WIN32
    std::wstring wide_path(model_path.begin(), model_path.end());
    session_ = std::make_unique<Ort::Session>(*env_, wide_path.c_str(), *session_options_);
#else
    session_ = std::make_unique<Ort::Session>(*env_, model_path.c_str(), *session_options_);
#endif

    size_t num_input_nodes = session_->GetInputCount();
    for (size_t i = 0; i < num_input_nodes; i++) {
        auto input_name = session_->GetInputNameAllocated(i, allocator_);
        input_names_.push_back(input_name.get());

        auto type_info = session_->GetInputTypeInfo(i);
        auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
        input_shapes_.push_back(tensor_info.GetShape());
    }

    size_t num_output_nodes = session_->GetOutputCount();
    for (size_t i = 0; i < num_output_nodes; i++) {
        auto output_name = session_->GetOutputNameAllocated(i, allocator_);
        output_names_.push_back(output_name.get());

        auto type_info = session_->GetOutputTypeInfo(i);
        auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
        output_shapes_.push_back(tensor_info.GetShape());
    }
}

OnnxSession::~OnnxSession() = default;

std::vector<std::vector<float>> OnnxSession::run(
    const std::vector<std::vector<float>>& inputs,
    const std::vector<std::vector<int64_t>>& input_shapes
) {
    std::vector<Ort::Value> input_tensors;
    std::vector<const char*> input_names_cstr;
    std::vector<const char*> output_names_cstr;

    for (const auto& name : input_names_) {
        input_names_cstr.push_back(name.c_str());
    }
    for (const auto& name : output_names_) {
        output_names_cstr.push_back(name.c_str());
    }

    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    for (size_t i = 0; i < inputs.size(); i++) {
        int64_t total_size = 1;
        for (auto dim : input_shapes[i]) {
            total_size *= dim;
        }

        input_tensors.push_back(Ort::Value::CreateTensor<float>(
            memory_info,
            const_cast<float*>(inputs[i].data()),
            total_size,
            input_shapes[i].data(),
            input_shapes[i].size()
        ));
    }

    auto output_tensors = session_->Run(
        Ort::RunOptions{nullptr},
        input_names_cstr.data(),
        input_tensors.data(),
        inputs.size(),
        output_names_cstr.data(),
        output_names_.size()
    );

    std::vector<std::vector<float>> outputs;
    for (auto& tensor : output_tensors) {
        auto tensor_info = tensor.GetTensorTypeAndShapeInfo();
        size_t total_size = tensor_info.GetElementCount();

        float* float_data = tensor.GetTensorMutableData<float>();
        outputs.push_back(std::vector<float>(float_data, float_data + total_size));
    }

    return outputs;
}

std::vector<std::string> OnnxSession::getInputNames() const {
    return input_names_;
}

std::vector<std::string> OnnxSession::getOutputNames() const {
    return output_names_;
}

std::vector<std::vector<int64_t>> OnnxSession::getInputShapes() const {
    return input_shapes_;
}

std::vector<std::vector<int64_t>> OnnxSession::getOutputShapes() const {
    return output_shapes_;
}

std::vector<float> lua_table_to_float_vector(lua_State* L, int idx) {
    std::vector<float> result;

    lua_pushnil(L);
    while (lua_next(L, idx) != 0) {
        if (lua_isnumber(L, -1)) {
            result.push_back(static_cast<float>(lua_tonumber(L, -1)));
        }
        lua_pop(L, 1);
    }

    return result;
}

std::vector<int64_t> lua_table_to_int64_vector(lua_State* L, int idx) {
    std::vector<int64_t> result;

    lua_pushnil(L);
    while (lua_next(L, idx) != 0) {
        if (lua_isnumber(L, -1)) {
            result.push_back(static_cast<int64_t>(lua_tonumber(L, -1)));
        }
        lua_pop(L, 1);
    }

    return result;
}

void push_float_vector_to_lua(lua_State* L, const std::vector<float>& vec) {
    lua_createtable(L, vec.size(), 0);
    for (size_t i = 0; i < vec.size(); i++) {
        lua_pushnumber(L, vec[i]);
        lua_rawseti(L, -2, i + 1);
    }
}

void push_int64_vector_to_lua(lua_State* L, const std::vector<int64_t>& vec) {
    lua_createtable(L, vec.size(), 0);
    for (size_t i = 0; i < vec.size(); i++) {
        lua_pushnumber(L, static_cast<lua_Number>(vec[i]));
        lua_rawseti(L, -2, i + 1);
    }
}

extern "C" {

int lua_onnx_load_model(lua_State* L) {
    const char* model_path = luaL_checkstring(L, 1);

    try {
        OnnxSession* session = new OnnxSession(model_path);

        OnnxSession** udata = (OnnxSession**)lua_newuserdata(L, sizeof(OnnxSession*));
        *udata = session;

        luaL_getmetatable(L, "OnnxSession");
        lua_setmetatable(L, -2);

        return 1;
    } catch (const std::exception& e) {
        lua_pushnil(L);
        lua_pushstring(L, e.what());
        return 2;
    }
}

int lua_onnx_run_inference(lua_State* L) {
    OnnxSession** session = (OnnxSession**)luaL_checkudata(L, 1, "OnnxSession");
    luaL_checktype(L, 2, LUA_TTABLE);
    luaL_checktype(L, 3, LUA_TTABLE);

    try {
        std::vector<std::vector<float>> inputs;
        lua_pushnil(L);
        while (lua_next(L, 2) != 0) {
            if (lua_istable(L, -1)) {
                inputs.push_back(lua_table_to_float_vector(L, lua_gettop(L)));
            }
            lua_pop(L, 1);
        }

        std::vector<std::vector<int64_t>> input_shapes;
        lua_pushnil(L);
        while (lua_next(L, 3) != 0) {
            if (lua_istable(L, -1)) {
                input_shapes.push_back(lua_table_to_int64_vector(L, lua_gettop(L)));
            }
            lua_pop(L, 1);
        }

        auto outputs = (*session)->run(inputs, input_shapes);

        lua_createtable(L, outputs.size(), 0);
        for (size_t i = 0; i < outputs.size(); i++) {
            push_float_vector_to_lua(L, outputs[i]);
            lua_rawseti(L, -2, i + 1);
        }

        return 1;
    } catch (const std::exception& e) {
        lua_pushnil(L);
        lua_pushstring(L, e.what());
        return 2;
    }
}

int lua_onnx_get_input_names(lua_State* L) {
    OnnxSession** session = (OnnxSession**)luaL_checkudata(L, 1, "OnnxSession");

    auto names = (*session)->getInputNames();
    lua_createtable(L, names.size(), 0);
    for (size_t i = 0; i < names.size(); i++) {
        lua_pushstring(L, names[i].c_str());
        lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

int lua_onnx_get_output_names(lua_State* L) {
    OnnxSession** session = (OnnxSession**)luaL_checkudata(L, 1, "OnnxSession");

    auto names = (*session)->getOutputNames();
    lua_createtable(L, names.size(), 0);
    for (size_t i = 0; i < names.size(); i++) {
        lua_pushstring(L, names[i].c_str());
        lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

int lua_onnx_get_input_shapes(lua_State* L) {
    OnnxSession** session = (OnnxSession**)luaL_checkudata(L, 1, "OnnxSession");

    auto shapes = (*session)->getInputShapes();
    lua_createtable(L, shapes.size(), 0);
    for (size_t i = 0; i < shapes.size(); i++) {
        push_int64_vector_to_lua(L, shapes[i]);
        lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

int lua_onnx_get_output_shapes(lua_State* L) {
    OnnxSession** session = (OnnxSession**)luaL_checkudata(L, 1, "OnnxSession");

    auto shapes = (*session)->getOutputShapes();
    lua_createtable(L, shapes.size(), 0);
    for (size_t i = 0; i < shapes.size(); i++) {
        push_int64_vector_to_lua(L, shapes[i]);
        lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

int lua_onnx_free_model(lua_State* L) {
    OnnxSession** session = (OnnxSession**)luaL_checkudata(L, 1, "OnnxSession");
    if (*session) {
        delete *session;
        *session = nullptr;
    }
    return 0;
}

int luaopen_onnx(lua_State* L) {
    luaL_newmetatable(L, "OnnxSession");
    lua_pushstring(L, "__gc");
    lua_pushcfunction(L, lua_onnx_free_model);
    lua_settable(L, -3);
    lua_pop(L, 1);

    static const luaL_Reg onnx_functions[] = {
        {"load_model", lua_onnx_load_model},
        {"run", lua_onnx_run_inference},
        {"get_input_names", lua_onnx_get_input_names},
        {"get_output_names", lua_onnx_get_output_names},
        {"get_input_shapes", lua_onnx_get_input_shapes},
        {"get_output_shapes", lua_onnx_get_output_shapes},
        {NULL, NULL}
    };

    luaL_newlib(L, onnx_functions);

    return 1;
}

}

}
