# Data Model: Sample OpenCV Camera Project on macOS

**Feature**: 002-opencv-camera-sample
**Date**: 2026-02-14

## Overview

This is a minimal single-file application with no persistent data. The "data model" consists of runtime state only.

## Runtime Entities

### CameraState

Represents the camera's current status during application execution.

| Field | Type | Description |
|-------|------|-------------|
| capture | cv::VideoCapture | The single camera capture instance |
| isOpen | bool | Whether camera opened successfully |
| timedOut | bool | Whether camera open timed out |
| frameWidth | int | Captured frame width in pixels |
| frameHeight | int | Captured frame height in pixels |
| fps | double | Actual frames per second |

**States**: UNOPENED → OPENING → OPEN / FAILED / TIMED_OUT

### ApplicationConfig

Static configuration used at startup.

| Field | Type | Description |
|-------|------|-------------|
| cameraIndex | int | Camera device index (default: 0) |
| timeoutSeconds | int | Max wait for camera open (default: 10) |
| windowName | string | Display window title |

## File Artifacts

| File | Purpose |
|------|---------|
| main.cpp | Single source file with all application logic |
| CMakeLists.txt | Build configuration |
| Info.plist | macOS camera permission metadata |
| README.md | Build and run instructions |

## Relationships

```
CMakeLists.txt → builds → main.cpp → produces → OpenCVCameraSample.app
Info.plist → embedded in → OpenCVCameraSample.app (via MACOSX_BUNDLE)
```
