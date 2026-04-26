#include "macos_camera_menu.h"

#import <Cocoa/Cocoa.h>
#import <AVFoundation/AVFoundation.h>

#include <cstdio>

@interface BAKCameraMenuTarget : NSObject {
@public
    CameraSwitchCallback _cb;
    NSMenu*              _menu;
}
- (void)cameraPicked:(id)sender;
@end

@implementation BAKCameraMenuTarget
- (void)cameraPicked:(NSMenuItem*)sender {
    int idx = (int)sender.tag;
    fprintf(stderr, "CameraMenu: switching to index %d (%s)\n",
            idx, sender.title.UTF8String ?: "?");
    if (_cb) _cb(idx);
    // Update check marks
    for (NSMenuItem* it in _menu.itemArray) {
        it.state = (it.tag == sender.tag) ? NSControlStateValueOn
                                          : NSControlStateValueOff;
    }
}
@end

// Keep target alive for the app lifetime.
static BAKCameraMenuTarget* g_target = nil;

void InstallCameraMenu(int currentIndex, CameraSwitchCallback cb)
{
    NSApplication* app = [NSApplication sharedApplication];
    if (!app) {
        fprintf(stderr, "CameraMenu: no NSApplication; can't install menu\n");
        return;
    }
    NSMenu* mainMenu = app.mainMenu;
    if (!mainMenu) {
        // GLFW usually sets up a minimal main menu. If not, build one.
        mainMenu = [[NSMenu alloc] init];
        app.mainMenu = mainMenu;
        NSMenuItem* appItem = [[NSMenuItem alloc] init];
        appItem.submenu = [[NSMenu alloc] initWithTitle:@"BaekAR"];
        [mainMenu addItem:appItem];
    }

    // Camera submenu
    NSMenu* cameraMenu = [[NSMenu alloc] initWithTitle:@"Camera"];
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

    g_target      = [[BAKCameraMenuTarget alloc] init];
    g_target->_cb = cb;
    g_target->_menu = cameraMenu;

    NSUInteger i = 0;
    for (AVCaptureDevice* d in ds.devices) {
        NSMenuItem* item = [[NSMenuItem alloc]
            initWithTitle:d.localizedName
                   action:@selector(cameraPicked:)
            keyEquivalent:@""];
        item.target = g_target;
        item.tag    = (NSInteger)i;
        if ((int)i == currentIndex) item.state = NSControlStateValueOn;
        [cameraMenu addItem:item];
        ++i;
    }

    NSMenuItem* cameraTop = [[NSMenuItem alloc]
        initWithTitle:@"Camera" action:nil keyEquivalent:@""];
    cameraTop.submenu = cameraMenu;
    [mainMenu addItem:cameraTop];

    fprintf(stderr, "CameraMenu: installed (%lu devices)\n",
            (unsigned long)ds.devices.count);
}
