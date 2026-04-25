# metal-cpp

This directory is intentionally empty in source control. Apple distributes
`metal-cpp` as a header bundle; download it once before configuring the build:

    ./third_party/fetch_metal_cpp.sh

The script unpacks Apple's `metal-cpp` headers in place so that
`#include <Metal/Metal.hpp>` resolves. CMake adds this directory to the
include path; the renderer's Objective-C++ implementation imports the
Apple frameworks directly and does not require metal-cpp on disk to build,
but having the headers available is helpful for projects that want to
extend the renderer in plain C++.

Upstream:
    https://developer.apple.com/metal/cpp/
