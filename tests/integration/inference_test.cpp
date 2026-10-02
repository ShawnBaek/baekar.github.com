// IInferenceEngine on the test models in tests/data/models (written by
// tools/models/make_test_models.py): scaled = 2 * image + 1 and
// channel_sum = sum of image over its channel axis. ONNX runs everywhere;
// on macOS the Core ML model must give the same numbers.

#include "adapters/inference/InferenceEngines.h"
#include "adapters/inference/OnnxModelInfo.h"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>

using namespace baekar;

namespace {

int failures = 0;

void expect(bool condition, const char* what) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", what);
        ++failures;
    }
}

const std::vector<std::int64_t> kShape = {1, 3, 4, 4};

Tensor makeInput() {
    Tensor input;
    input.name = "image";
    input.shape = kShape;
    input.data.resize(Tensor::elementCount(kShape));
    for (std::size_t i = 0; i < input.data.size(); ++i) input.data[i] = 0.25f * static_cast<float>(i) - 3.0f;
    return input;
}

std::map<std::string, Tensor> byName(const std::vector<Tensor>& tensors) {
    std::map<std::string, Tensor> result;
    for (const Tensor& t : tensors) result[t.name] = t;
    return result;
}

// Checks one engine's outputs against the expected values.
void checkOutputs(const char* engine, const Tensor& input, const std::vector<Tensor>& outputs) {
    auto named = byName(outputs);
    const Tensor& scaled = named["scaled"];
    const Tensor& sum = named["channel_sum"];
    std::fprintf(stderr, "%s: %zu output(s)\n", engine, outputs.size());
    expect(outputs.size() == 2, "two outputs");
    expect(scaled.shape == kShape && scaled.consistent(), "scaled shape 1x3x4x4");
    expect(sum.consistent() && sum.data.size() == 16, "channel_sum has 16 values");
    double scaledError = 0, sumError = 0;
    for (std::size_t i = 0; i < scaled.data.size() && i < input.data.size(); ++i)
        scaledError = std::max(scaledError, std::abs(scaled.data[i] - (2.0 * input.data[i] + 1.0)));
    for (std::size_t p = 0; p < sum.data.size(); ++p) {
        const double expected = input.data[p] + input.data[16 + p] + input.data[32 + p];
        sumError = std::max(sumError, std::abs(sum.data[p] - expected));
    }
    std::fprintf(stderr, "%s: max error scaled %.2e, channel_sum %.2e\n", engine, scaledError, sumError);
    // Core ML may run in float16 on the GPU or Neural Engine.
    expect(scaledError < 2e-2 && sumError < 2e-2, "outputs match 2x+1 and the channel sum");
}

}  // namespace

int main() {
    const std::string models = std::string(BAEKAR_SOURCE_DIR) + "/tests/data/models";
    const std::string onnxPath = models + "/affine.onnx";
    const std::string coreMlPath = models + "/affine.mlmodel";

    // Metadata straight from the ONNX protobuf.
    {
        OnnxModelInfo info;
        std::string error;
        expect(readOnnxModelInfo(onnxPath, info, &error), "ONNX metadata reads");
        expect(info.inputs.size() == 1 && info.inputs[0].name == "image" && info.inputs[0].shape == kShape,
               "one input 'image' 1x3x4x4 (initializers excluded)");
        expect(info.outputs.size() == 2 && info.outputs[0].name == "scaled" &&
                   info.outputs[1].name == "channel_sum" &&
                   info.outputs[1].shape == std::vector<std::int64_t>({1, 1, 4, 4}),
               "outputs 'scaled' and 'channel_sum' with shapes");

        const std::string junk = (std::filesystem::temp_directory_path() / "baekar_not_a_model.onnx").string();
        std::ofstream(junk, std::ios::binary) << "\xff\xff\xff\xff not protobuf";
        expect(!readOnnxModelInfo(junk, info, &error) && !error.empty(), "garbage is rejected");
        std::filesystem::remove(junk);
    }

    const Tensor input = makeInput();
    std::vector<Tensor> onnxOutputs;

    // ONNX through OpenCV DNN.
    {
        std::string error;
        auto engine = openInferenceModel(onnxPath, &error);
        expect(engine != nullptr, "ONNX model opens");
        if (engine) {
            std::fprintf(stderr, "%s\n", engine->describe().c_str());
            expect(engine->inputs().size() == 1 && engine->outputs().size() == 2, "engine reports inputs/outputs");
            expect(engine->run({input}, onnxOutputs, &error), "ONNX run");
            checkOutputs("OpenCV DNN", input, onnxOutputs);

            Tensor wrong = input;
            wrong.data.pop_back();
            std::vector<Tensor> ignored;
            expect(!engine->run({wrong}, ignored, &error) && !error.empty(), "inconsistent input is rejected");
        }
    }

    // Unknown file types fail with a message.
    {
        std::string error;
        expect(openInferenceModel(models + "/affine.tflite", &error) == nullptr && !error.empty(),
               "unknown model type is rejected");
    }

    // Core ML: same numbers on Apple platforms, a clear error elsewhere.
    {
        std::string error;
        auto engine = openInferenceModel(coreMlPath, &error);
        if (coreMlAvailable()) {
            expect(engine != nullptr, "Core ML model opens");
            if (!engine) std::fprintf(stderr, "Core ML: %s\n", error.c_str());
            if (engine) {
                std::fprintf(stderr, "%s\n", engine->describe().c_str());
                const auto inputs = engine->inputs();
                expect(inputs.size() == 1 && inputs[0].name == "image" && inputs[0].shape == kShape,
                       "Core ML input 'image' 1x3x4x4");
                std::vector<Tensor> outputs;
                expect(engine->run({input}, outputs, &error), "Core ML run");
                if (!error.empty()) std::fprintf(stderr, "Core ML: %s\n", error.c_str());
                checkOutputs("Core ML", input, outputs);
            }
        } else {
            expect(engine == nullptr && error.find("macOS") != std::string::npos,
                   "Core ML model explains it needs Apple platforms");
        }
    }

    if (failures == 0) std::fprintf(stderr, "inference_test: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
