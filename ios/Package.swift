// swift-tools-version:5.9
// BaekAR capture (roadmap F3). BaekARDataset is Foundation-only and builds
// on Linux; BaekARRecorder and BaekARCaptureUI need ARKit (iPhone/iPad).
import PackageDescription

let package = Package(
    name: "BaekARSwift",
    platforms: [.iOS(.v17), .macOS(.v14)],
    products: [
        .library(name: "BaekARDataset", targets: ["BaekARDataset"]),
        .library(name: "BaekARRecorder", targets: ["BaekARRecorder"]),
        .library(name: "BaekARCaptureUI", targets: ["BaekARCaptureUI"]),
    ],
    targets: [
        .target(name: "BaekARDataset"),
        .target(name: "BaekARRecorder", dependencies: ["BaekARDataset"]),
        .target(name: "BaekARCaptureUI", dependencies: ["BaekARRecorder"]),
        .testTarget(name: "BaekARDatasetTests", dependencies: ["BaekARDataset"]),
    ]
)
