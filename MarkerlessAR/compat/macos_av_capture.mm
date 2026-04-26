#include "macos_av_capture.h"

#import <AVFoundation/AVFoundation.h>
#import <CoreVideo/CoreVideo.h>

#include <atomic>
#include <mutex>
#include <vector>
#include <cstdio>
#include <cstring>

@interface AVCapDelegate : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
@property (nonatomic, assign) void* owner;
@end

struct AVCap {
    AVCaptureSession*           session = nil;
    AVCaptureDeviceInput*       input   = nil;
    AVCaptureVideoDataOutput*   output  = nil;
    AVCapDelegate*              delegate = nil;
    dispatch_queue_t            queue   = nullptr;

    std::mutex                  mtx;
    std::vector<uint8_t>        latest;          // packed BGR (3 bytes/px)
    int                         actualW = 0;
    int                         actualH = 0;
    std::atomic<bool>           hasFrame{false};
};

@implementation AVCapDelegate
- (void)captureOutput:(AVCaptureOutput*)output
        didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
        fromConnection:(AVCaptureConnection*)connection {
    if (!self.owner) return;
    AVCap* cap = (AVCap*)self.owner;
    CVImageBufferRef pb = CMSampleBufferGetImageBuffer(sampleBuffer);
    if (!pb) return;
    CVPixelBufferLockBaseAddress(pb, kCVPixelBufferLock_ReadOnly);
    int w = (int)CVPixelBufferGetWidth(pb);
    int h = (int)CVPixelBufferGetHeight(pb);
    int stride = (int)CVPixelBufferGetBytesPerRow(pb);
    const uint8_t* src = (const uint8_t*)CVPixelBufferGetBaseAddress(pb);

    // Convert BGRA → packed BGR (drop alpha) so cv::Mat construction is
    // straightforward downstream and existing CV pipeline expects 3-channel.
    std::vector<uint8_t> buf((size_t)w * h * 3);
    for (int y = 0; y < h; ++y) {
        const uint8_t* srow = src + y * stride;
        uint8_t* drow = buf.data() + (size_t)y * w * 3;
        for (int x = 0; x < w; ++x) {
            drow[x*3+0] = srow[x*4+0];  // B
            drow[x*3+1] = srow[x*4+1];  // G
            drow[x*3+2] = srow[x*4+2];  // R
        }
    }
    CVPixelBufferUnlockBaseAddress(pb, kCVPixelBufferLock_ReadOnly);

    {
        std::lock_guard<std::mutex> lk(cap->mtx);
        cap->latest.swap(buf);
        cap->actualW = w;
        cap->actualH = h;
    }
    cap->hasFrame.store(true);
}
@end

static AVCaptureDevice* DeviceAtIndex(int idx) {
    NSArray<AVCaptureDeviceType>* types = @[
        AVCaptureDeviceTypeBuiltInWideAngleCamera,
        AVCaptureDeviceTypeExternalUnknown,
        AVCaptureDeviceTypeContinuityCamera,
    ];
    AVCaptureDeviceDiscoverySession* ds =
        [AVCaptureDeviceDiscoverySession
            discoverySessionWithDeviceTypes:types
                                  mediaType:AVMediaTypeVideo
                                   position:AVCaptureDevicePositionUnspecified];
    NSArray<AVCaptureDevice*>* devices = ds.devices;
    if (devices.count == 0) return nil;
    if (idx < 0 || idx >= (int)devices.count) idx = 0;
    return devices[idx];
}

AVCap* AVCap_Open(int index, int /*width*/, int /*height*/)
{
    AVCaptureDevice* dev = DeviceAtIndex(index);
    if (!dev) {
        fprintf(stderr, "AVCap: no device at index %d\n", index);
        return nullptr;
    }
    fprintf(stderr, "AVCap: opening %s (uniqueID=%s)\n",
            dev.localizedName.UTF8String ?: "?",
            dev.uniqueID.UTF8String ?: "?");

    AVCap* cap = new AVCap();

    NSError* err = nil;
    cap->input = [AVCaptureDeviceInput deviceInputWithDevice:dev error:&err];
    if (!cap->input) {
        fprintf(stderr, "AVCap: deviceInput failed: %s\n", err.localizedDescription.UTF8String);
        delete cap;
        return nullptr;
    }

    cap->session = [[AVCaptureSession alloc] init];
    if ([cap->session canAddInput:cap->input]) [cap->session addInput:cap->input];

    cap->output = [[AVCaptureVideoDataOutput alloc] init];
    cap->output.videoSettings = @{
        (id)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_32BGRA)
    };
    cap->output.alwaysDiscardsLateVideoFrames = YES;

    cap->queue    = dispatch_queue_create("baekar.av_capture", DISPATCH_QUEUE_SERIAL);
    cap->delegate = [[AVCapDelegate alloc] init];
    cap->delegate.owner = cap;
    [cap->output setSampleBufferDelegate:cap->delegate queue:cap->queue];
    if ([cap->session canAddOutput:cap->output]) [cap->session addOutput:cap->output];

    // Pick a session preset that hints toward 640x480-class output for
    // throughput. Continuity Camera will still deliver native 1920x1080
    // and we resize downstream — the preset is advisory.
    if ([cap->session canSetSessionPreset:AVCaptureSessionPreset640x480]) {
        cap->session.sessionPreset = AVCaptureSessionPreset640x480;
    } else if ([cap->session canSetSessionPreset:AVCaptureSessionPresetMedium]) {
        cap->session.sessionPreset = AVCaptureSessionPresetMedium;
    }

    [cap->session startRunning];
    fprintf(stderr, "AVCap: session started\n");
    return cap;
}

bool AVCap_GetLatestBGR(AVCap* cap, uint8_t* bgr, int width, int height)
{
    if (!cap || !bgr) return false;
    if (!cap->hasFrame.exchange(false)) return false;
    std::lock_guard<std::mutex> lk(cap->mtx);
    if (cap->latest.empty()) return false;
    // If caller's buffer matches the actual size, copy directly. Otherwise
    // refuse and let them re-query the actual size first.
    if (width != cap->actualW || height != cap->actualH) return false;
    if ((int)cap->latest.size() != width * height * 3) return false;
    memcpy(bgr, cap->latest.data(), (size_t)width * height * 3);
    return true;
}

void AVCap_GetActualSize(AVCap* cap, int* outW, int* outH)
{
    if (!cap) { if (outW) *outW = 0; if (outH) *outH = 0; return; }
    std::lock_guard<std::mutex> lk(cap->mtx);
    if (outW) *outW = cap->actualW;
    if (outH) *outH = cap->actualH;
}

void AVCap_Close(AVCap* cap)
{
    if (!cap) return;
    if (cap->session) [cap->session stopRunning];
    cap->session  = nil;
    cap->input    = nil;
    cap->output   = nil;
    cap->delegate = nil;
    delete cap;
}
