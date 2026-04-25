# Feature Specification: Sample OpenCV Camera Project on macOS

**Feature Branch**: `002-opencv-camera-sample`
**Created**: 2026-02-14
**Status**: Draft
**Input**: User description: "make a sample opencv project on mac to open camera successfully"

## Context

Creating a minimal, standalone sample project that demonstrates how to correctly open and display a live camera feed using OpenCV on macOS. This serves as a reference implementation and verification tool — proving that the camera pipeline works before integrating into larger projects like BackAR. macOS has well-known AVFoundation quirks with OpenCV's VideoCapture that require specific workarounds (env vars, Info.plist keys, timeout handling).

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Open Camera and Display Feed (Priority: P1)

As a developer on macOS, I want to run a sample application that opens the camera and displays a live video feed in a window, so that I can verify my OpenCV installation and camera access work correctly.

**Why this priority**: This is the entire purpose of the project. Without a working camera feed, there is nothing to demonstrate.

**Independent Test**: Build and run the sample binary. A window appears showing the live camera feed. Press ESC or Q to quit cleanly.

**Acceptance Scenarios**:

1. **Given** a macOS machine with a camera and OpenCV installed, **When** the user builds and runs the sample, **Then** a window opens displaying the live camera feed.
2. **Given** the application is running with a live camera feed, **When** the user presses ESC or Q, **Then** the window closes and the application exits cleanly without hanging.
3. **Given** the application is running, **When** the camera feed is active, **Then** the video displays smoothly without visible lag or frame drops.

---

### User Story 2 - Handle Camera Unavailability Gracefully (Priority: P2)

As a developer on macOS, I want the application to display a clear error message when the camera is unavailable (denied permissions, no hardware, or stuck daemon), so that I can diagnose the problem without guessing.

**Why this priority**: Camera access fails frequently on macOS due to permissions, Continuity Camera issues, and daemon crashes. Clear diagnostics save significant debugging time.

**Independent Test**: Deny camera permission (or cover/disconnect camera), launch the application, and verify it prints a diagnostic message and exits gracefully instead of hanging or crashing.

**Acceptance Scenarios**:

1. **Given** camera permission is denied in System Settings, **When** the user launches the application, **Then** a clear error message is displayed indicating the permission issue and how to fix it.
2. **Given** no camera hardware is available, **When** the user launches the application, **Then** a clear error message is displayed indicating no camera was found.
3. **Given** the camera daemon is stuck (open times out), **When** the user launches the application, **Then** the application times out after a reasonable period and reports the issue rather than hanging indefinitely.

---

### User Story 3 - Build from Source with Minimal Steps (Priority: P3)

As a developer on macOS, I want to build the sample project with a single build command after installing OpenCV, so that I can get started quickly without configuring complex build systems.

**Why this priority**: A sample project that is hard to build defeats its purpose as a quick verification tool.

**Independent Test**: On a fresh macOS environment with Homebrew OpenCV installed, follow the README and build the project in under 2 minutes.

**Acceptance Scenarios**:

1. **Given** macOS with Xcode command-line tools and Homebrew OpenCV installed, **When** the user runs the documented build command, **Then** the project compiles and links without errors.
2. **Given** a missing dependency (e.g., OpenCV not installed), **When** the user runs the build, **Then** the build system reports the missing dependency clearly.

---

### Edge Cases

- What happens when the camera open call hangs (macOS daemon crash)? The application should time out and report the issue.
- What happens when multiple cameras are available (e.g., built-in + Continuity Camera)? The application should open the default camera (index 0).
- What happens on macOS 15+ where Continuity Camera is the default? The application should handle this via Info.plist configuration.
- What happens if OpenCV was built without AVFoundation support? The build should succeed but camera open should fail with a clear message.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The project MUST compile on macOS 13+ using Clang and a standard build tool with Homebrew OpenCV
- **FR-002**: The application MUST open the default camera and display a live video feed in a window
- **FR-003**: The application MUST exit cleanly when the user presses ESC or Q
- **FR-004**: The application MUST work around the macOS AVFoundation authorization blocking issue to prevent the camera open call from hanging
- **FR-005**: The application MUST include macOS metadata for camera permission prompts and Continuity Camera support on macOS 15+
- **FR-006**: The application MUST implement a timeout on camera open (no longer than 15 seconds) to prevent indefinite hanging
- **FR-007**: The application MUST display a clear, actionable error message when the camera cannot be opened, including guidance on checking system privacy settings
- **FR-008**: The application MUST display the camera resolution and FPS in the window title or on-screen overlay

### Key Entities

- **Sample Application**: A single-file program with a build configuration that builds an executable demonstrating camera access on macOS
- **Info.plist**: macOS application metadata file with camera permission keys
- **Build Configuration**: Build file finding OpenCV and configuring the macOS app bundle

## Assumptions

- macOS 13 (Ventura) or later is the target platform
- OpenCV 4.x is installed via Homebrew (`brew install opencv`)
- Xcode command-line tools are installed for the compiler
- A build tool (CMake or similar) is available
- The project is a standalone sample, not integrated into BackAR
- The sample uses OpenCV's built-in window display for showing the camera feed, keeping dependencies minimal
- Camera index 0 is the default camera to open
- The project targets a single source file plus build configuration plus Info.plist for maximum simplicity

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: The project compiles with zero errors on macOS 13+ using the documented build steps
- **SC-002**: The application opens a window displaying a live camera feed within 5 seconds of launch
- **SC-003**: The application exits cleanly (no hang, no crash) when the user presses ESC or Q
- **SC-004**: When the camera is unavailable, the application displays a diagnostic message and exits within 15 seconds (no indefinite hang)
- **SC-005**: A developer can install dependencies, build, and run the sample by following the README in under 5 minutes
