#include "macos_window_capture.h"

#import <Foundation/Foundation.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>
#import <dispatch/dispatch.h>

#include <atomic>
#include <mutex>
#include <vector>
#include <cstdio>

// ---- Picker ----

uint32_t WCPicker_PickWindowID(void)
{
    __block NSArray<SCWindow*>* windows = nil;
    __block NSError* err = nil;
    dispatch_semaphore_t sema = dispatch_semaphore_create(0);

    [SCShareableContent getShareableContentExcludingDesktopWindows:YES
                                            onScreenWindowsOnly:YES
                                              completionHandler:^(SCShareableContent* content, NSError* e) {
        windows = content.windows;
        err = e;
        dispatch_semaphore_signal(sema);
    }];
    dispatch_semaphore_wait(sema, dispatch_time(DISPATCH_TIME_NOW, 10LL * NSEC_PER_SEC));

    if (err || windows.count == 0) {
        fprintf(stderr, "WCPicker: no shareable windows (err=%s)\n",
                err ? err.localizedDescription.UTF8String : "none");
        return 0;
    }

    fprintf(stderr, "\n=== Pick a window to render as an AR texture ===\n");
    NSUInteger n = MIN((NSUInteger)20, windows.count);
    for (NSUInteger i = 0; i < n; ++i) {
        SCWindow* w = windows[i];
        const char* app = w.owningApplication.applicationName.UTF8String ?: "?";
        const char* title = w.title.UTF8String ?: "(no title)";
        fprintf(stderr, "  [%2lu] %s — %s  (%dx%d)\n",
                (unsigned long)i, app, title,
                (int)w.frame.size.width, (int)w.frame.size.height);
    }
    fprintf(stderr, "Enter index (0-%lu), or anything else to skip: ", (unsigned long)(n-1));
    fflush(stderr);

    char line[64] = {0};
    if (!fgets(line, sizeof(line), stdin)) return 0;
    int idx = -1;
    if (sscanf(line, "%d", &idx) != 1 || idx < 0 || idx >= (int)n) {
        fprintf(stderr, "WCPicker: skipped\n");
        return 0;
    }

    SCWindow* picked = windows[idx];
    fprintf(stderr, "WCPicker: chose [%d] %s — %s (windowID=%u)\n",
            idx,
            picked.owningApplication.applicationName.UTF8String ?: "?",
            picked.title.UTF8String ?: "(no title)",
            (unsigned)picked.windowID);
    return picked.windowID;
}

// ---- Stream output delegate ----

@interface WCStreamOutput : NSObject <SCStreamOutput, SCStreamDelegate>
@property (nonatomic, assign) WCStream* owner;
@end

// ---- WCStream ----

struct WCStream {
    int width = 0;
    int height = 0;

    SCStream* stream = nil;
    WCStreamOutput* output = nil;
    dispatch_queue_t queue = nullptr;

    std::mutex mtx;
    std::vector<uint8_t> latest;     // BGRA
    std::atomic<bool> hasFrame{false};
};

@implementation WCStreamOutput
- (void)stream:(SCStream*)stream didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer ofType:(SCStreamOutputType)type {
    if (type != SCStreamOutputTypeScreen || !self.owner) return;
    CVPixelBufferRef pb = CMSampleBufferGetImageBuffer(sampleBuffer);
    if (!pb) return;
    CVPixelBufferLockBaseAddress(pb, kCVPixelBufferLock_ReadOnly);
    size_t srcW = CVPixelBufferGetWidth(pb);
    size_t srcH = CVPixelBufferGetHeight(pb);
    size_t srcStride = CVPixelBufferGetBytesPerRow(pb);
    const uint8_t* src = (const uint8_t*)CVPixelBufferGetBaseAddress(pb);

    // Resize source to target width/height with nearest-neighbor (kept simple
    // — ScreenCaptureKit downsamples to the configured size when it can, so
    // this rescale path is the fallback when stride/dimensions don't match).
    int dstW = self.owner->width;
    int dstH = self.owner->height;
    std::vector<uint8_t> buf(dstW * dstH * 4);

    if ((int)srcW == dstW && (int)srcH == dstH && (int)srcStride == dstW * 4) {
        memcpy(buf.data(), src, dstW * dstH * 4);
    } else {
        for (int y = 0; y < dstH; ++y) {
            int sy = (int)((int64_t)y * srcH / dstH);
            const uint8_t* srow = src + sy * srcStride;
            uint8_t* drow = buf.data() + y * dstW * 4;
            for (int x = 0; x < dstW; ++x) {
                int sx = (int)((int64_t)x * srcW / dstW);
                memcpy(drow + x * 4, srow + sx * 4, 4);
            }
        }
    }

    CVPixelBufferUnlockBaseAddress(pb, kCVPixelBufferLock_ReadOnly);

    {
        std::lock_guard<std::mutex> lk(self.owner->mtx);
        self.owner->latest.swap(buf);
    }
    self.owner->hasFrame.store(true);
}
- (void)stream:(SCStream*)stream didStopWithError:(NSError*)error {
    fprintf(stderr, "WCStream: stopped (err=%s)\n",
            error ? error.localizedDescription.UTF8String : "none");
}
@end

