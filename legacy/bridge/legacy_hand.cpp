// Moved from EngineMain.cpp (InitializeEngineMain and mainLoop).

#include "legacy_hand.h"

#include "HandyAR/FingertipPoseEstimation.h"

#include <cstdio>
#include <iostream>
#include <vector>

namespace legacy_hand {
namespace {

Matrix toMatrix(const D3DXMATRIXA16& in) {
    Matrix out;
    for (int i = 0; i < 16; ++i) out[i] = in[i];
    return out;
}

// HandyAR takes char*; keep writable copies of the paths.
std::vector<char> writable(const std::string& text) {
    std::vector<char> buffer(text.begin(), text.end());
    buffer.push_back('\0');
    return buffer;
}

}  // namespace

struct HandTracker::Impl {
    FingertipPoseEstimation estimation;
    bool processing = false;
};

HandTracker::HandTracker() : impl_(new Impl) {}

HandTracker::~HandTracker() = default;

bool HandTracker::Initialize(const cv::Mat& firstFrame, const std::string& calibrationPath,
                             const std::string& fingertipPath) {
    // load fingertip coordinates (if the file does not exist this just fails
    // and the user may measure fingertip coordinates and save them)
    std::vector<char> fingertip = writable(fingertipPath);
    if (impl_->estimation.LoadFingertipCoordinates(fingertip.data()) == false)
        printf("fingertip coordinate '%s' file was not loaded.\n", fingertip.data());
    else
        printf("fingertip coordinate '%s' file was loaded.\n", fingertip.data());

    fprintf(stderr, "DBG: About to init FingertipPoseEstimation...\n");
    IplImage header = cvIplImage(firstFrame);
    std::vector<char> calibration = writable(calibrationPath);
    const bool ok = impl_->estimation.Initialize(&header, calibration.data());
    if (!ok) fprintf(stderr, "Warning: FingertipPoseEstimation init failed (continuing anyway)\n");
    fprintf(stderr, "DBG: FingertipPoseEstimation init done\n");
    return true;
}

void HandTracker::Capture(const cv::Mat& bgr, std::int64_t tickCount) {
    IplImage header = cvIplImage(bgr);
    impl_->estimation.OnCapture(&header, tickCount);
}

State HandTracker::Process(const cv::Mat& bgr, std::int64_t tickCount) {
    Capture(bgr, tickCount);
    //핑거에 대한 프로세싱을 시작하는 부분
    impl_->estimation.OnProcess();
    impl_->estimation.TickCountBegin();  // (7) Rendering
    impl_->processing = true;

    State state;
    D3DXMATRIXA16 matProj, matView;
    impl_->estimation.D3DXMakeProjectionMatrix(&matProj);
    impl_->estimation.D3DXMakeViewMatrix(&matView);
    state.projection = toMatrix(matProj);
    state.view = toMatrix(matView);
    state.validPose = impl_->estimation.QueryValidPose();
    if (state.validPose) {
        std::cout << " Valid Fingertip!! " << std::endl;
        // Index finger, smoothed, in 320x240 HandyAR space -> 640x480 window.
        const CvPoint2D32f tip = impl_->estimation.QueryFingertip2D(1);
        state.indexFingertip = cv::Point2f(tip.x * 2.0f, tip.y * 2.0f);
    }
    return state;
}

void HandTracker::FinishFrame() {
    if (!impl_->processing) return;
    impl_->estimation.TickCountEnd();  // (7) Rendering
    impl_->estimation.TickCountNewLine();
    impl_->processing = false;
}

cv::Mat HandTracker::DebugView() {
    IplImage* image = impl_->estimation.OnDisplay();
    return image ? cv::cvarrToMat(image) : cv::Mat();
}

}  // namespace legacy_hand
