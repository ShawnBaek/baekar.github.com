# 0001: Refactor around the working engine

- Status: Accepted
- Date: 2026-07-26

## Context

The macOS engine works, but application lifecycle, capture, tracking, pose, rendering, interaction, and platform code meet in `EngineMain.cpp`. The 2012 code also uses shared global state and manual resource ownership. Rewriting all of it at once would make behavior changes difficult to identify.

## Decision

Use an incremental ports-and-adapters migration.

- Add characterization coverage before extracting a boundary.
- Wrap an existing implementation before changing its algorithm.
- Keep the application executable as the composition root.
- Let adapters depend on application ports; do not let the core depend on adapters.
- Move physical files only after their build target and dependency direction are clear.

The first extracted build boundary is marker tracking. The application and tests link the same implementation instead of compiling separate copies.

## Consequences

- Some old and new structure will coexist during the foundation PRs.
- Interfaces will be introduced only where there is a real implementation choice or platform boundary.
- Small PRs can be compared with the merged macOS behavior.
- Folder cleanup comes later than ownership and dependency cleanup.
