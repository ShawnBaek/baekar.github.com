#include <opencv2/opencv.hpp>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstdio>

#import <AVFoundation/AVFoundation.h>
#import <dispatch/dispatch.h>

static const int CAMERA_INDEX = 0;
static const int TIMEOUT_SECONDS = 10;
static const char* WINDOW_NAME = "OpenCV Camera Sample";

/// Request camera permission via AVFoundation and block until user responds.
/// Returns true if permission was granted.
static bool requestCameraPermission()
{
    AVAuthorizationStatus status = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];

    if (status == AVAuthorizationStatusAuthorized) {
        return true;
    }

    if (status == AVAuthorizationStatusDenied || status == AVAuthorizationStatusRestricted) {
        return false;
    }

    // Status is NotDetermined — request permission and wait for user response.
    __block bool granted = false;
    dispatch_semaphore_t sema = dispatch_semaphore_create(0);

    [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:^(BOOL g) {
        granted = g;
        dispatch_semaphore_signal(sema);
    }];

    // Wait up to 60 seconds for user to respond to the dialog.
    dispatch_semaphore_wait(sema, dispatch_time(DISPATCH_TIME_NOW, 60LL * NSEC_PER_SEC));
    return granted;
}

int main(int argc, char* argv[])
{
    // On macOS, camera access requires:
    //   1. An .app bundle with Info.plist containing NSCameraUsageDescription
    //   2. Explicit user permission via the macOS TCC dialog
    //   3. For macOS 15+: NSCameraUseContinuityCameraDeviceType in Info.plist
    //
    // We skip OpenCV's built-in AVFoundation auth handling (which has threading
    // issues — OpenCV #7519) and request permission directly via AVFoundation.
    setenv("OPENCV_AVFOUNDATION_SKIP_AUTH", "1", 1);

    fprintf(stderr, "Checking camera permission...\n");

    if (!requestCameraPermission()) {
        fprintf(stderr, "\nERROR: Camera permission denied.\n");
        fprintf(stderr, "Grant access in: System Settings > Privacy & Security > Camera\n");
        fprintf(stderr, "\nTo reset permission for this app:\n");
        fprintf(stderr, "  tccutil reset Camera com.baekar.opencv-camera-sample\n");
        return 1;
    }

    fprintf(stderr, "Camera permission granted. Opening camera %d (timeout: %ds)...\n",
            CAMERA_INDEX, TIMEOUT_SECONDS);

    // Open camera in a background thread with timeout.
    // Even with permission granted, VideoCapture::open() can block on the
    // main thread if the AVFoundation camera daemon is stuck. A background
    // thread with timeout prevents the app from freezing.
    // OPENCV_AVFOUNDATION_SKIP_AUTH=1 is set above so OpenCV won't try to
    // request auth again (we already handled it).
    cv::VideoCapture cap;
    std::atomic<bool> openDone{false};
    bool timedOut = false;

    std::thread([&]() {
        cap.open(CAMERA_INDEX);
        openDone = true;
    }).detach();

    auto start = std::chrono::steady_clock::now();
    while (!openDone) {
        auto elapsed = std::chrono::steady_clock::now() - start;
        if (std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() >= TIMEOUT_SECONDS) {
            timedOut = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (timedOut) {
        fprintf(stderr, "ERROR: Camera open timed out after %d seconds. "
                "The camera daemon may be stuck.\n", TIMEOUT_SECONDS);
    }

    if (!cap.isOpened()) {
        fprintf(stderr, "\nERROR: Could not open camera (index %d).\n", CAMERA_INDEX);
        fprintf(stderr, "Possible causes:\n");
        fprintf(stderr, "  - No camera hardware detected\n");
        fprintf(stderr, "  - Camera daemon stuck: try 'sudo killall -9 appleh13camerad cameracaptured VDCAssistant'\n");
        return 1;
    }

    int width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    fprintf(stderr, "Camera opened: %dx%d\n", width, height);

    cv::namedWindow(WINDOW_NAME, cv::WINDOW_AUTOSIZE);

    cv::Mat frame;
    int frameCount = 0;
    auto fpsStart = std::chrono::steady_clock::now();
    double fps = 0.0;

    while (true) {
        if (!cap.read(frame) || frame.empty()) {
            fprintf(stderr, "WARNING: Empty frame, retrying...\n");
            continue;
        }

        frameCount++;
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - fpsStart).count();
        if (elapsed >= 1.0) {
            fps = frameCount / elapsed;
            frameCount = 0;
            fpsStart = now;

            char title[128];
            snprintf(title, sizeof(title), "%s — %dx%d @ %.1f FPS",
                     WINDOW_NAME, width, height, fps);
            cv::setWindowTitle(WINDOW_NAME, title);
        }

        cv::imshow(WINDOW_NAME, frame);

        int key = cv::waitKey(1) & 0xFF;
        if (key == 27 || key == 'q' || key == 'Q') {
            break;
        }
    }

    cap.release();
    cv::destroyAllWindows();
    fprintf(stderr, "Clean exit.\n");
    return 0;
}
