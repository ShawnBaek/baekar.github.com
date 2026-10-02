#ifndef BAEKAR_APPLICATION_PORTS_IINFERENCE_ENGINE_H
#define BAEKAR_APPLICATION_PORTS_IINFERENCE_ENGINE_H

#include "core/Tensor.h"

#include <string>
#include <vector>

namespace baekar {

// Strategy for running a neural network: Core ML on macOS/iOS, ONNX through
// OpenCV DNN everywhere else. Learned trackers (hand pose, features,
// matching) depend on this port, not on a runtime. Not thread-safe: use one
// engine per thread.
class IInferenceEngine {
public:
    virtual ~IInferenceEngine() = default;

    virtual bool load(const std::string& modelPath, std::string* error = nullptr) = 0;
    virtual std::vector<TensorInfo> inputs() const = 0;
    virtual std::vector<TensorInfo> outputs() const = 0;
    // Inputs are matched by name. Fills one tensor per model output, in
    // outputs() order.
    virtual bool run(const std::vector<Tensor>& inputs, std::vector<Tensor>& outputs,
                     std::string* error = nullptr) = 0;
    virtual std::string describe() const = 0;
};

}  // namespace baekar

#endif  // BAEKAR_APPLICATION_PORTS_IINFERENCE_ENGINE_H
