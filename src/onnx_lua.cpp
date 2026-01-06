#include "onnx_lua.h"
#include <sol/sol.hpp>
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

extern "C" {

int luaopen_onnx(lua_State* L) {
    sol::state_view lua(L);

    auto onnx_module = lua.create_table();

    lua.new_usertype<OnnxSession>("OnnxSession",
        sol::constructors<OnnxSession(const std::string&)>(),
        "run", &OnnxSession::run,
        "getInputNames", &OnnxSession::getInputNames,
        "getOutputNames", &OnnxSession::getOutputNames,
        "getInputShapes", &OnnxSession::getInputShapes,
        "getOutputShapes", &OnnxSession::getOutputShapes
    );

    onnx_module["load_model"] = [](sol::this_state L, const std::string& model_path) -> sol::object {
        sol::state_view lua(L);
        try {
            auto session = std::make_shared<OnnxSession>(model_path);
            return sol::make_object(lua, session);
        } catch (const std::exception& e) {
            return sol::make_object(lua, sol::nil);
        }
    };

    onnx_module["run"] = [](sol::this_state L,
                            std::shared_ptr<OnnxSession> session,
                            const sol::table& inputs_table,
                            const sol::table& shapes_table) -> sol::object {
        sol::state_view lua(L);

        if (!session) {
            return sol::make_object(lua, sol::nil);
        }

        try {
            std::vector<std::vector<float>> inputs;
            for (const auto& pair : inputs_table) {
                if (pair.second.is<sol::table>()) {
                    sol::table input_table = pair.second.as<sol::table>();
                    std::vector<float> input_vec;
                    for (const auto& val_pair : input_table) {
                        if (val_pair.second.is<float>() || val_pair.second.is<double>()) {
                            input_vec.push_back(val_pair.second.as<float>());
                        }
                    }
                    inputs.push_back(input_vec);
                }
            }

            std::vector<std::vector<int64_t>> shapes;
            for (const auto& pair : shapes_table) {
                if (pair.second.is<sol::table>()) {
                    sol::table shape_table = pair.second.as<sol::table>();
                    std::vector<int64_t> shape_vec;
                    for (const auto& val_pair : shape_table) {
                        if (val_pair.second.is<int>() || val_pair.second.is<double>()) {
                            shape_vec.push_back(val_pair.second.as<int64_t>());
                        }
                    }
                    shapes.push_back(shape_vec);
                }
            }

            auto outputs = session->run(inputs, shapes);

            sol::table result = lua.create_table();
            for (size_t i = 0; i < outputs.size(); i++) {
                sol::table output_table = lua.create_table();
                for (size_t j = 0; j < outputs[i].size(); j++) {
                    output_table[j + 1] = outputs[i][j];
                }
                result[i + 1] = output_table;
            }

            return sol::make_object(lua, result);
        } catch (const std::exception& e) {
            return sol::make_object(lua, sol::nil);
        }
    };

    onnx_module["get_input_names"] = [](sol::this_state L, std::shared_ptr<OnnxSession> session) -> sol::object {
        sol::state_view lua(L);
        if (!session) {
            return sol::make_object(lua, sol::nil);
        }

        auto names = session->getInputNames();
        sol::table result = lua.create_table();
        for (size_t i = 0; i < names.size(); i++) {
            result[i + 1] = names[i];
        }
        return sol::make_object(lua, result);
    };

    onnx_module["get_output_names"] = [](sol::this_state L, std::shared_ptr<OnnxSession> session) -> sol::object {
        sol::state_view lua(L);
        if (!session) {
            return sol::make_object(lua, sol::nil);
        }

        auto names = session->getOutputNames();
        sol::table result = lua.create_table();
        for (size_t i = 0; i < names.size(); i++) {
            result[i + 1] = names[i];
        }
        return sol::make_object(lua, result);
    };

    onnx_module["get_input_shapes"] = [](sol::this_state L, std::shared_ptr<OnnxSession> session) -> sol::object {
        sol::state_view lua(L);
        if (!session) {
            return sol::make_object(lua, sol::nil);
        }

        auto shapes = session->getInputShapes();
        sol::table result = lua.create_table();
        for (size_t i = 0; i < shapes.size(); i++) {
            sol::table shape_table = lua.create_table();
            for (size_t j = 0; j < shapes[i].size(); j++) {
                shape_table[j + 1] = shapes[i][j];
            }
            result[i + 1] = shape_table;
        }
        return sol::make_object(lua, result);
    };

    onnx_module["get_output_shapes"] = [](sol::this_state L, std::shared_ptr<OnnxSession> session) -> sol::object {
        sol::state_view lua(L);
        if (!session) {
            return sol::make_object(lua, sol::nil);
        }

        auto shapes = session->getOutputShapes();
        sol::table result = lua.create_table();
        for (size_t i = 0; i < shapes.size(); i++) {
            sol::table shape_table = lua.create_table();
            for (size_t j = 0; j < shapes[i].size(); j++) {
                shape_table[j + 1] = shapes[i][j];
            }
            result[i + 1] = shape_table;
        }
        return sol::make_object(lua, result);
    };

    onnx_module.push(L);
    return 1;
}

}

}
