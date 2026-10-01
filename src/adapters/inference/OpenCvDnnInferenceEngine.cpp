#include "adapters/inference/OpenCvDnnInferenceEngine.h"

#include "adapters/inference/OnnxModelInfo.h"

#include <opencv2/core.hpp>

namespace baekar {

bool OpenCvDnnInferenceEngine::load(const std::string& modelPath, std::string* error) {
    OnnxModelInfo info;
    if (!readOnnxModelInfo(modelPath, info, error)) return false;
    try {
        net_ = cv::dnn::readNetFromONNX(modelPath);
    } catch (const cv::Exception& e) {
        if (error) *error = e.what();
        return false;
    }
    if (net_.empty()) {
        if (error) *error = "OpenCV DNN could not load " + modelPath;
        return false;
    }
    net_.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
    net_.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
    path_ = modelPath;
    inputs_ = std::move(info.inputs);
    outputs_ = std::move(info.outputs);
    return true;
}

bool OpenCvDnnInferenceEngine::run(const std::vector<Tensor>& inputs, std::vector<Tensor>& outputs,
                                   std::string* error) {
    auto failWith = [&](const std::string& message) {
        if (error) *error = message;
        return false;
    };
    if (net_.empty()) return failWith("no model loaded");
    if (inputs.size() != inputs_.size()) return failWith("expected " + std::to_string(inputs_.size()) + " input(s)");
    try {
        for (const Tensor& input : inputs) {
            if (!input.consistent()) return failWith("input " + input.name + ": data does not match its shape");
            std::vector<int> sizes(input.shape.begin(), input.shape.end());
            // setInput copies the blob, so wrapping the caller's data is safe.
            const cv::Mat blob(static_cast<int>(sizes.size()), sizes.data(), CV_32F,
                               const_cast<float*>(input.data.data()));
            net_.setInput(blob, input.name);
        }
        std::vector<cv::String> names;
        for (const TensorInfo& output : outputs_) names.push_back(output.name);
        std::vector<cv::Mat> results;
        net_.forward(results, names);

        outputs.clear();
        for (std::size_t i = 0; i < results.size() && i < outputs_.size(); ++i) {
            const cv::Mat result = results[i].isContinuous() ? results[i] : results[i].clone();
            Tensor tensor;
            tensor.name = outputs_[i].name;
            for (int d = 0; d < result.dims; ++d) tensor.shape.push_back(result.size[d]);
            const float* begin = result.ptr<float>();
            tensor.data.assign(begin, begin + result.total());
            outputs.push_back(std::move(tensor));
        }
    } catch (const cv::Exception& e) {
        return failWith(e.what());
    }
    return outputs.size() == outputs_.size() || failWith("model returned fewer outputs than it declares");
}

std::string OpenCvDnnInferenceEngine::describe() const {
    return "OpenCV DNN " + std::string(CV_VERSION) + " (CPU)" + (path_.empty() ? "" : ": " + path_);
}

}  // namespace baekar
