#include "macos_camera_auth.h"

#import <AVFoundation/AVFoundation.h>
#import <dispatch/dispatch.h>

bool RequestCameraPermission(void)
{
    AVAuthorizationStatus status =
        [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];

    if (status == AVAuthorizationStatusAuthorized) {
        return true;
    }
    if (status == AVAuthorizationStatusDenied ||
        status == AVAuthorizationStatusRestricted) {
        return false;
    }

    __block bool granted = false;
    dispatch_semaphore_t sema = dispatch_semaphore_create(0);
    [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo
                             completionHandler:^(BOOL g) {
        granted = g;
        dispatch_semaphore_signal(sema);
    }];
    dispatch_semaphore_wait(sema,
        dispatch_time(DISPATCH_TIME_NOW, 60LL * NSEC_PER_SEC));
    return granted;
}

int PickCameraIndex(void)
{
    // OpenCV's AVFoundation backend enumerates devices using
    // AVCaptureDeviceDiscoverySession with the modern device-type list.
    // To match what cv::VideoCapture(N) expects, use the same set.
    NSArray<AVCaptureDeviceType>* types = @[
        AVCaptureDeviceTypeBuiltInWideAngleCamera,
        AVCaptureDeviceTypeExternalUnknown,
        AVCaptureDeviceTypeContinuityCamera,  // iPhone via Continuity
    ];
    AVCaptureDeviceDiscoverySession* ds =
        [AVCaptureDeviceDiscoverySession
            discoverySessionWithDeviceTypes:types
                                  mediaType:AVMediaTypeVideo
                                   position:AVCaptureDevicePositionUnspecified];
    NSArray<AVCaptureDevice*>* devices = ds.devices;

    if (devices.count == 0) {
        fprintf(stderr, "CameraPicker: no AVFoundation video devices found\n");
        return -1;
    }

    fprintf(stderr, "\n=== Pick a camera ===\n");
    for (NSUInteger i = 0; i < devices.count; ++i) {
        AVCaptureDevice* d = devices[i];
        const char* name = d.localizedName.UTF8String ?: "?";
        const char* type = d.deviceType.UTF8String ?: "?";
        fprintf(stderr, "  [%2lu] %s  (%s)\n",
                (unsigned long)i, name, type);
    }
    fprintf(stderr, "Enter index (0-%lu), or anything else for default (0): ",
            (unsigned long)(devices.count - 1));
    fflush(stderr);

    char line[64] = {0};
    if (!fgets(line, sizeof(line), stdin)) return -1;
    int idx = -1;
    if (sscanf(line, "%d", &idx) != 1 || idx < 0 || idx >= (int)devices.count) {
        fprintf(stderr, "CameraPicker: keeping default index 0\n");
        return -1;
    }
    fprintf(stderr, "CameraPicker: chose [%d] %s\n",
            idx, devices[idx].localizedName.UTF8String ?: "?");
    return idx;
}
