#include "CoreMlInferenceEngine.h"

#import <CoreML/CoreML.h>
#import <Foundation/Foundation.h>

namespace baekar {

struct CoreMlInferenceEngine::Model {
    MLModel* model = nil;
};

namespace {

std::string describeError(NSError* error) {
    return error ? std::string(error.localizedDescription.UTF8String) : std::string("unknown Core ML error");
}

std::vector<std::int64_t> toShape(NSArray<NSNumber*>* shape) {
    std::vector<std::int64_t> result;
    for (NSNumber* d in shape) result.push_back(d.longLongValue);
    return result;
}

// Feature descriptions come in a dictionary; sort by name so inputs() and
// outputs() have a stable order.
std::vector<TensorInfo> describeFeatures(NSDictionary<NSString*, MLFeatureDescription*>* features) {
    std::vector<TensorInfo> result;
    NSArray<NSString*>* names = [features.allKeys sortedArrayUsingSelector:@selector(compare:)];
    for (NSString* name in names) {
        MLFeatureDescription* description = features[name];
        TensorInfo info;
        info.name = name.UTF8String;
        if (description.type == MLFeatureTypeMultiArray && description.multiArrayConstraint)
            info.shape = toShape(description.multiArrayConstraint.shape);
        result.push_back(info);
    }
    return result;
}

// Offset of row-major element `index` in a (possibly strided) multi-array.
NSInteger stridedOffset(std::size_t index, const std::vector<std::int64_t>& shape, NSArray<NSNumber*>* strides) {
    NSInteger offset = 0;
    for (NSInteger d = static_cast<NSInteger>(shape.size()) - 1; d >= 0; --d) {
        const std::int64_t extent = shape[static_cast<std::size_t>(d)];
        offset += static_cast<NSInteger>(index % static_cast<std::size_t>(extent)) * strides[d].integerValue;
        index /= static_cast<std::size_t>(extent);
    }
    return offset;
}

NSArray<NSNumber*>* multiIndex(std::size_t index, const std::vector<std::int64_t>& shape) {
    NSMutableArray<NSNumber*>* result = [NSMutableArray arrayWithCapacity:shape.size()];
    for (std::size_t d = 0; d < shape.size(); ++d) [result addObject:@0];
    for (NSInteger d = static_cast<NSInteger>(shape.size()) - 1; d >= 0; --d) {
        const std::size_t extent = static_cast<std::size_t>(shape[static_cast<std::size_t>(d)]);
        result[static_cast<NSUInteger>(d)] = @(index % extent);
        index /= extent;
    }
    return result;
}

}  // namespace

CoreMlInferenceEngine::CoreMlInferenceEngine() : model_(std::make_unique<Model>()) {}
CoreMlInferenceEngine::~CoreMlInferenceEngine() = default;

bool CoreMlInferenceEngine::load(const std::string& modelPath, std::string* error) {
    @autoreleasepool {
        NSURL* url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:modelPath.c_str()]];
        NSError* nsError = nil;
        NSURL* compiled = url;
        if (![url.pathExtension.lowercaseString isEqualToString:@"mlmodelc"]) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
            compiled = [MLModel compileModelAtURL:url error:&nsError];
#pragma clang diagnostic pop
            if (!compiled) {
                if (error) *error = "Core ML could not compile " + modelPath + ": " + describeError(nsError);
                return false;
            }
        }
        MLModelConfiguration* configuration = [[MLModelConfiguration alloc] init];
        configuration.computeUnits = MLComputeUnitsAll;
        MLModel* model = [MLModel modelWithContentsOfURL:compiled configuration:configuration error:&nsError];
        if (!model) {
            if (error) *error = "Core ML could not load " + modelPath + ": " + describeError(nsError);
            return false;
        }
        model_->model = model;
        path_ = modelPath;
        inputs_ = describeFeatures(model.modelDescription.inputDescriptionsByName);
        outputs_ = describeFeatures(model.modelDescription.outputDescriptionsByName);
        return true;
    }
}

bool CoreMlInferenceEngine::run(const std::vector<Tensor>& inputs, std::vector<Tensor>& outputs, std::string* error) {
    auto failWith = [&](const std::string& message) {
        if (error) *error = message;
        return false;
    };
    if (!model_->model) return failWith("no model loaded");
    @autoreleasepool {
        NSMutableDictionary<NSString*, MLFeatureValue*>* features = [NSMutableDictionary dictionary];
        for (const Tensor& input : inputs) {
            if (!input.consistent()) return failWith("input " + input.name + ": data does not match its shape");
            NSMutableArray<NSNumber*>* shape = [NSMutableArray array];
            for (std::int64_t d : input.shape) [shape addObject:@(d)];
            NSError* nsError = nil;
            MLMultiArray* array = [[MLMultiArray alloc] initWithShape:shape
                                                             dataType:MLMultiArrayDataTypeFloat32
                                                                error:&nsError];
            if (!array) return failWith("input " + input.name + ": " + describeError(nsError));
            float* data = static_cast<float*>(array.dataPointer);
            for (std::size_t i = 0; i < input.data.size(); ++i)
                data[stridedOffset(i, input.shape, array.strides)] = input.data[i];
            features[[NSString stringWithUTF8String:input.name.c_str()]] = [MLFeatureValue featureValueWithMultiArray:array];
        }

        NSError* nsError = nil;
        MLDictionaryFeatureProvider* provider = [[MLDictionaryFeatureProvider alloc] initWithDictionary:features
                                                                                                  error:&nsError];
        if (!provider) return failWith(describeError(nsError));
        id<MLFeatureProvider> result = [model_->model predictionFromFeatures:provider error:&nsError];
        if (!result) return failWith("Core ML prediction failed: " + describeError(nsError));

        outputs.clear();
        for (const TensorInfo& info : outputs_) {
            MLMultiArray* array = [result featureValueForName:[NSString stringWithUTF8String:info.name.c_str()]]
                                      .multiArrayValue;
            if (!array) return failWith("output " + info.name + " is not a multi-array");
            Tensor tensor;
            tensor.name = info.name;
            tensor.shape = toShape(array.shape);
            const std::size_t count = Tensor::elementCount(tensor.shape);
            tensor.data.resize(count);
            for (std::size_t i = 0; i < count; ++i) {
                const NSInteger offset = stridedOffset(i, tensor.shape, array.strides);
                switch (array.dataType) {
                    case MLMultiArrayDataTypeFloat32:
                        tensor.data[i] = static_cast<const float*>(array.dataPointer)[offset];
                        break;
                    case MLMultiArrayDataTypeDouble:
                        tensor.data[i] = static_cast<float>(static_cast<const double*>(array.dataPointer)[offset]);
                        break;
                    default:  // float16, int32: through NSNumber, by multi-index
                        tensor.data[i] = array[multiIndex(i, tensor.shape)].floatValue;
                        break;
                }
            }
            outputs.push_back(std::move(tensor));
        }
        return true;
    }
}

std::string CoreMlInferenceEngine::describe() const {
    return "Core ML (CPU, GPU, Neural Engine)" + (path_.empty() ? std::string() : ": " + path_);
}

}  // namespace baekar
