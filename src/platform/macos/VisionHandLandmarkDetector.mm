#include "VisionHandLandmarkDetector.h"

#import <CoreVideo/CoreVideo.h>
#import <Foundation/Foundation.h>
#import <Vision/Vision.h>

#include <opencv2/imgproc.hpp>

#include <cstdio>

namespace baekar {

struct VisionHandLandmarkDetector::Impl {
    VNDetectHumanHandPoseRequest* request = nil;
    CVPixelBufferRef buffer = nullptr;  // reused while the frame size stays the same
    bool reportedError = false;

    ~Impl() {
        if (buffer) CVPixelBufferRelease(buffer);
    }

    // Copies a BGR image into the reused BGRA pixel buffer Vision reads.
    bool fill(const cv::Mat& bgr) {
        if (buffer && (static_cast<int>(CVPixelBufferGetWidth(buffer)) != bgr.cols ||
                       static_cast<int>(CVPixelBufferGetHeight(buffer)) != bgr.rows)) {
            CVPixelBufferRelease(buffer);
            buffer = nullptr;
        }
        if (!buffer) {
            NSDictionary* attributes = @{(id)kCVPixelBufferIOSurfacePropertiesKey : @{}};
            if (CVPixelBufferCreate(kCFAllocatorDefault, bgr.cols, bgr.rows, kCVPixelFormatType_32BGRA,
                                    (__bridge CFDictionaryRef)attributes, &buffer) != kCVReturnSuccess)
                return false;
        }
        CVPixelBufferLockBaseAddress(buffer, 0);
        cv::Mat bgra(bgr.rows, bgr.cols, CV_8UC4, CVPixelBufferGetBaseAddress(buffer),
                     CVPixelBufferGetBytesPerRow(buffer));
        cv::cvtColor(bgr, bgra, cv::COLOR_BGR2BGRA);
        CVPixelBufferUnlockBaseAddress(buffer, 0);
        return true;
    }
};

namespace {

// Vision joint names in HandJoint order.
NSArray<VNHumanHandPoseObservationJointName>* jointNames() {
    static NSArray<VNHumanHandPoseObservationJointName>* names = @[
        VNHumanHandPoseObservationJointNameWrist,
        VNHumanHandPoseObservationJointNameThumbCMC, VNHumanHandPoseObservationJointNameThumbMP,
        VNHumanHandPoseObservationJointNameThumbIP, VNHumanHandPoseObservationJointNameThumbTip,
        VNHumanHandPoseObservationJointNameIndexMCP, VNHumanHandPoseObservationJointNameIndexPIP,
        VNHumanHandPoseObservationJointNameIndexDIP, VNHumanHandPoseObservationJointNameIndexTip,
        VNHumanHandPoseObservationJointNameMiddleMCP, VNHumanHandPoseObservationJointNameMiddlePIP,
        VNHumanHandPoseObservationJointNameMiddleDIP, VNHumanHandPoseObservationJointNameMiddleTip,
        VNHumanHandPoseObservationJointNameRingMCP, VNHumanHandPoseObservationJointNameRingPIP,
        VNHumanHandPoseObservationJointNameRingDIP, VNHumanHandPoseObservationJointNameRingTip,
        VNHumanHandPoseObservationJointNameLittleMCP, VNHumanHandPoseObservationJointNameLittlePIP,
        VNHumanHandPoseObservationJointNameLittleDIP, VNHumanHandPoseObservationJointNameLittleTip,
    ];
    return names;
}

}  // namespace

VisionHandLandmarkDetector::VisionHandLandmarkDetector() : impl_(std::make_unique<Impl>()) {
    impl_->request = [[VNDetectHumanHandPoseRequest alloc] init];
    impl_->request.maximumHandCount = 1;
}

VisionHandLandmarkDetector::~VisionHandLandmarkDetector() = default;

HandLandmarks VisionHandLandmarkDetector::detect(const cv::Mat& bgr) {
    HandLandmarks hand;
    if (bgr.empty() || bgr.type() != CV_8UC3) return hand;
    @autoreleasepool {
        if (!impl_->fill(bgr)) return hand;
        VNImageRequestHandler* handler = [[VNImageRequestHandler alloc] initWithCVPixelBuffer:impl_->buffer
                                                                                      options:@{}];
        NSError* error = nil;
        if (![handler performRequests:@[ impl_->request ] error:&error]) {
            if (!impl_->reportedError) {
                std::fprintf(stderr, "BaekAR: Vision hand pose failed: %s\n",
                             error ? error.localizedDescription.UTF8String : "unknown error");
                impl_->reportedError = true;
            }
            return hand;
        }
        VNHumanHandPoseObservation* observation = impl_->request.results.firstObject;
        if (!observation) return hand;
        NSDictionary<VNHumanHandPoseObservationJointName, VNRecognizedPoint*>* points =
            [observation recognizedPointsForJointsGroupName:VNHumanHandPoseObservationJointsGroupNameAll
                                                      error:&error];
        if (!points) return hand;
        NSArray<VNHumanHandPoseObservationJointName>* names = jointNames();
        for (int i = 0; i < kHandJointCount; ++i) {
            VNRecognizedPoint* point = points[names[static_cast<NSUInteger>(i)]];
            if (!point) continue;
            // Vision points are normalized with the origin at the bottom left.
            hand.points[i] = cv::Point2f(static_cast<float>(point.location.x * bgr.cols),
                                         static_cast<float>((1.0 - point.location.y) * bgr.rows));
            hand.jointConfidence[i] = point.confidence;
        }
        hand.confidence = observation.confidence;
        hand.found = true;
    }
    return hand;
}

std::string VisionHandLandmarkDetector::describe() const { return "Apple Vision hand pose"; }

}  // namespace baekar
