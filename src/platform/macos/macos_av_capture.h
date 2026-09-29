#pragma once

#ifdef __APPLE__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Native AVFoundation capture using the modern AVCaptureDeviceDiscoverySession.
// Replaces cv::VideoCapture on macOS — OpenCV's AVFoundation backend uses
// the deprecated devicesWithMediaType: API which doesn't enumerate iPhone
// Continuity Camera. This helper does, by indexing the same enumerated
// device list as PickCameraIndex (compat/macos_camera_auth.mm).

typedef struct AVCap AVCap;

// Open the camera at the given index from the discovery enumeration
// (built-in, external USB, Continuity Camera). Pass -1 for the default.
// Frames are configured for BGRA 32-bit at the requested width/height
// (AVFoundation may snap to a supported preset).
AVCap* AVCap_Open(int index, int width, int height);

// Copies the latest captured frame into `bgr` (size = w*h*3, packed BGR).
// Returns false if no fresh frame is ready since the last call.
// `width`/`height` must match what AVCap_Open returned (or report 0 if
// caller wants to skip the size check).
bool   AVCap_GetLatestBGR(AVCap*, uint8_t* bgr, int width, int height);

// Returns the actual frame size AVFoundation is delivering (e.g. 1920x1080
// for iPhone Continuity Camera). The caller should resize as needed.
void   AVCap_GetActualSize(AVCap*, int* outW, int* outH);

void   AVCap_Close(AVCap*);

#ifdef __cplusplus
}
#endif

#endif // __APPLE__
