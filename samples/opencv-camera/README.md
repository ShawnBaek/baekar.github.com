# OpenCV Camera Sample for macOS

Minimal sample demonstrating how to correctly open and display a live camera feed using OpenCV on macOS, with all necessary AVFoundation workarounds.

## Prerequisites

- macOS 13 (Ventura) or later
- Xcode command-line tools: `xcode-select --install`
- [Homebrew](https://brew.sh)

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
# Launch as .app bundle (required for camera permission dialog on first run)
open OpenCVCameraSample.app
```

On first launch, macOS will ask for camera permission. Click **Allow**.

## Expected Behavior

1. A window opens showing your live camera feed
2. The window title shows resolution and FPS (e.g., "OpenCV Camera Sample — 1280x720 @ 30.0 FPS")
3. Press **ESC** or **Q** to quit

## macOS Camera Workarounds

This sample includes three critical workarounds for OpenCV camera access on macOS:

1. **`OPENCV_AVFOUNDATION_SKIP_AUTH=1`** — Prevents `VideoCapture::open()` from blocking on AVFoundation authorization (OpenCV issue #7519)
2. **Info.plist with `NSCameraUseContinuityCameraDeviceType`** — Required for macOS 15+ Continuity Camera support
3. **10-second timeout** — Opens camera in a background thread with timeout to prevent indefinite hang if the camera daemon is stuck

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Camera permission denied | System Settings > Privacy & Security > Camera > Enable for OpenCVCameraSample |
| Camera hangs on open | App times out after 10s. Try: `sudo killall -9 appleh13camerad cameracaptured VDCAssistant` |
| No camera found | Check that a camera is connected. External USB cameras may need a driver. |
| Build fails: OpenCV not found | Run `brew install opencv` and retry |
| Build fails: CMake not found | Run `brew install cmake` and retry |
