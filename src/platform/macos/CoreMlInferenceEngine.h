#ifndef BAEKAR_PLATFORM_MACOS_CORE_ML_INFERENCE_ENGINE_H
#define BAEKAR_PLATFORM_MACOS_CORE_ML_INFERENCE_ENGINE_H

#include "application/ports/IInferenceEngine.h"

#include <memory>

namespace baekar {

// Core ML models (.mlmodel, .mlpackage, compiled .mlmodelc) on the CPU, GPU
// and Neural Engine. Uncompiled models are compiled at load time. Inputs
// and outputs must be multi-arrays; inputs are passed as float32.
class CoreMlInferenceEngine final : public IInferenceEngine {
public:
    CoreMlInferenceEngine();
    ~CoreMlInferenceEngine() override;

    bool load(const std::string& modelPath, std::string* error = nullptr) override;
    std::vector<TensorInfo> inputs() const override { return inputs_; }
    std::vector<TensorInfo> outputs() const override { return outputs_; }
    bool run(const std::vector<Tensor>& inputs, std::vector<Tensor>& outputs,
             std::string* error = nullptr) override;
    std::string describe() const override;

private:
    struct Model;
    std::unique_ptr<Model> model_;
    std::string path_;
    std::vector<TensorInfo> inputs_;
    std::vector<TensorInfo> outputs_;
};

}  // namespace baekar

#endif  // BAEKAR_PLATFORM_MACOS_CORE_ML_INFERENCE_ENGINE_H
