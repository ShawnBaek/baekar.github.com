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
