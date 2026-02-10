# Quickstart: Building BackAR on macOS

**Branch**: `001-macos-port` | **Date**: 2026-02-08

## Prerequisites

- macOS 13 (Ventura) or later
- Xcode Command Line Tools (`xcode-select --install`)
- Homebrew (`https://brew.sh`)

## Install Dependencies

```bash
brew install cmake opencv glfw glm freeglut
```

## Build

```bash
cd BackAR
mkdir build && cd build
cmake ..
make -j$(sysctl -n hw.ncpu)
```

## Run

```bash
# From the build directory
./BackAR
```

**Note**: macOS will prompt for camera access permission on first run. Grant access to enable AR functionality.

## Test AR Functionality

1. Launch the application
2. Print or display one of the reference images from `MarkerlessAR/image/` on a screen or paper
3. Point your camera at the reference image
4. Observe 3D object overlay appearing on the detected image

## Troubleshooting

- **Camera not found**: Ensure camera permissions are granted in System Settings > Privacy & Security > Camera
- **Build errors with OpenCV**: Verify OpenCV 4.x is installed: `brew info opencv`
- **GLFW not found**: Verify installation: `brew info glfw`
- **OpenGL deprecation warnings**: Expected on macOS; the legacy OpenGL profile still works
