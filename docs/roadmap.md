# BaekAR roadmap

The foundation refactoring (stages 1–7) is merged. This roadmap covers what comes next: three groundwork PRs, a hand-tracking PR, and one large PR per year for the markerless AR catch-up (2013–2026).

## License policy

**Accepted (2026-09-30): Apache-2.0 for the project, permissive dependencies only.** See `LICENSE`, `NOTICE` and `THIRD_PARTY.md`.

The owner wants BaekAR to stay open source, and may also use it in their own app later. A GPL-3.0 project would require that app's source to be released under the GPL too. Non-commercial research licenses would rule out app use entirely. Apache-2.0 keeps the project open source and allows use in closed or commercial apps. It also includes a patent grant.

What this means for the work:

- Stop linking the bundled GPL-3.0 BRISK/AGAST. The engine already uses OpenCV's `cv::BRISK`.
- GPL-3.0 SLAM systems (LSD-SLAM, ORB-SLAM2/3, DSO, VINS-Mono, OpenVINS) are not wrapped. The yearly PR implements the method from its paper and may use permissive libraries (Ceres, Basalt, Open3D) as references.
- Non-commercial models (SuperPoint/SuperGlue weights, the original Inria 3DGS, DUSt3R/MASt3R, NVIDIA instant-ngp) are not shipped. Each yearly PR picks a permissive alternative, or writes its own implementation.
- 2012 code with unknown or non-commercial terms (HandyAR, `BaekARCamera`) stays in `legacy/` as a comparison baseline. It does not go into an app.
- Every PR that adds a dependency or model records its license in `legacy/README.md` or a new `THIRD_PARTY.md`. Licenses are checked again when the PR is written; the names below are candidates, not approvals.

## Order

```text
F1 cleanup ─┬─ F2 evaluation ─┬─ Y2013 ─ Y2014 ─ Y2015 ─ Y2016 ─ Y2017 ─ Y2018 ─ Y2019
            └─ F3 capture ────┴─ H hand ───────────────────────────────────────────┘
                                                                                    │
            Y2020 ─ Y2021 ─ Y2022 ─ R Metal ─ Y2023 ─ Y2024 ─ Y2025 ─ Y2026 ─────────┘
```

Step 2 (macOS verification on a real Mac) runs alongside F1. It needs the owner's Mac.

## Groundwork

