# onnx-lua

onnx runtime bindings for luajit 2.1.0

## setup

```bash
git clone --recursive <repo-url>
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

-- get model info
local input_names = onnx.get_input_names(model)
local input_shapes = onnx.get_input_shapes(model)
local output_names = onnx.get_output_names(model)

-- prepare input data (flattened)
local input_data = {1.0, 2.0, 3.0, 4.0}
local inputs = {input_data}
local shapes = {{1, 4}}

-- run inference
local outputs = onnx.run(model, inputs, shapes)

-- get results
local result = outputs[1]
print("output:", result[1], result[2])  -- output: -3.47 1.94
```

## integration

```cpp
lua_pushcfunction(L, OnnxLua::luaopen_onnx);
lua_call(L, 0, 1);
lua_setglobal(L, "onnx");
```

link libraries:
- `build/onnx_lua.lib` - lua bindings
- `onnxruntime/lib/onnxruntime.lib` - onnx runtime
- `winmm.lib`, `ws2_32.lib` - system dependencies
