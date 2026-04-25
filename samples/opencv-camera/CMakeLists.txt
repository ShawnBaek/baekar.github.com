cmake_minimum_required(VERSION 3.20)
project(OpenCVCameraSample LANGUAGES CXX OBJCXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV 4 REQUIRED)

add_executable(OpenCVCameraSample main.mm)

target_link_libraries(OpenCVCameraSample PRIVATE
    ${OpenCV_LIBS}
    "-framework AVFoundation"
    "-framework Foundation"
    "-lobjc"
)

if(APPLE)
    set_target_properties(OpenCVCameraSample PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
        MACOSX_BUNDLE_BUNDLE_NAME "OpenCVCameraSample"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.backar.opencv-camera-sample"
    )
endif()
