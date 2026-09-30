cmake_minimum_required(VERSION 3.20)
if(APPLE)
    project(BaekAR LANGUAGES CXX OBJCXX)
else()
    project(BaekAR LANGUAGES CXX)
endif()

add_library(baekar_build_options INTERFACE)
target_compile_definitions(baekar_build_options INTERFACE
    GL_SILENCE_DEPRECATION
    # Enable HandyAR fingertip pipeline (gates wonjo_dx::bDrawHand=TRUE in wonjo.cpp).
    # Without this, OnProcess never runs and the gesture subsystem stays dormant.
    _DRAW_HAND
)

# New architecture code (src/, apps/, tests/unit): C++17, strict warnings.
function(baekar_use_modern_cpp target_name)
    set_target_properties(${target_name} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
    )
    target_compile_options(${target_name} PRIVATE -Wall -Wextra)
endfunction()

function(baekar_use_legacy_cpp target_name)
    set_target_properties(${target_name} PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS YES
    )
endfunction()

# Find dependencies
# The 2012 engine uses OpenCV's C API (IplImage, cv* functions), which
# OpenCV 5 removed. Homebrew's `opencv` formula is now 5.x, so prefer the
# versioned `opencv@4` keg when it is installed. Moving to OpenCV 5 is a
# separate migration (ADR 0002).
find_package(OpenCV 4 REQUIRED
    HINTS
        /opt/homebrew/opt/opencv@4/lib/cmake/opencv4
        /usr/local/opt/opencv@4/lib/cmake/opencv4
)
find_package(Threads REQUIRED)
find_package(glfw3 3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
# assimp loads the original .X mesh files (Arrow3Axis.X, hand001.X, ipad.X)
# the thesis ships (now in assets/meshes/). Optional: missing assimp falls
# back to the synthetic primitives in DrawMesh.
find_package(assimp QUIET)

# GLM is header-only; find via pkg-config or just use include path
find_path(GLM_INCLUDE_DIR glm/glm.hpp
    HINTS /opt/homebrew/include /usr/local/include
)
if(NOT GLM_INCLUDE_DIR)
    message(FATAL_ERROR "GLM not found. Install with: brew install glm")
endif()

include(CTest)

# 2012 engine sources. EngineMain.cpp was split into the legacy_* wrappers
# below (tracking, pose, hand, renderer, scene); main() lives in
# apps/baekar/main.cpp.
set(ENGINE_SOURCES
    legacy/bridge/legacy_marker_tracker.cpp
    legacy/bridge/marker_geometry.cpp
    legacy/bridge/legacy_pose.cpp
    legacy/bridge/legacy_hand.cpp
    legacy/bridge/legacy_renderer.cpp
    legacy/bridge/legacy_scene.cpp
    legacy/MarkerlessAR/d3d.cpp
    legacy/MarkerlessAR/wonjo.cpp
    legacy/MarkerlessAR/cam.cpp
    legacy/MarkerlessAR/BaekAR.cpp
    legacy/MarkerlessAR/Contents.cpp
    legacy/MarkerlessAR/Detector.cpp
    # legacy/MarkerlessAR/SungwookFeature.cpp  # GPL-3.0 header, pulls in bundled BRISK; replaced by legacy/bridge/marker_geometry.cpp
    legacy/MarkerlessAR/SungwookTransformFunctions.cpp
    legacy/MarkerlessAR/SungwookUtility.cpp
    legacy/MarkerlessAR/stdafx.cpp
)

# macOS platform layer: AVFoundation capture, camera permission and menu,
# ScreenCaptureKit window capture. Only this target links Apple frameworks.
if(APPLE)
    add_library(baekar_platform_macos STATIC
        src/platform/macos/macos_camera_auth.mm
        src/platform/macos/macos_window_capture.mm
        src/platform/macos/macos_av_capture.mm
        src/platform/macos/macos_camera_menu.mm
        src/platform/macos/AvFoundationFrameSource.cpp
        src/platform/macos/ScreenCaptureWindowSource.cpp
    )
    # These .mm files capture Obj-C objects via __block; without ARC the
    # captures don't auto-retain and the autorelease pool drains them
    # before they're read (objc_msgSend crash on dangling pointer).
    set_source_files_properties(
        src/platform/macos/macos_window_capture.mm
        src/platform/macos/macos_av_capture.mm
        src/platform/macos/macos_camera_menu.mm
        PROPERTIES COMPILE_FLAGS "-fobjc-arc"
    )
    set_target_properties(baekar_platform_macos PROPERTIES
        CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
    target_include_directories(baekar_platform_macos PUBLIC
        ${CMAKE_SOURCE_DIR}/src
        ${OpenCV_INCLUDE_DIRS}
    )
    target_link_libraries(baekar_platform_macos PUBLIC
        ${OpenCV_LIBS}
        "-framework AVFoundation"
        "-framework Foundation"
        "-framework Cocoa"
        "-framework ScreenCaptureKit"
        "-framework CoreMedia"
        "-framework CoreVideo"
        "-framework CoreGraphics"
        "-lobjc"
    )
endif()


# The bundled BRISK and AGAST (legacy/MarkerlessAR/brisk, agast) are
# GPL-3.0 and are not built: the engine uses OpenCV's cv::BRISK, and the
# project is Apache-2.0 (LICENSE, THIRD_PARTY.md).

# Calibration module
set(CALIBRATION_SOURCES
    # legacy/MarkerlessAR/calibration/BaekARCamera.cpp  # Excluded — duplicate CCamera class, only used by its own .cpp
    legacy/MarkerlessAR/calibration/Camera.cpp
    legacy/MarkerlessAR/calibration/cvFindExtrinsicCameraParams3.cpp
)

# Kato Pose Estimation
set(KATO_SOURCES
    legacy/MarkerlessAR/KatoPoseEstimation/KatoPoseEstimator.cpp
    legacy/MarkerlessAR/KatoPoseEstimation/KatoPoseEstimatorUtils.cpp
    legacy/MarkerlessAR/KatoPoseEstimation/PoseEstimator.cpp
)

# HandyAR hand tracking
set(HANDYAR_SOURCES
    # legacy/MarkerlessAR/HandyAR/Capture.cpp  # Replaced by the IFrameSource adapters (src/adapters/frame_source)
    legacy/MarkerlessAR/HandyAR/ColorDistribution.cpp
    legacy/MarkerlessAR/HandyAR/ColorHistogram.cpp
    legacy/MarkerlessAR/HandyAR/FingerTip.cpp
    legacy/MarkerlessAR/HandyAR/FingertipPoseEstimation.cpp
    legacy/MarkerlessAR/HandyAR/FingertipTracker.cpp
    # legacy/MarkerlessAR/HandyAR/FlyCap.cpp  # Excluded — requires PointGrey SDK
    legacy/MarkerlessAR/HandyAR/HandRegion.cpp
    legacy/MarkerlessAR/HandyAR/MixGaussian.cpp
    legacy/MarkerlessAR/HandyAR/PoseEstimation.cpp
)

# First extracted architecture boundary. Both the application and its
# characterization tests use the same tracking implementation.
add_library(baekar_tracking STATIC
    src/adapters/tracking/multi_marker_detector.cpp
    src/adapters/frame_source/synthetic_marker_source.cpp
)
set_target_properties(baekar_tracking PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO)
target_include_directories(baekar_tracking PUBLIC
    ${CMAKE_SOURCE_DIR}/src/adapters/tracking
    ${CMAKE_SOURCE_DIR}/src/adapters/frame_source
    ${OpenCV_INCLUDE_DIRS}
)
target_link_libraries(baekar_tracking PUBLIC
    baekar_build_options
    ${OpenCV_LIBS}
    Threads::Threads
)

if(BUILD_TESTING)
    add_executable(BaekARTrackingTests
        tests/multi_marker_detector_test.cpp
    )
    baekar_use_legacy_cpp(BaekARTrackingTests)
    target_compile_definitions(BaekARTrackingTests PRIVATE
        BAEKAR_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    )
    target_link_libraries(BaekARTrackingTests PRIVATE baekar_tracking)

    foreach(marker_count IN ITEMS 1 3 10)
        add_test(
            NAME multi_marker_tracking_${marker_count}
            COMMAND BaekARTrackingTests --marker-count ${marker_count}
        )
        set_tests_properties(multi_marker_tracking_${marker_count} PROPERTIES
            LABELS "characterization;tracking"
            TIMEOUT 45
        )
    endforeach()
endif()

# The 2012 engine. It stays at C++14 because its calibration sources use the
# `register` keyword that C++17 removed (ADR 0002). New code links it instead
# of compiling its sources.
add_library(baekar_legacy STATIC
    ${ENGINE_SOURCES}
    ${CALIBRATION_SOURCES}
    ${KATO_SOURCES}
    ${HANDYAR_SOURCES}
)
baekar_use_legacy_cpp(baekar_legacy)

target_include_directories(baekar_legacy PUBLIC
    ${CMAKE_SOURCE_DIR}/legacy/bridge
    ${CMAKE_SOURCE_DIR}/legacy/MarkerlessAR
    ${CMAKE_SOURCE_DIR}/legacy/MarkerlessAR/calibration
    ${CMAKE_SOURCE_DIR}/legacy/MarkerlessAR/KatoPoseEstimation
    ${CMAKE_SOURCE_DIR}/legacy/MarkerlessAR/HandyAR
    ${OpenCV_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
    ${GLUT_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIR}
)

target_link_libraries(baekar_legacy PUBLIC
    baekar_build_options
    baekar_tracking
    ${OpenCV_LIBS}
    glfw
    ${OPENGL_LIBRARIES}
    ${GLUT_LIBRARIES}
)


if(assimp_FOUND)
    target_link_libraries(baekar_legacy PUBLIC assimp::assimp)
    target_compile_definitions(baekar_legacy PUBLIC BAEKAR_HAVE_ASSIMP=1)
endif()

# ---------------------------------------------------------------------------
# Architecture layers (docs/architecture/design.md)
#   baekar_core        value types, OpenCV core only
#   baekar_application ports, AppConfig, Application facade
#   baekar_adapters    GLFW window, legacy engine pipeline
#   BaekAR             composition root (apps/baekar/main.cpp)
# ---------------------------------------------------------------------------
add_library(baekar_core INTERFACE)
target_include_directories(baekar_core INTERFACE ${CMAKE_SOURCE_DIR}/src ${OpenCV_INCLUDE_DIRS})
target_link_libraries(baekar_core INTERFACE ${OpenCV_LIBS})

add_library(baekar_application STATIC
    src/application/AppConfig.cpp
    src/application/Application.cpp
    src/application/InteractionController.cpp
)
baekar_use_modern_cpp(baekar_application)
target_compile_options(baekar_application PRIVATE -Werror)
target_link_libraries(baekar_application PUBLIC baekar_core)

add_library(baekar_adapters STATIC
    src/adapters/window/GlfwWindow.cpp
    src/adapters/frame_source/FrameSources.cpp
    src/adapters/tracking/MarkerTrackers.cpp
    src/adapters/pose/LegacyCameraPoseEstimator.cpp
    src/adapters/hand/HandTrackers.cpp
    src/adapters/render/LegacyGlRenderer.cpp
    src/adapters/scene/ContentsScene.cpp
)
baekar_use_modern_cpp(baekar_adapters)
target_link_libraries(baekar_adapters PUBLIC
    baekar_application
    baekar_legacy
    baekar_tracking
    glfw
    ${OPENGL_LIBRARIES}
)
if(APPLE)
    # The macOS camera and window capture reach the app through adapters.
    target_link_libraries(baekar_adapters PUBLIC baekar_platform_macos)
endif()

add_executable(BaekAR apps/baekar/main.cpp)
baekar_use_modern_cpp(BaekAR)
target_link_libraries(BaekAR PRIVATE baekar_adapters)

if(BUILD_TESTING)
    add_executable(BaekARUnitTests tests/unit/app_config_test.cpp)
    baekar_use_modern_cpp(BaekARUnitTests)
    target_link_libraries(BaekARUnitTests PRIVATE baekar_application)
    add_test(NAME app_config COMMAND BaekARUnitTests)
    set_tests_properties(app_config PROPERTIES LABELS "unit")

    add_executable(BaekARInteractionTests tests/unit/interaction_controller_test.cpp)
    baekar_use_modern_cpp(BaekARInteractionTests)
    target_link_libraries(BaekARInteractionTests PRIVATE baekar_application)
    add_test(NAME interaction_controller COMMAND BaekARInteractionTests)
    set_tests_properties(interaction_controller PROPERTIES LABELS "unit")

    add_executable(BaekARFrameSourceTests tests/integration/frame_source_test.cpp)
    baekar_use_modern_cpp(BaekARFrameSourceTests)
    target_compile_definitions(BaekARFrameSourceTests PRIVATE
        BAEKAR_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
    target_link_libraries(BaekARFrameSourceTests PRIVATE baekar_adapters)
    add_test(NAME frame_source_record_replay COMMAND BaekARFrameSourceTests)
    set_tests_properties(frame_source_record_replay PROPERTIES LABELS "integration")

    add_executable(BaekARLegacyCharacterizationTests
        tests/characterization/legacy_tracking_pose_test.cpp)
    baekar_use_modern_cpp(BaekARLegacyCharacterizationTests)
    target_compile_definitions(BaekARLegacyCharacterizationTests PRIVATE
        BAEKAR_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
    target_link_libraries(BaekARLegacyCharacterizationTests PRIVATE baekar_adapters)
    add_test(NAME legacy_tracking_and_pose COMMAND BaekARLegacyCharacterizationTests)
    set_tests_properties(legacy_tracking_and_pose PROPERTIES
        LABELS "characterization;tracking" TIMEOUT 60)

    add_executable(BaekARMarkerGeometryTests tests/characterization/marker_geometry_test.cpp)
    baekar_use_modern_cpp(BaekARMarkerGeometryTests)
    target_compile_definitions(BaekARMarkerGeometryTests PRIVATE
        BAEKAR_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
    target_link_libraries(BaekARMarkerGeometryTests PRIVATE baekar_legacy)
    add_test(NAME marker_geometry COMMAND BaekARMarkerGeometryTests)
    set_tests_properties(marker_geometry PROPERTIES LABELS "characterization;tracking")
endif()

# macOS: build as .app bundle so the system shows camera permission dialog
if(APPLE)
    set_target_properties(BaekAR PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Info.plist
        MACOSX_BUNDLE_BUNDLE_NAME "BaekAR"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.baekar.engine"
    )
endif()

# Runtime data next to the binary (the engine opens these cwd-relative).
set(BAEKAR_ASSETS ${CMAKE_SOURCE_DIR}/assets)
add_custom_command(TARGET BaekAR POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${BAEKAR_ASSETS}/image $<TARGET_FILE_DIR:BaekAR>/image
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${BAEKAR_ASSETS}/calibration $<TARGET_FILE_DIR:BaekAR>/calibration
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${BAEKAR_ASSETS}/database $<TARGET_FILE_DIR:BaekAR>/database
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${BAEKAR_ASSETS}/meshes $<TARGET_FILE_DIR:BaekAR>/Engine
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/legacy/MarkerlessAR/3dobjects $<TARGET_FILE_DIR:BaekAR>/3dobjects
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${BAEKAR_ASSETS}/handyar/skin.dis $<TARGET_FILE_DIR:BaekAR>/skin.dis
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${BAEKAR_ASSETS}/handyar/skin.mgm $<TARGET_FILE_DIR:BaekAR>/skin.mgm
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${BAEKAR_ASSETS}/handyar/nonskin.dis $<TARGET_FILE_DIR:BaekAR>/nonskin.dis
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${BAEKAR_ASSETS}/handyar/nonskin.mgm $<TARGET_FILE_DIR:BaekAR>/nonskin.mgm
    COMMENT "Symlinking runtime assets next to the binary"
)