| PR | Scope | Verified by |
|---|---|---|
| **F1 cleanup** (done) | `LICENSE` (Apache-2.0), `NOTICE`, `THIRD_PARTY.md`. The bundled BRISK/AGAST and `SungwookFeature.cpp` leave the build. `CCamera` stops appending to `viewlog.txt`. OpenCV 5: [ADR 0003](architecture/adr/0003-opencv-5.md). The multi-marker test waits per frame instead of sleeping, so it passes on slow machines. | Linux + macOS CI |
| **F2 evaluation** (done; public-dataset runs wait for network access) | A sensor bundle on the frame-source port: RGB, depth, IMU, intrinsics and a reference pose. TUM RGB-D, EuRoC and BaekAR dataset readers ([format](dataset-format.md)); `--record` writes the BaekAR format. `baekar_eval`: marker benchmark (corner error, latency, lag, camera path), ATE/RPE with Sim(3)/SE(3) alignment, dataset summary, Markdown + PNG reports. | Generated fixtures and the synthetic camera on Linux CI. TUM/EuRoC hosts are blocked by this environment's network policy. |
| **F3 capture + Core ML** (done; first device recording needs the owner's iPhone) | iPhone capture app (`ios/`, Swift, ARKit). It records RGB (JPEG), LiDAR depth, 200 Hz IMU, intrinsics and the ARKit pose into a BaekAR dataset folder that `--replay` plays back. `IInferenceEngine`: Core ML on macOS/iOS, ONNX through OpenCV DNN everywhere else (no new dependency; ONNX Runtime can be added behind the same port if a model needs operators OpenCV lacks). | Swift dataset writer tested on Linux and macOS CI and read back by the C++ engine; unsigned iOS device build in CI; Core ML and ONNX give the same results on macOS CI |

## Hand tracking (pulled forward)

HandyAR's skin-color method finds no hand pose even on a clear, spread-hand photo (0 of 300 frames). Skin-color segmentation is an abandoned approach, while learned hand tracking keeps improving:

- MediaPipe Hands and the Apple Vision hand-pose API (2019–2020)
- hands as the primary input on Meta Quest and Apple Vision Pro
- single-image 3D hand mesh recovery (2024 onward)

| PR | Scope | Verified by |
|---|---|---|
| **H hand** | New `IHandTracker` implementations: Apple Vision hand pose (macOS/iOS, on-device), and MediaPipe Hands (Apache-2.0) through the F3 inference port on Linux. `InteractionController` keeps working unchanged. HandyAR stays as the comparison baseline. | Recorded hand sessions (F3), fingertip detection rate, pick-and-drag on the window plane |

## Yearly PRs

Each yearly PR contains:

- research notes
- one implementation behind a port, selected in the composition root
- an F2 report comparing it with the previous year on the same data
- macOS verification

| PR | Focus | New port or change | Candidate implementation (license checked at PR time) | Data |
|---|---|---|---|---|
| Y2013 | Semi-dense visual odometry | `IOdometry`: camera trajectory without a marker | Own implementation from Engel et al. 2013 | TUM mono, iPhone RGB |
| Y2014 | Direct SLAM, keyframe maps | `IMapper`: keyframes and map | Own, following LSD-SLAM | TUM, iPhone |
| Y2015 | Feature SLAM, relocalization | Loop closure, relocalization | Own, following ORB-SLAM; Ceres for bundle adjustment | TUM, iPhone |
| Y2016 | Stereo and RGB-D SLAM | Depth input path | Own; Open3D as a reference | TUM RGB-D, iPhone LiDAR, Main + Ultra Wide pair |
| Y2017 | Visual-inertial tracking, world anchors | `IImuSource`, persistent anchors | Own pre-integration; Basalt as a reference | EuRoC, iPhone IMU, ARKit pose as reference |
| Y2018 | Learned local features | `IFeatureExtractor` on Core ML | XFeat or another permissive detector | TUM, iPhone |
| Y2019 | Scene understanding, occlusion | Depth-based occlusion in the renderer | LiDAR depth, Apple Vision person segmentation | iPhone |
| Y2020 | Learned matching | `IFeatureMatcher` | LightGlue with permissive features | TUM, iPhone |
| Y2021 | Detector-free matching, learned SLAM | Dense matching, learned SLAM back end | LoFTR, DROID-SLAM | TUM, EuRoC |
| Y2022 | Neural implicit maps | `IReconstruction` | Permissive implicit-SLAM reference | iPhone RGB-D |
| R Metal | Metal renderer | Second `IRenderer` | Own | macOS |
| Y2023 | 3D Gaussian Splatting | Splat rendering on Metal | gsplat (Apache-2.0) as a reference | ARKit pose + RGB, LiDAR init |
| Y2024 | Learned dense reconstruction; 3D hand mesh | Multi-view reconstruction; hand mesh behind `IHandTracker` | Permissive DUSt3R-class alternative (DUSt3R/MASt3R are non-commercial) | iPhone three cameras, hand recordings |
| Y2025 | 3D visual foundation models | Feed-forward reconstruction | VGGT-class model if its license allows | iPhone |
| Y2026 | Streaming spatial reconstruction | Real-time streaming map | Streaming reconstruction model if its license allows | Full iPhone sensor stream |

Y2013–Y2015 are the largest. They move the engine from marker tracking to map-building SLAM, and they fix the shape of `IOdometry` and `IMapper` for every later year.

## Working rules

- Branches: `foundation/f1-cleanup`, `hand/learned-tracking`, `year/2013-semidense-vo`, and so on.
- A PR is opened only when the owner asks.
- Linux CI and public datasets verify what can run headless. iPhone recordings and real-Mac checks need the owner.
- Architecture rules in `docs/architecture/README.md` still apply: new work enters through a port, and the 2012 pipeline stays available for comparison.
