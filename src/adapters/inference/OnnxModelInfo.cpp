#include "adapters/inference/OnnxModelInfo.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <set>

namespace baekar {
namespace {

// Protobuf wire format: a sequence of (field number, wire type) keys, each
// followed by a varint, a fixed 32/64-bit value or a length-prefixed blob.
// Only the fields needed for names and shapes are decoded.
class Reader {
public:
    Reader(const unsigned char* begin, const unsigned char* end) : p_(begin), end_(end) {}

    bool done() const { return p_ >= end_ || failed_; }
    bool failed() const { return failed_; }

    bool next(std::uint32_t& field, std::uint32_t& wireType) {
        std::uint64_t key = 0;
        if (!varint(key)) return false;
        field = static_cast<std::uint32_t>(key >> 3);
        wireType = static_cast<std::uint32_t>(key & 7);
        return field != 0 || fail();
    }
    bool varint(std::uint64_t& value) {
        value = 0;
        for (int shift = 0; shift < 64; shift += 7) {
            if (p_ >= end_) return fail();
            const unsigned char byte = *p_++;
            value |= static_cast<std::uint64_t>(byte & 0x7f) << shift;
            if (!(byte & 0x80)) return true;
        }
        return fail();
    }
    bool bytes(Reader& inner) {
        std::uint64_t size = 0;
        if (!varint(size) || size > static_cast<std::uint64_t>(end_ - p_)) return fail();
        inner = Reader(p_, p_ + size);
        p_ += size;
        return true;
    }
    bool string(std::string& value) {
        Reader inner(nullptr, nullptr);
        if (!bytes(inner)) return false;
        value.assign(inner.p_, inner.end_);
        return true;
    }
    bool skip(std::uint32_t wireType) {
        std::uint64_t ignored = 0;
        Reader inner(nullptr, nullptr);
        switch (wireType) {
            case 0: return varint(ignored);
            case 1: return advance(8);
            case 2: return bytes(inner);
            case 5: return advance(4);
            default: return fail();
        }
    }

private:
    bool advance(std::size_t n) {
        if (static_cast<std::size_t>(end_ - p_) < n) return fail();
        p_ += n;
        return true;
    }
    bool fail() {
        failed_ = true;
        return false;
    }

    const unsigned char* p_;
    const unsigned char* end_;
    bool failed_ = false;
};

// TensorShapeProto.Dimension: dim_value = 1, dim_param = 2.
std::int64_t readDimension(Reader r) {
    std::int64_t value = -1;
    std::uint32_t field, wire;
    while (!r.done() && r.next(field, wire)) {
        std::uint64_t v = 0;
        if (field == 1 && wire == 0 && r.varint(v)) value = static_cast<std::int64_t>(v);
        else r.skip(wire);
    }
    return value;
}

// ValueInfoProto: name = 1, type = 2 -> TypeProto.tensor_type = 1 ->
// shape = 2 -> TensorShapeProto.dim = 1.
TensorInfo readValueInfo(Reader r) {
    TensorInfo info;
    std::uint32_t field, wire;
    while (!r.done() && r.next(field, wire)) {
        if (field == 1 && wire == 2) {
            r.string(info.name);
        } else if (field == 2 && wire == 2) {
            Reader type(nullptr, nullptr);
            r.bytes(type);
            while (!type.done() && type.next(field, wire)) {
                if (field != 1 || wire != 2) { type.skip(wire); continue; }
                Reader tensor(nullptr, nullptr);
                type.bytes(tensor);
                while (!tensor.done() && tensor.next(field, wire)) {
                    if (field != 2 || wire != 2) { tensor.skip(wire); continue; }
                    Reader shape(nullptr, nullptr);
                    tensor.bytes(shape);
                    while (!shape.done() && shape.next(field, wire)) {
                        Reader dim(nullptr, nullptr);
                        if (field == 1 && wire == 2 && shape.bytes(dim)) info.shape.push_back(readDimension(dim));
                        else shape.skip(wire);
                    }
                }
            }
        } else {
            r.skip(wire);
        }
    }
    return info;
}

// TensorProto.name = 8.
std::string readTensorName(Reader r) {
    std::string name;
    std::uint32_t field, wire;
    while (!r.done() && r.next(field, wire)) {
        if (field == 8 && wire == 2) r.string(name);
        else r.skip(wire);
    }
    return name;
}

}  // namespace

bool readOnnxModelInfo(const std::string& path, OnnxModelInfo& info, std::string* error) {
    auto failWith = [&](const std::string& message) {
        if (error) *error = message;
        return false;
    };
    std::ifstream file(path, std::ios::binary);
    if (!file) return failWith("cannot open " + path);
    const std::vector<unsigned char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    // ModelProto.graph = 7; GraphProto.initializer = 5, input = 11, output = 12.
    Reader model(buffer.data(), buffer.data() + buffer.size());
    std::vector<TensorInfo> inputs, outputs;
    std::set<std::string> initializers;
    bool sawGraph = false;
    std::uint32_t field, wire;
    while (!model.done() && model.next(field, wire)) {
        if (field != 7 || wire != 2) { model.skip(wire); continue; }
        sawGraph = true;
        Reader graph(nullptr, nullptr);
        model.bytes(graph);
        while (!graph.done() && graph.next(field, wire)) {
            Reader item(nullptr, nullptr);
            if (wire != 2) { graph.skip(wire); continue; }
            graph.bytes(item);
            if (field == 5) initializers.insert(readTensorName(item));
            else if (field == 11) inputs.push_back(readValueInfo(item));
            else if (field == 12) outputs.push_back(readValueInfo(item));
        }
        if (graph.failed()) return failWith("malformed ONNX graph in " + path);
    }
    if (model.failed() || !sawGraph) return failWith(path + " is not an ONNX model");

    // Before IR version 4, initializers were also listed as graph inputs.
    inputs.erase(std::remove_if(inputs.begin(), inputs.end(),
                                [&](const TensorInfo& t) { return initializers.count(t.name) > 0; }),
                 inputs.end());
    info.inputs = std::move(inputs);
    info.outputs = std::move(outputs);
    return true;
}

}  // namespace baekar
