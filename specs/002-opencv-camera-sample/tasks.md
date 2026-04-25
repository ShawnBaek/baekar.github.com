# Tasks: Sample OpenCV Camera Project on macOS

**Input**: Design documents from `/specs/002-opencv-camera-sample/`
**Prerequisites**: plan.md (required), spec.md (required), research.md, data-model.md, quickstart.md

**Tests**: Not requested — manual verification (build, launch, verify camera feed).

**Organization**: Tasks are grouped by user story to enable independent implementation and testing. Each phase produces a buildable/runnable increment.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (US1, US2, US3)
- Include exact file paths in descriptions

---

## Phase 1: Setup (Project Structure + Build System)

**Purpose**: Create the project directory, CMakeLists.txt, and Info.plist so the project can be configured and built on macOS.

- [x] T001 Create directory samples/opencv-camera/ at repository root
- [x] T002 [P] Create samples/opencv-camera/CMakeLists.txt with cmake_minimum_required(3.20), project(OpenCVCameraSample), set C++14 standard, find_package(OpenCV 4 REQUIRED), add_executable for main.cpp, set MACOSX_BUNDLE TRUE with MACOSX_BUNDLE_INFO_PLIST pointing to Info.plist, link ${OpenCV_LIBS}
- [x] T003 [P] Create samples/opencv-camera/Info.plist with CFBundleExecutable (OpenCVCameraSample), CFBundleIdentifier (com.backar.opencv-camera-sample), NSCameraUsageDescription ("This app needs camera access to display a live video feed."), NSCameraUseContinuityCameraDeviceType (true), NSHighResolutionCapable (true)
- [x] T004 Verify Phase 1: run `mkdir build && cd build && cmake ..` inside samples/opencv-camera/ and confirm CMake configuration succeeds (source won't compile yet since main.cpp doesn't exist)

**Checkpoint**: CMake configures successfully. Build system is ready.

---

## Phase 2: User Story 1 — Open Camera and Display Feed (Priority: P1)

**Goal**: Create main.cpp that opens the camera with all macOS AVFoundation workarounds and displays a live feed in a window using cv::imshow. ESC or Q quits cleanly.

**Independent Test**: Build and run the binary. A window appears showing the live camera feed. Press ESC or Q to quit.

- [x] T005 [US1] Create samples/opencv-camera/main.cpp with the following structure: includes (opencv2/opencv.hpp, thread, atomic, chrono, cstdlib, cstdio), main() function that: (1) calls setenv("OPENCV_AVFOUNDATION_SKIP_AUTH", "1", 1) as the first line, (2) creates cv::VideoCapture object, (3) opens camera index 0 in a detached std::thread with std::atomic<bool> signaling and 10-second timeout polling at 100ms intervals, (4) checks isOpened() and prints diagnostic if failed, (5) enters frame loop: cap.read(frame), cv::imshow("OpenCV Camera Sample", frame), cv::waitKey(1) checking for ESC(27) or 'q'(113), (6) releases capture and destroys windows on exit
- [x] T006 [US1] Add FPS and resolution display to samples/opencv-camera/main.cpp — after camera opens, read CAP_PROP_FRAME_WIDTH and CAP_PROP_FRAME_HEIGHT; in the frame loop, compute FPS from frame timestamps using std::chrono; update window title via cv::setWindowTitle() with format "OpenCV Camera Sample — 640x480 @ 30.0 FPS"
- [x] T007 [US1] Verify US1: build with `cd build && cmake .. && make`, run with `open OpenCVCameraSample.app`, confirm camera feed displays in window with resolution/FPS in title bar, confirm ESC and Q both quit cleanly

**Checkpoint**: Camera opens, feed displays, ESC/Q quits. US1 complete.

---

## Phase 3: User Story 2 — Handle Camera Unavailability Gracefully (Priority: P2)

**Goal**: Add clear, actionable error messages for all camera failure modes: permission denied, no hardware, daemon stuck (timeout).

**Independent Test**: Deny camera permission or disconnect camera, launch the app, verify diagnostic message appears and app exits gracefully within 15 seconds.

