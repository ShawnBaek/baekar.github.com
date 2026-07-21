cmake_minimum_required(VERSION 3.20)
if(APPLE)
    project(BaekAR LANGUAGES CXX OBJCXX)
else()
    project(BaekAR LANGUAGES CXX)
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Suppress macOS OpenGL deprecation warnings
add_definitions(-DGL_SILENCE_DEPRECATION)

# Enable HandyAR fingertip pipeline (gates wonjo_dx::bDrawHand=TRUE in wonjo.cpp).
# Without this, OnProcess never runs and the gesture subsystem stays dormant.
add_definitions(-D_DRAW_HAND)

# Find dependencies
find_package(OpenCV 4 REQUIRED)
find_package(glfw3 3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
# assimp loads the original .X mesh files (Arrow3Axis.X, hand001.X, ipad.X)
# the thesis ships under MarkerlessAR/Engine/. Optional: missing assimp falls
# back to the synthetic primitives in DrawMesh.
find_package(assimp QUIET)

# GLM is header-only; find via pkg-config or just use include path
find_path(GLM_INCLUDE_DIR glm/glm.hpp
    HINTS /opt/homebrew/include /usr/local/include
)
if(NOT GLM_INCLUDE_DIR)
    message(FATAL_ERROR "GLM not found. Install with: brew install glm")
endif()

# Source files - main engine
set(ENGINE_SOURCES
    MarkerlessAR/EngineMain.cpp
    MarkerlessAR/d3d.cpp
    MarkerlessAR/wonjo.cpp
    MarkerlessAR/cam.cpp
    MarkerlessAR/BaekAR.cpp
    MarkerlessAR/Contents.cpp
    MarkerlessAR/Detector.cpp
    MarkerlessAR/SungwookFeature.cpp
    MarkerlessAR/SungwookTransformFunctions.cpp
    MarkerlessAR/SungwookUtility.cpp
    MarkerlessAR/stdafx.cpp
)

list(APPEND ENGINE_SOURCES MarkerlessAR/Contents.cpp)

if(APPLE)
    list(APPEND ENGINE_SOURCES
        MarkerlessAR/compat/macos_camera_auth.mm
        MarkerlessAR/compat/macos_window_capture.mm
        MarkerlessAR/compat/macos_av_capture.mm
        MarkerlessAR/compat/macos_camera_menu.mm
    )
    # These .mm files capture Obj-C objects via __block; without ARC the
    # captures don't auto-retain and the autorelease pool drains them
    # before they're read (objc_msgSend crash on dangling pointer).
    set_source_files_properties(
        MarkerlessAR/compat/macos_window_capture.mm
        MarkerlessAR/compat/macos_av_capture.mm
        MarkerlessAR/compat/macos_camera_menu.mm
        PROPERTIES COMPILE_FLAGS "-fobjc-arc"
    )
endif()


# BRISK feature descriptor
set(BRISK_SOURCES
    MarkerlessAR/brisk/brisk.cpp
)

# AGAST feature detector
set(AGAST_SOURCES
    MarkerlessAR/agast/agast5_8.cc
    MarkerlessAR/agast/agast5_8_nms.cc
    MarkerlessAR/agast/agast7_12d.cc
    MarkerlessAR/agast/agast7_12d_nms.cc
    MarkerlessAR/agast/agast7_12s.cc
    MarkerlessAR/agast/agast7_12s_nms.cc
    MarkerlessAR/agast/AstDetector.cc
    MarkerlessAR/agast/nonMaximumSuppression.cc
    MarkerlessAR/agast/oast9_16.cc
    MarkerlessAR/agast/oast9_16_nms.cc
)

# Calibration module
set(CALIBRATION_SOURCES
    # MarkerlessAR/calibration/BaekARCamera.cpp  # Excluded — duplicate CCamera class, only used by its own .cpp
    MarkerlessAR/calibration/Camera.cpp
    MarkerlessAR/calibration/cvFindExtrinsicCameraParams3.cpp
)

# Kato Pose Estimation
set(KATO_SOURCES
    MarkerlessAR/KatoPoseEstimation/KatoPoseEstimator.cpp
    MarkerlessAR/KatoPoseEstimation/KatoPoseEstimatorUtils.cpp
    MarkerlessAR/KatoPoseEstimation/PoseEstimator.cpp
)

# HandyAR hand tracking
set(HANDYAR_SOURCES
    MarkerlessAR/HandyAR/Capture.cpp
    MarkerlessAR/HandyAR/ColorDistribution.cpp
    MarkerlessAR/HandyAR/ColorHistogram.cpp
    MarkerlessAR/HandyAR/FingerTip.cpp
    MarkerlessAR/HandyAR/FingertipPoseEstimation.cpp
    MarkerlessAR/HandyAR/FingertipTracker.cpp
    # MarkerlessAR/HandyAR/FlyCap.cpp  # Excluded — requires PointGrey SDK
    MarkerlessAR/HandyAR/HandRegion.cpp
    MarkerlessAR/HandyAR/MixGaussian.cpp
    MarkerlessAR/HandyAR/PoseEstimation.cpp
    MarkerlessAR/compat/multi_marker_detector.cpp
    MarkerlessAR/compat/synthetic_marker_source.cpp
)

# Combine all sources
set(ALL_SOURCES
    ${ENGINE_SOURCES}
    ${BRISK_SOURCES}
    ${AGAST_SOURCES}
    ${CALIBRATION_SOURCES}
    ${KATO_SOURCES}
    ${HANDYAR_SOURCES}
)

add_executable(BaekAR ${ALL_SOURCES})

target_include_directories(BaekAR PRIVATE
    ${CMAKE_SOURCE_DIR}/MarkerlessAR
    ${CMAKE_SOURCE_DIR}/MarkerlessAR/brisk
    ${CMAKE_SOURCE_DIR}/MarkerlessAR/agast
    ${CMAKE_SOURCE_DIR}/MarkerlessAR/calibration
    ${CMAKE_SOURCE_DIR}/MarkerlessAR/KatoPoseEstimation
    ${CMAKE_SOURCE_DIR}/MarkerlessAR/HandyAR
    ${OpenCV_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
    ${GLUT_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIR}
)

target_link_libraries(BaekAR PRIVATE
    ${OpenCV_LIBS}
    glfw
    ${OPENGL_LIBRARIES}
    ${GLUT_LIBRARIES}
)

if(assimp_FOUND)
    target_link_libraries(BaekAR PRIVATE assimp::assimp)
    target_compile_definitions(BaekAR PRIVATE BAEKAR_HAVE_ASSIMP=1)
endif()

if(APPLE)
    target_link_libraries(BaekAR PRIVATE
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

# macOS: build as .app bundle so the system shows camera permission dialog
if(APPLE)
    set_target_properties(BaekAR PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Info.plist
        MACOSX_BUNDLE_BUNDLE_NAME "BaekAR"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.baekar.engine"
    )
endif()

# Copy asset directories to build output for runtime access
add_custom_command(TARGET BaekAR POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/MarkerlessAR/image
        $<TARGET_FILE_DIR:BaekAR>/image
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/MarkerlessAR/calibration
        $<TARGET_FILE_DIR:BaekAR>/calibration
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/MarkerlessAR/database
        $<TARGET_FILE_DIR:BaekAR>/database
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/MarkerlessAR/3dobjects
        $<TARGET_FILE_DIR:BaekAR>/3dobjects
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/MarkerlessAR/Engine
        $<TARGET_FILE_DIR:BaekAR>/Engine
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/MarkerlessAR/skin.dis
        $<TARGET_FILE_DIR:BaekAR>/skin.dis
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/MarkerlessAR/skin.mgm
        $<TARGET_FILE_DIR:BaekAR>/skin.mgm
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/MarkerlessAR/nonskin.dis
        $<TARGET_FILE_DIR:BaekAR>/nonskin.dis
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/MarkerlessAR/nonskin.mgm
        $<TARGET_FILE_DIR:BaekAR>/nonskin.mgm
    COMMENT "Symlinking asset directories and data files to build output"
)
