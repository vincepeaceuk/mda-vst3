# MDA Revival, 19 November 2026

Richard requested macOS VST3 versions of Piano, ePiano, DX10, JX10 and Leslie for Cubase, with updated interfaces. He chose compact, deliberately vintage MDA styling.

Acceptance checks:

- Five separate native Apple Silicon VST3 bundles, four MIDI instruments and one stereo rotary effect.
- Preserve the supplied JUCE ports' DSP and factory programs; document any safety fixes and compatibility limits.
- Compact custom editors, all parameters exposed to host automation, visible factory preset selection.
- Edited parameters and selected programs survive project save/restore.
- Render finite audio at 44.1, 48, 96 and 192 kHz with small and large blocks; exercise every factory program.
- Leslie maximum delay depth is safe at high sample rates, with AddressSanitizer and UndefinedBehaviorSanitizer checks.
- Run pluginval and inspect the actual editors; verify Cubase discovery and use when possible on this Mac.
- Package the binaries with install instructions, complete project source, dependency versions and licence notices.

The original downloaded VST2 bundles stay available as reference. VST3 instruments use new IDs: old Cubase sessions do not automatically migrate. Legacy FXP/FXB import and exact audio equivalence to the original Intel VST2 binaries require separate work.

Sources: hollance/mda-plugins-juce at 552702479a9eac534f3abbf6e878a5eab473e0cb; JUCE 9.0.5 at d60b7a78030661574a615b586d9325ba7a7fa7dd; original MIT Leslie processor from Steinberg's mda-vst3 example.