- [x] T008 [US2] Add camera error diagnostics to samples/opencv-camera/main.cpp — after timeout or isOpened() failure, print to stderr: (1) "ERROR: Could not open camera (index 0).", (2) "Possible causes:", (3) "  - Camera permission denied: System Settings > Privacy & Security > Camera", (4) "  - No camera hardware detected", (5) "  - Camera daemon stuck: try 'sudo killall -9 appleh13camerad cameracaptured VDCAssistant'", (6) return exit code 1
- [x] T009 [US2] Add timeout-specific messaging to samples/opencv-camera/main.cpp — if the 10-second timeout expires before VideoCapture::open() returns, print "ERROR: Camera open timed out after 10 seconds. The camera daemon may be stuck." before the general diagnostics
- [x] T010 [US2] Verify US2: build and run with camera permission denied (or camera disconnected), confirm diagnostic messages appear on stderr and app exits with code 1 within 15 seconds

**Checkpoint**: All failure modes produce clear diagnostics. US2 complete.

---

## Phase 4: User Story 3 — Build from Source with Minimal Steps (Priority: P3)

**Goal**: Write README.md with prerequisites, install, build, run, and troubleshooting instructions so any macOS developer can get the sample running in under 5 minutes.

**Independent Test**: Follow README on a fresh environment — install deps, build, run, verify camera feed.

- [x] T011 [US3] Create samples/opencv-camera/README.md with sections: (1) "OpenCV Camera Sample for macOS" title and one-line description, (2) "Prerequisites" listing macOS 13+, Xcode CLI tools, Homebrew, (3) "Install Dependencies" with `brew install opencv cmake`, (4) "Build" with `mkdir build && cd build && cmake .. && make`, (5) "Run" with `open OpenCVCameraSample.app` and note about first-launch permission dialog, (6) "Expected Behavior" describing window with camera feed and FPS in title, ESC/Q to quit, (7) "Troubleshooting" table covering: camera permission denied, camera hangs, no camera found, OpenCV not found, CMake not found
- [x] T012 [US3] Verify US3: follow README from scratch — install deps, build, run, confirm the documented steps work end-to-end

**Checkpoint**: README enables fresh build in under 5 minutes. US3 complete.

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1 (Setup)**: No dependencies — start immediately
- **Phase 2 (US1 - Camera Feed)**: Depends on Phase 1 — needs CMakeLists.txt and Info.plist
- **Phase 3 (US2 - Error Handling)**: Depends on Phase 2 — adds error handling to existing main.cpp
- **Phase 4 (US3 - README)**: Depends on Phase 2 — needs working app to document; can run in parallel with Phase 3

### Within Each Phase

- Tasks marked [P] within the same phase can run in parallel
- Sequential tasks (no [P]) depend on prior tasks in that phase
- Verification tasks (T004, T007, T010, T012) must run after all preceding tasks in their phase

### Parallel Opportunities

**Phase 1** (setup parallelizable):
```
Parallel group: T002, T003 (independent files — CMakeLists.txt and Info.plist)
```

**Phase 3 + Phase 4** (can overlap):
```
US2 (T008-T010) and US3 (T011-T012) can run in parallel since US2 modifies main.cpp while US3 creates README.md
```

---

## Implementation Strategy

### MVP First (US1 Only — Phase 1 + Phase 2)

1. Complete Phase 1: Project structure + build system
2. Complete Phase 2: Camera open + display + quit
3. **STOP and VALIDATE**: Build, run, camera feed visible, ESC quits
4. MVP complete — camera works on macOS

### Incremental Delivery

1. Phase 1 + Phase 2 → Camera feed working (US1 MVP)
2. Phase 3 → Error handling added (US2)
3. Phase 4 → README written (US3, feature complete)

---

## Notes

- Total tasks: 12 (T001-T012)
- Tasks per story: Setup=4, US1=3, US2=3, US3=2
- [P] tasks = different files, no dependencies within phase
- No test tasks (manual verification per phase)
- Commit after each task or logical group
- The entire project is 4 files: main.cpp, CMakeLists.txt, Info.plist, README.md
