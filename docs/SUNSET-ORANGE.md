# Sunset orange — version 0.1.1

On 19 November 2026 Richard requested a sunset-orange colour scheme across all five plugins. The shared editor now uses warm orange panels, burnt-orange gradient headers, cream displays and keys, and dark brown text and faders. The compact vintage layouts remain the basis of the design.

Validation: the native arm64 Release build succeeded and all 11 existing CTest targets passed. Five actual editor screenshots were regenerated in `docs/screenshots/`; Piano and JX10 were visually inspected at full size. The core text colours have calculated contrast ratios of 7.04:1 for labels and 4.80:1 for values against the orange panel.

All 164 audio-rendering cases produced exactly the same energy and peak measurements as the previous 0.1.0 builds. This is a regression check of those test cases, not a claim of sample-for-sample equivalence for every possible performance. No processing or parameter-state code was changed for this refresh.

All five installed 0.1.1 bundles passed strict macOS signature verification. The installer moved the previous installed copies to a dated folder under `~/Library/Audio/Plug-Ins/MDA Revival Backups/`. Existing Cubase processes need restarting to load the new binaries and palette. Plugin identifiers are retained, so 0.1.0 VST3 instances can load this update with their existing settings.

The CTest log is `docs/evidence/sunset-0.1.1-ctest.txt`. The initial port's pluginval and Cubase acceptance evidence remains in `docs/VERIFICATION.md`; those manual Cubase checks were performed before this colour refresh.
