#ifndef BAEKAR_ADAPTERS_INFERENCE_ONNX_MODEL_INFO_H
#define BAEKAR_ADAPTERS_INFERENCE_ONNX_MODEL_INFO_H

#include "core/Tensor.h"

#include <string>
#include <vector>

namespace baekar {

// Graph inputs (without initializers) and outputs of an ONNX file, read
// straight from its protobuf encoding. OpenCV DNN does not expose them.
struct OnnxModelInfo {
    std::vector<TensorInfo> inputs;
    std::vector<TensorInfo> outputs;
};

bool readOnnxModelInfo(const std::string& path, OnnxModelInfo& info, std::string* error = nullptr);

}  // namespace baekar

#endif  // BAEKAR_ADAPTERS_INFERENCE_ONNX_MODEL_INFO_H
