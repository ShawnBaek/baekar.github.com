#ifndef BAEKAR_CORE_TENSOR_H
#define BAEKAR_CORE_TENSOR_H

#include <cstdint>
#include <string>
#include <vector>

namespace baekar {

// Name and shape of a model input or output. -1 marks a dimension the model
// leaves open (batch size, image size).
struct TensorInfo {
    std::string name;
    std::vector<std::int64_t> shape;
};

// Dense float32 tensor in row-major (C) order.
struct Tensor {
    std::string name;
    std::vector<std::int64_t> shape;
    std::vector<float> data;

    static std::size_t elementCount(const std::vector<std::int64_t>& shape) {
        std::size_t n = 1;
        for (std::int64_t d : shape) n *= static_cast<std::size_t>(d < 0 ? 0 : d);
        return n;
    }
    bool consistent() const { return data.size() == elementCount(shape); }
};

}  // namespace baekar

#endif  // BAEKAR_CORE_TENSOR_H
