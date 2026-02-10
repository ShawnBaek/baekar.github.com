# Feature Specification: macOS Port of BackAR

**Feature Branch**: `001-macos-port`
**Created**: 2026-02-08
**Status**: Draft
**Input**: Port BackAR (C++ Markerless AR Engine with 6DoF Hand Pose Estimation) from Windows/Visual Studio 2010 to compile and run on modern macOS. No refactoring, no revamp, no new features.

## Context

BackAR is a ~13-year-old C++ augmented reality engine originally built for Windows with Visual Studio 2010. It uses DirectX 9, OpenCV 2.3.1, Win32 API, and GLUT across ~83 source files. The project includes markerless feature tracking (BRISK/AGAST), pose estimation (Kato algorithm), 6DoF hand tracking (HandyAR), and 3D object rendering.

The goal is strictly to make the existing codebase compile and run on modern macOS. No architectural changes, no feature additions, no code modernization beyond what is necessary for compilation.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Build from Source on macOS (Priority: P1)

As a developer on macOS, I want to clone the BackAR repository and build it from source so that I get a running executable without manual dependency hunting or build errors.

**Why this priority**: Without a successful build, nothing else works. This is the foundational gate for all other stories.

**Independent Test**: Clone the repo on a clean macOS machine, install dependencies via a package manager, run the build command, and verify a binary is produced with zero compilation errors.

**Acceptance Scenarios**:

1. **Given** a macOS machine (macOS 13+) with Xcode command-line tools installed, **When** the developer follows the README build instructions, **Then** all dependencies are installed and the project compiles without errors.
2. **Given** the project is cloned fresh, **When** the developer runs the build system, **Then** all ~83 source files compile and link into an executable.
3. **Given** a successful build, **When** the developer inspects compiler output, **Then** there are no unresolved symbols or missing header errors.

---

### User Story 2 - Run AR Engine on macOS (Priority: P2)

As a developer on macOS, I want to launch the built BackAR application so that it opens a window, captures camera input, and displays the AR scene (feature detection + 3D overlay on a reference image).

**Why this priority**: A binary that compiles but doesn't run provides no value. Running the engine validates the port end-to-end.

**Independent Test**: Launch the built executable, point the camera at one of the bundled reference images, and verify that the AR overlay (3D object) appears on screen.

**Acceptance Scenarios**:

1. **Given** a successfully built BackAR binary, **When** the user launches it, **Then** a window opens displaying the camera feed.
2. **Given** the application is running with a camera feed, **When** the user points the camera at a known reference image (from the `image/` directory), **Then** the application detects features and overlays a 3D object on the reference image.
3. **Given** the application is running, **When** the user moves the camera around the reference image, **Then** the 3D overlay tracks the image in real-time with stable pose estimation.

---

### User Story 3 - Updated README with Build Instructions (Priority: P3)

As a developer discovering BackAR, I want a clear README that explains what the project is, how to install dependencies, and how to build and run it on macOS so that I can get started without guessing.

**Why this priority**: Documentation ensures the port is usable by others, not just the original developer. Lower priority because the software must work first.

**Independent Test**: Give the README to a developer unfamiliar with the project and verify they can build and run BackAR by following only the documented steps.

**Acceptance Scenarios**:

1. **Given** a developer reads the README, **When** they follow the dependency installation steps, **Then** all required libraries are installed on their macOS system.
2. **Given** a developer reads the README, **When** they follow the build steps, **Then** the project compiles successfully.
3. **Given** a developer reads the README, **When** they follow the run instructions, **Then** the application launches and displays the camera feed with AR capability.

---

### Edge Cases

- What happens when no camera is connected? The application should display an error message rather than crashing.
- What happens when OpenCV cannot find a camera on macOS (privacy permissions)? The application should report the issue clearly.
- What happens when a required dependency (OpenCV, GLFW) is not installed? The build system should fail with a clear error message indicating the missing dependency.
- What happens when reference images or calibration data files are missing from expected paths? The application should report missing files rather than segfaulting.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Project MUST compile on macOS 13+ using Clang (Xcode command-line tools) without errors
- **FR-002**: Project MUST use a cross-platform build system (CMake) to replace Visual Studio solution files
- **FR-003**: All DirectX 9 rendering code MUST be replaced with OpenGL equivalents that work on macOS
- **FR-004**: All Win32 API window management (CreateWindow, RegisterClass, MSG loop, WndProc) MUST be replaced with a cross-platform windowing library
- **FR-005**: All Windows-specific threading (\_beginthreadex, HANDLE, CRITICAL\_SECTION) MUST be replaced with C++ standard threading primitives
- **FR-006**: All OpenCV 2.3.1 API calls MUST be updated to work with OpenCV 4.x, including migration from legacy C API (IplImage, CvMat) to modern C++ API (cv::Mat)
- **FR-007**: All MSVC-specific code MUST be replaced with standard C++ equivalents (stdext::hash\_map, \_\_forceinline, ATL headers)
- **FR-008**: OpenGL/GLUT headers MUST use macOS-compatible include paths
- **FR-009**: All hardcoded Windows file paths MUST be replaced with relative or configurable paths
- **FR-010**: The application MUST open a window and display live camera feed on macOS
- **FR-011**: Feature detection (BRISK/AGAST) MUST function and detect features in camera frames
- **FR-012**: Pose estimation MUST compute and apply 3D transformations when a reference image is detected
- **FR-013**: The application MUST render 3D objects overlaid on detected reference images
- **FR-014**: The README MUST document prerequisites, dependency installation, build steps, and run instructions for macOS
- **FR-015**: Existing functionality MUST be preserved — no features removed, no behavior changed beyond platform adaptation

### Key Entities

- **Build Configuration**: CMakeLists.txt defining source files, include paths, linked libraries, and compiler flags for macOS
- **Reference Images**: Existing images in `image/` directory used for feature matching and AR tracking
- **Calibration Data**: Camera parameters and configuration in `calibration/` directory
- **3D Assets**: Bundled models (bunny, iPad) used for AR overlay rendering

## Assumptions

- macOS 13 (Ventura) or later is the target platform, using Apple Clang via Xcode command-line tools
- OpenCV 4.x will be installed via Homebrew (`brew install opencv`)
- GLFW will be used as the windowing library replacement (lightweight, widely available via Homebrew)
- OpenGL (legacy profile available on macOS) will be used for rendering, replacing DirectX 9
- The OpenCV GPU module usage (cv::gpu::) will be disabled or stubbed out, as CUDA is not available on macOS
- Camera access will use OpenCV's VideoCapture which handles macOS AVFoundation internally
- The embedded BRISK/AGAST implementations will be used rather than OpenCV's built-in versions, to preserve original behavior
- SSE intrinsics (emmintrin.h) are supported by Apple Clang and will work without changes
- Sprint-based delivery: each sprint produces a PR that is verified as compilable and runnable on macOS

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: The project compiles with zero errors on macOS 13+ using the documented build steps
- **SC-002**: The built application launches and displays a live camera feed in a window on macOS
- **SC-003**: When pointed at a bundled reference image, the application detects features and overlays a 3D object within 3 seconds of viewing
- **SC-004**: The application runs stably for at least 5 minutes of continuous use without crashing
- **SC-005**: A new developer can install dependencies, build, and run the application by following only the README, in under 15 minutes
- **SC-006**: All original AR capabilities (feature detection, pose estimation, 3D rendering) function on macOS as they did on Windows
