import torch
import torch.nn as nn

class SimpleModel(nn.Module):
    def __init__(self):
        super().__init__()
        self.linear = nn.Linear(4, 2)

    def forward(self, x):
        return self.linear(x)

model = SimpleModel()
model.eval()

dummy_input = torch.randn(1, 4)

torch.onnx.export(
    model,
    dummy_input,
    "test_model.onnx",
    export_params=True,
    opset_version=14,
    input_names=['input'],
    output_names=['output'],
    dynamo=False
)

print("created test_model.onnx")
