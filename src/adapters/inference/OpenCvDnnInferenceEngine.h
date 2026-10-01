#ifndef BAEKAR_ADAPTERS_INFERENCE_OPENCV_DNN_INFERENCE_ENGINE_H
#define BAEKAR_ADAPTERS_INFERENCE_OPENCV_DNN_INFERENCE_ENGINE_H

#include "application/ports/IInferenceEngine.h"

#include <opencv2/dnn.hpp>

namespace baekar {

// ONNX models on the CPU through OpenCV's DNN module: the portable engine
// (Linux, macOS without Core ML). No dependency beyond OpenCV.
class OpenCvDnnInferenceEngine final : public IInferenceEngine {
public:
    bool load(const std::string& modelPath, std::string* error = nullptr) override;
    std::vector<TensorInfo> inputs() const override { return inputs_; }
    std::vector<TensorInfo> outputs() const override { return outputs_; }
    bool run(const std::vector<Tensor>& inputs, std::vector<Tensor>& outputs,
             std::string* error = nullptr) override;
    std::string describe() const override;

private:
    cv::dnn::Net net_;
    std::string path_;
    std::vector<TensorInfo> inputs_;
    std::vector<TensorInfo> outputs_;
};

}  // namespace baekar

#endif  // BAEKAR_ADAPTERS_INFERENCE_OPENCV_DNN_INFERENCE_ENGINE_H
