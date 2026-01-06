# onnx-lua

onnx runtime bindings for luajit 2.1.0 using sol2

## setup

```bash
git clone --recursive https://github.com/chasebarrington/onnx-lua.git
```

download [onnx runtime 1.17.1](https://github.com/microsoft/onnxruntime/releases/download/v1.17.1/onnxruntime-win-x64-1.17.1.zip) and extract to `onnxruntime/`

## build

```bash
build_luajit.bat
build_onnx_lua.bat
```

## usage

```lua
-- load model
local model = onnx.load_model("model.onnx")

-- inspect model
local input_names = onnx.get_input_names(model)      -- {"input"}
local input_shapes = onnx.get_input_shapes(model)    -- {{1, 4}}
local output_names = onnx.get_output_names(model)    -- {"output"}
local output_shapes = onnx.get_output_shapes(model)  -- {{1, 2}}

print("model expects input:", input_names[1], "with shape", input_shapes[1][1], "x", input_shapes[1][2])
print("model outputs:", output_names[1], "with shape", output_shapes[1][1], "x", output_shapes[1][2])

-- prepare input matching expected shape
local input_data = {1.0, 2.0, 3.0, 4.0}
local outputs = onnx.run(model, {input_data}, input_shapes)

-- get results
print("output:", outputs[1][1], outputs[1][2])  -- output: -3.47 1.94
```

## integration

```cpp
lua_pushcfunction(L, OnnxLua::luaopen_onnx);
lua_call(L, 0, 1);
lua_setglobal(L, "onnx");
```

include directories:
- `include/` - onnx_lua headers
- `sol2/include/` - sol2 headers
- `LuaJIT-2.1.0-beta3/src/` - luajit headers
- `onnxruntime/include/` - onnx runtime headers

link libraries:
- `build/onnx_lua.lib` - lua bindings
- `onnxruntime/lib/onnxruntime.lib` - onnx runtime
- `winmm.lib`, `ws2_32.lib` - system dependencies
