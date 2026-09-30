#pragma once

#ifdef __APPLE__

#ifdef __cplusplus
extern "C" {
#endif

// Type of the callback fired when the user picks a camera from the menu.
typedef void (*CameraSwitchCallback)(int newDeviceIndex);

// Install a "Camera" menu in the macOS menu bar that lists every video
// device from AVCaptureDeviceDiscoverySession (same enumeration as
// PickCameraIndex / AVCap_Open). Selecting a device fires the callback
// with its 0-based index. Indices match AVCap_Open's. Initial check
// mark is placed on `currentIndex`.
void InstallCameraMenu(int currentIndex, CameraSwitchCallback cb);

#ifdef __cplusplus
}
#endif

#endif // __APPLE__
