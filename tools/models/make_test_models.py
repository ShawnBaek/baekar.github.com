"""Writes the inference test models in tests/data/models/.

Both models compute the same thing from one input "image" (float32,
1x3x4x4):
  scaled = 2 * image + 1         (1x3x4x4)
  channel_sum = sum over axis 1  (1x1x4x4)

  python3 -m venv .venv && .venv/bin/pip install onnx coremltools numpy
  .venv/bin/python tools/models/make_test_models.py

coremltools only writes the model spec here; Core ML compiles it on the
Mac at load time.
"""
import os

import numpy as np
import onnx
from onnx import TensorProto, helper

OUT = os.path.join(os.path.dirname(__file__), "..", "..", "tests", "data", "models")
SHAPE = [1, 3, 4, 4]


def write_onnx(path):
    two = helper.make_tensor("two", TensorProto.FLOAT, [], [2.0])
    one = helper.make_tensor("one", TensorProto.FLOAT, [], [1.0])
    nodes = [
        helper.make_node("Mul", ["image", "two"], ["doubled"]),
        helper.make_node("Add", ["doubled", "one"], ["scaled"]),
        helper.make_node("ReduceSum", ["image"], ["channel_sum"], axes=[1], keepdims=1),
    ]
    graph = helper.make_graph(
        nodes, "baekar_affine",
        [helper.make_tensor_value_info("image", TensorProto.FLOAT, SHAPE)],
        [helper.make_tensor_value_info("scaled", TensorProto.FLOAT, SHAPE),
         helper.make_tensor_value_info("channel_sum", TensorProto.FLOAT, [1, 1, 4, 4])],
        initializer=[two, one])
    model = helper.make_model(graph, opset_imports=[helper.make_opsetid("", 11)],
                              producer_name="baekar-test-models")
    model.ir_version = 7
    onnx.checker.check_model(model)
    onnx.save(model, path)


def write_coreml(path):
    import coremltools as ct
    from coremltools.models import datatypes
    from coremltools.models.neural_network import NeuralNetworkBuilder

    builder = NeuralNetworkBuilder(
        [("image", datatypes.Array(*SHAPE))],
        [("scaled", datatypes.Array(*SHAPE)), ("channel_sum", datatypes.Array(1, 1, 4, 4))],
        disable_rank5_shape_mapping=True)
    builder.add_activation("affine", "LINEAR", "image", "scaled", params=[2.0, 1.0])
    builder.add_reduce_sum("sum", "image", "channel_sum", axes=[1], keepdims=True)
    spec = builder.spec
    spec.description.metadata.shortDescription = "BaekAR inference test model"
    ct.utils.save_spec(spec, path)


if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    write_onnx(os.path.join(OUT, "affine.onnx"))
    write_coreml(os.path.join(OUT, "affine.mlmodel"))
    print("wrote", sorted(os.listdir(OUT)))
