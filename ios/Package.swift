// swift-tools-version:5.9
// BaekAR capture (roadmap F3). BaekARDataset is Foundation-only and builds
// on Linux; BaekARCapture and BaekARCaptureUI need ARKit (iPhone/iPad).
import PackageDescription

let package = Package(
    name: "BaekARCapture",
    platforms: [.iOS(.v17), .macOS(.v14)],
    products: [
        .library(name: "BaekARDataset", targets: ["BaekARDataset"]),
        .library(name: "BaekARCapture", targets: ["BaekARCapture"]),
        .library(name: "BaekARCaptureUI", targets: ["BaekARCaptureUI"]),
    ],
    targets: [
        .target(name: "BaekARDataset"),
        .target(name: "BaekARCapture", dependencies: ["BaekARDataset"]),
        .target(name: "BaekARCaptureUI", dependencies: ["BaekARCapture"]),
        .testTarget(name: "BaekARDatasetTests", dependencies: ["BaekARDataset"]),
    ]
)
