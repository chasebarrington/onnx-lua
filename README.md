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
local model = onnx.load_model("model.onnx")
local outputs = onnx.run(model, {input}, {{1, 4}})
```

## integration

```cpp
lua_pushcfunction(L, OnnxLua::luaopen_onnx);
lua_call(L, 0, 1);
lua_setglobal(L, "onnx");
```

link: `onnx_lua.lib`, `lua51.lib`, `onnxruntime.lib`, `winmm.lib`, `ws2_32.lib`
