#pragma once

#ifdef __APPLE__

#ifdef __cplusplus
extern "C" {
#endif

bool RequestCameraPermission(void);

// Interactive stdin/stderr picker over AVFoundation video devices.
// Returns the chosen device's OpenCV index (0-based, matches the order
// AVCaptureDevice.devices reports — same order cv::VideoCapture(N) uses).
// Returns -1 to keep cv::VideoCapture's default (index 0).
int  PickCameraIndex(void);

#ifdef __cplusplus
}
#endif

#endif
