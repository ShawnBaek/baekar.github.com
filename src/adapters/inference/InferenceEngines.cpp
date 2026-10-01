#include "adapters/inference/InferenceEngines.h"

#include "adapters/inference/OpenCvDnnInferenceEngine.h"

#ifdef __APPLE__
#include "platform/macos/CoreMlInferenceEngine.h"
#endif

#include <algorithm>
#include <cctype>
#include <filesystem>

namespace baekar {

bool coreMlAvailable() {
#ifdef __APPLE__
    return true;
#else
    return false;
#endif
}

std::unique_ptr<IInferenceEngine> openInferenceModel(const std::string& path, std::string* error) {
    // .mlpackage and .mlmodelc are folders; a trailing slash would hide the extension.
    std::string trimmed = path;
    while (trimmed.size() > 1 && trimmed.back() == '/') trimmed.pop_back();
    std::string extension = std::filesystem::path(trimmed).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    std::unique_ptr<IInferenceEngine> engine;
    if (extension == ".onnx") {
        engine = std::make_unique<OpenCvDnnInferenceEngine>();
    } else if (extension == ".mlmodel" || extension == ".mlpackage" || extension == ".mlmodelc") {
#ifdef __APPLE__
        engine = std::make_unique<CoreMlInferenceEngine>();
#else
        if (error) *error = "Core ML models need macOS or iOS; use the .onnx export on this platform";
        return nullptr;
#endif
    } else {
        if (error) *error = "unknown model type '" + extension + "' (expected .onnx, .mlmodel, .mlpackage or .mlmodelc)";
        return nullptr;
    }
    if (!engine->load(path, error)) return nullptr;
    return engine;
}

}  // namespace baekar