WCStream* WCStream_Open(uint32_t cgWindowID, int width, int height)
{
    __block SCWindow* match = nil;
    dispatch_semaphore_t sema = dispatch_semaphore_create(0);
    [SCShareableContent getShareableContentExcludingDesktopWindows:YES
                                            onScreenWindowsOnly:YES
                                              completionHandler:^(SCShareableContent* content, NSError* e) {
        for (SCWindow* w in content.windows) {
            if (w.windowID == cgWindowID) { match = w; break; }
        }
        dispatch_semaphore_signal(sema);
    }];
    dispatch_semaphore_wait(sema, dispatch_time(DISPATCH_TIME_NOW, 5LL * NSEC_PER_SEC));
    if (!match) {
        fprintf(stderr, "WCStream: window id %u not found in shareable content\n", cgWindowID);
        return nullptr;
    }

    WCStream* s = new WCStream();
    s->width = width;
    s->height = height;
    s->latest.assign(width * height * 4, 0);
    s->output = [[WCStreamOutput alloc] init];
    s->output.owner = s;
    s->queue = dispatch_queue_create("baekar.window_capture", DISPATCH_QUEUE_SERIAL);

    SCContentFilter* filter = [[SCContentFilter alloc] initWithDesktopIndependentWindow:match];
    SCStreamConfiguration* cfg = [[SCStreamConfiguration alloc] init];
    cfg.width = width;
    cfg.height = height;
    cfg.pixelFormat = kCVPixelFormatType_32BGRA;
    cfg.minimumFrameInterval = CMTimeMake(1, 30);  // 30 fps cap
    cfg.queueDepth = 3;
    cfg.showsCursor = YES;

    NSError* err = nil;
    s->stream = [[SCStream alloc] initWithFilter:filter configuration:cfg delegate:s->output];
    BOOL added = [s->stream addStreamOutput:s->output type:SCStreamOutputTypeScreen sampleHandlerQueue:s->queue error:&err];
    if (!added) {
        fprintf(stderr, "WCStream: addStreamOutput failed: %s\n", err.localizedDescription.UTF8String);
        delete s;
        return nullptr;
    }
    [s->stream startCaptureWithCompletionHandler:^(NSError* e) {
        if (e) fprintf(stderr, "WCStream: start failed: %s\n", e.localizedDescription.UTF8String);
        else   fprintf(stderr, "WCStream: capture started for window %u (%dx%d)\n", cgWindowID, width, height);
    }];

    return s;
}

bool WCStream_LatestFrame(WCStream* s, uint8_t* bgra, int width, int height)
{
    if (!s || !bgra) return false;
    if (width != s->width || height != s->height) return false;
    if (!s->hasFrame.exchange(false)) return false;
    std::lock_guard<std::mutex> lk(s->mtx);
    if ((int)s->latest.size() != width * height * 4) return false;
    memcpy(bgra, s->latest.data(), width * height * 4);
    return true;
}

void WCStream_Close(WCStream* s)
{
    if (!s) return;
    if (s->stream) {
        dispatch_semaphore_t sema = dispatch_semaphore_create(0);
        [s->stream stopCaptureWithCompletionHandler:^(NSError*) {
            dispatch_semaphore_signal(sema);
        }];
        dispatch_semaphore_wait(sema, dispatch_time(DISPATCH_TIME_NOW, 2LL * NSEC_PER_SEC));
    }
    s->stream = nil;
    s->output = nil;
    delete s;
}
