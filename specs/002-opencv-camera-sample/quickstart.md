# Quickstart: Sample OpenCV Camera Project on macOS

**Feature**: 002-opencv-camera-sample
**Date**: 2026-02-14

## Prerequisites

- macOS 13 (Ventura) or later
- Xcode command-line tools: `xcode-select --install`
- Homebrew: https://brew.sh

## Install Dependencies

```bash
brew install opencv cmake
```

## Build

```bash
mkdir build && cd build
cmake ..
make
```

## Run

```bash
# Run the app bundle (required for camera permissions)
open OpenCVCameraSample.app

# Or run directly (may not get camera permission on first launch)
./OpenCVCameraSample.app/Contents/MacOS/OpenCVCameraSample
```

## Expected Behavior

1. A window opens showing your live camera feed
2. Camera resolution and FPS are displayed in the window title
3. Press **ESC** or **Q** to quit

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Camera permission denied | System Settings > Privacy & Security > Camera > Enable for OpenCVCameraSample |
| Camera hangs on open | App times out after 10 seconds. Try: `sudo killall -9 appleh13camerad cameracaptured VDCAssistant` |
| No camera found | Check that a camera is connected. External USB cameras may need a driver. |
| Build fails: OpenCV not found | Run `brew install opencv` and retry |
| Build fails: CMake not found | Run `brew install cmake` and retry |
