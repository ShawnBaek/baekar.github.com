cmake_minimum_required(VERSION 3.21)
project(BaekAR LANGUAGES CXX OBJCXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OBJCXX_STANDARD 17)

if(NOT APPLE)
    message(FATAL_ERROR "BaekAR currently only builds on macOS. See README.md.")
endif()

find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs videoio features2d calib3d video)
find_package(glfw3 REQUIRED)

set(BAEKAR_THIRD_PARTY ${CMAKE_SOURCE_DIR}/third_party)

# Shared cross-platform vision/render layer.
add_library(baekar_engine STATIC
    engine/vision/BriskTracker.cpp
    engine/vision/FeaturePose.cpp
    engine/render_metal/MetalRenderer.mm
)
target_include_directories(baekar_engine PUBLIC
    ${CMAKE_SOURCE_DIR}/engine
    ${BAEKAR_THIRD_PARTY}/metal-cpp
    ${OpenCV_INCLUDE_DIRS}
)
target_link_libraries(baekar_engine PUBLIC
    ${OpenCV_LIBS}
    "-framework Metal"
    "-framework MetalKit"
    "-framework Foundation"
    "-framework QuartzCore"
    "-framework Cocoa"
)
target_compile_options(baekar_engine PRIVATE -Wall -Wno-deprecated-declarations)
# .mm files use ARC-style bridging casts (__bridge); enable ARC for them.
set_source_files_properties(
    engine/render_metal/MetalRenderer.mm
    engine/main.mm
    PROPERTIES COMPILE_FLAGS "-fobjc-arc"
)

# Compile Shaders.metal into a default.metallib next to the executable.
set(METAL_SHADER_SRC ${CMAKE_SOURCE_DIR}/engine/render_metal/Shaders.metal)
set(METAL_SHADER_AIR ${CMAKE_BINARY_DIR}/Shaders.air)
set(METAL_LIB ${CMAKE_BINARY_DIR}/default.metallib)
add_custom_command(
    OUTPUT ${METAL_LIB}
    COMMAND xcrun -sdk macosx metal -c ${METAL_SHADER_SRC} -o ${METAL_SHADER_AIR}
    COMMAND xcrun -sdk macosx metallib ${METAL_SHADER_AIR} -o ${METAL_LIB}
    DEPENDS ${METAL_SHADER_SRC}
    COMMENT "Compiling Metal shader library"
    VERBATIM
)
add_custom_target(baekar_shaders ALL DEPENDS ${METAL_LIB})

add_executable(baekar engine/main.mm)
add_dependencies(baekar baekar_shaders)
target_link_libraries(baekar PRIVATE baekar_engine glfw)
target_compile_options(baekar PRIVATE -Wall)

# Embed Info.plist into the binary so macOS TCC can read NSCameraUsageDescription
# without us needing to produce a full .app bundle.
target_link_options(baekar PRIVATE
    "LINKER:-sectcreate,__TEXT,__info_plist,${CMAKE_SOURCE_DIR}/engine/Info.plist.in"
)

option(BAEKAR_BUILD_TESTS "Build smoke tests" ON)
if(BAEKAR_BUILD_TESTS)
    add_executable(test_feature_pose tests/test_feature_pose.cpp)
    target_link_libraries(test_feature_pose PRIVATE baekar_engine)
    enable_testing()
    add_test(NAME feature_pose COMMAND test_feature_pose)
endif()
