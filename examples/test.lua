local model = onnx.load_model("test_model.onnx")
if not model then
    print("error: failed to load model")
    os.exit(1)
end

local input = {1.0, 2.0, 3.0, 4.0}
local inputs = {input}
local shapes = {{1, 4}}

print("input: [" .. table.concat(input, ", ") .. "]")

local outputs = onnx.run(model, inputs, shapes)
if not outputs then
    print("error: inference failed")
    os.exit(1)
end

print("output: [" .. outputs[1][1] .. ", " .. outputs[1][2] .. "]")
