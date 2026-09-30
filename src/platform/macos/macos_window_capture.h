#pragma once

#ifdef __APPLE__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// One-off interactive picker (text-mode on stdin/stderr).
// Returns the chosen window's CGWindowID, or 0 if cancelled / no windows.
uint32_t WCPicker_PickWindowID(void);

// Streaming capture of a chosen window via ScreenCaptureKit.
// Frames are delivered on an internal dispatch queue and copied into a
// triple-buffered ring; consumers read the latest frame via WCStream_LatestFrame.
typedef struct WCStream WCStream;

WCStream* WCStream_Open(uint32_t cgWindowID, int width, int height);

// Copies the most recent frame into `bgra` (size = width*height*4).
// Returns true if a frame was available since last call (or a fresh one is
// ready), false otherwise. The caller's `width`/`height` must match
// WCStream_Open.
bool WCStream_LatestFrame(WCStream*, uint8_t* bgra, int width, int height);

void WCStream_Close(WCStream*);

#ifdef __cplusplus
}
#endif

#endif // __APPLE__
