# Legacy sources

`legacy/MarkerlessAR/` is the 2012 BaekAR engine as it was ported to macOS in 2026, kept with its original layout so its relative includes still work. It builds as the C++14 `baekar_legacy` library. The untouched 2012 tree is on the [`snapshot/2012-original`](https://github.com/ShawnBaek/baekar.github.com/tree/snapshot/2012-original) branch.

`legacy/bridge/` is new code: small C++14 wrappers that expose the 2012 classes through plain headers, so the C++17 adapters in `src/adapters/` never include a 2012 header.

| Bridge | Wraps | Adapter |
|---|---|---|
| `legacy_marker_tracker` | `ThreadBRISKMatching` + `ThreadTracking` (formerly `EngineMain.cpp`) | `LegacySingleMarkerTracker` |
| `legacy_pose` | `CCamera` pose and D3DX matrices | `LegacyCameraPoseEstimator` |
| `legacy_hand` | HandyAR `FingertipPoseEstimation` | `HandyArHandTracker` |
| `legacy_renderer` | `wonjo_dx` + the 2012 overlays and `init()` | `LegacyGlRenderer` |
| `legacy_scene` | `Contents` + window texture + picking | `ContentsScene` |

The VS2010 solution (`BaekAR.sln`, `.suo`, `.sdf`, `BaekAR_Sample/`) and build outputs under `MarkerlessAR/Engine/` are kept for history only.

## Provenance and licenses

Checked from the file headers before naming this folder (architecture rule 6). "Unknown" means the files carry no license statement; ask the authors before redistributing.

| Path | Origin | License |
|---|---|---|
| `MarkerlessAR/brisk/` | BRISK — Leutenegger, Chli, Siegwart, ETH Zurich (2011) | **GPL-3.0-or-later** |
| `MarkerlessAR/agast/` | AGAST — Elmar Mair (2010) | **GPL-3.0-or-later** |
| `MarkerlessAR/HandyAR/` | HandyAR — Taehee Lee (UCLA), adapted | Unknown |
| `MarkerlessAR/calibration/cvFindExtrinsicCameraParams3.*` | Taehee Lee | Unknown |
| `MarkerlessAR/calibration/BaekARCamera.*` | Wonwoo Lee (GIST), "non-commercial applications and research" | Non-commercial (header note) |
| `MarkerlessAR/KatoPoseEstimation/` | Kato & Billinghurst pose algorithm, BaekAR implementation | Unknown |
| `MarkerlessAR/3dobjects/bunny/` | Stanford bunny data | Unknown |
| Other `MarkerlessAR/*` | BaekAR (Sungwook Baek and team, 2012) | Project owner |

**GPL note.** The bundled BRISK and AGAST, and `SungwookFeature.cpp` (which carries BRISK's GPL header), are no longer built. The engine uses OpenCV's `cv::BRISK`, and the three helpers the tracker needed are rewritten in `legacy/bridge/marker_geometry.cpp`. The project license and third-party list are in `LICENSE` and `THIRD_PARTY.md`.
