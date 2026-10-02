#ifndef BAEKAR_ADAPTERS_INFERENCE_INFERENCE_ENGINES_H
#define BAEKAR_ADAPTERS_INFERENCE_INFERENCE_ENGINES_H

#include "application/ports/IInferenceEngine.h"

#include <memory>
#include <string>

namespace baekar {

// Picks the engine from the file type and loads the model:
//   .onnx                          OpenCV DNN (every platform)
//   .mlmodel .mlpackage .mlmodelc  Core ML (Apple only)
// Returns nullptr and sets `error` when the model cannot be loaded here.
std::unique_ptr<IInferenceEngine> openInferenceModel(const std::string& path, std::string* error = nullptr);

// True when this build can run Core ML models.
bool coreMlAvailable();

}  // namespace baekar

#endif  // BAEKAR_ADAPTERS_INFERENCE_INFERENCE_ENGINES_H
