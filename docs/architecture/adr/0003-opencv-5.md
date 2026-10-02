# 0003: OpenCV 5 and the 2012 C API

- Status: Accepted
- Date: 2026-09-30

## Context

Homebrew's `opencv` formula moved to OpenCV 5.0, which removed the C API (`IplImage`, `CvMat`, `cv*` functions and the `*_c.h` headers). The 2012 engine in `legacy/MarkerlessAR` uses that API throughout: HandyAR, calibration, Kato, the D3D-over-GL renderer and the compat shims. The code written since the foundation work (`src/`, `apps/`, `legacy/bridge/marker_geometry.cpp`) uses only the C++ API.

## Decision

- Keep building against OpenCV 4 (`opencv@4` on macOS, the distribution package on Linux).
- Do not port the 2012 C-API code to OpenCV 5. The roadmap replaces it piece by piece: HandyAR in PR H, the renderer in R Metal, and the marker pose and trackers in the yearly PRs.
- New code must not use the C API, so that it builds on OpenCV 5 unchanged.
- Once no product path needs `baekar_legacy`, it becomes an optional target (`BAEKAR_WITH_LEGACY`) that requires OpenCV 4, and the rest of the build moves to OpenCV 5.

## Consequences

- macOS users need `brew install opencv@4`; CMake finds the keg automatically.
- Porting the legacy C-API code costs no time now; it is spent instead on its replacements.
- CI keeps building against OpenCV 4 until `baekar_legacy` is optional.
