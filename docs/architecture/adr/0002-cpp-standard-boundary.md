# 0002: Use C++17 for new architecture code

- Status: Accepted for the next foundation PR
- Date: 2026-07-26

## Context

BaekAR currently builds as C++14. New application and core code can use C++17, but the BRISK and AGAST sources still contain constructs that produce C++17 compatibility warnings. HandyAR and calibration also depend on the legacy OpenCV C API.

## Decision

- Keep the architecture-baseline PR at C++14.
- Set language requirements per CMake target instead of globally.
- Move new core and application targets to C++17 in the next PR.
- Keep an isolated legacy target at C++14 for as long as it is needed.
- Do not combine the language upgrade with an OpenCV major-version upgrade.
- Consider C++20 only after the legacy boundary builds and the behavior gates pass.

## Consequences

- New code can adopt C++17 without editing the 2012 algorithms first.
- Compiler warnings in legacy sources stay visible instead of being hidden by a rewrite.
- OpenCV modernization remains a separate, measurable migration.
