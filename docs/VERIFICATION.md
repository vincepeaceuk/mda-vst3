# Verification — MDA Revival 0.1.0

Executed on 19 November 2026, macOS 26.6.2, arm64. This is a local preview build.

## Result

| Check | Executed result |
| --- | --- |
| Release build | PASS: five native arm64 VST3 bundles, Apple Clang 16, CMake/Ninja, JUCE 9.0.5 |
| Project tests | PASS: 11/11 CTest targets |
| Audio rendering | PASS: 164 cases across all five plugins, including all 103 factory-program slots |
| Rates and blocks | 44,100 / 48,000 / 96,000 / 192,000 Hz; 32 / 256 / 1024 samples |
| pluginval 1.0.4 | PASS: strictness 5, all five plugins, GUI tests included, same rates/blocks |
| State recall | PASS: factory-program number and edited values survive restore in direct processor and VST3-host tests |
| Invalid state/program | PASS: malformed state is ignored; invalid program indices and names are handled safely |
| Editors | PASS: five real editors generated and visually inspected; 708×388 Piano/ePiano, 708×464 DX10, 768×540 JX10, 708×326 Leslie |
| Leslie high-rate stress | PASS: full depth/speed at all four rates, silence after reset, deterministic impulse after reset; native UBSan execution |
| Bundle signatures | PASS: strict codesign verification after manifest generation and after copying to the user plugin folder |
| Installer | PASS: two runs in an isolated installation folder; five installed bundles and five preserved backup bundles verified |
| Cubase 16.0.5 | PASS: all five discovered by the native arm64 scanner, none on its blocklist; detailed acceptance below |

Factory slots: Piano 8, ePiano 8, DX10 32, JX10 52, Leslie 3. The ePiano count includes the original duplicate placeholder programs.

## Cubase acceptance

Launched the installed Cubase 16.0.5.174 and created a separate empty test project in `build/cubase-qa/`. The initial selector did not list the new plugins. **Settings > Plug-ins > Rescan** detected them. The native `Cubase Pro VST3 Cache (arm64)/vst3plugins.xml` contains all five bundle names, and `vst3blocklist.xml` contains none of them.

Piano and ePiano were instantiated on instrument tracks; Leslie was inserted on the Piano channel. Their custom editors rendered correctly inside Cubase. Piano's program control wrapped from Broken Piano to mda Piano; Leslie's control selected Fast Rotation and its output was changed to −6.0 dB through the accessible slider. The Piano audition keyboard was exercised, and a nonzero Cubase channel peak was observed. ePiano's Autopan factory program was subsequently visible in Cubase.

DX10 and JX10 were confirmed in Cubase's instrument selector and scanner cache; their editors, sound and state were exercised by the automated VST3 host and pluginval, not separately instantiated during the manual Cubase checks. The new two-instrument test project was left open and was **not saved or reopened**. Cubase save/reopen, recorded automation, hardware MIDI and an extended music session remain manual follow-up checks. Automated host state round trips passed for all five.

## Reproduction

```sh
python3 tools/bootstrap.py
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build -j 4
ctest --test-dir build --output-on-failure -j 1
```

The final CTest run reported `100% tests passed out of 11`. Detailed CTest logs are in `build/Testing/Temporary/LastTest.log`; audio metrics and hosted editor images are in `build/evidence/`. Direct processor screenshots are in `docs/screenshots/`.

pluginval was downloaded from Tracktion's official v1.0.4 release. For each bundle:

```sh
deps/pluginval/pluginval.app/Contents/MacOS/pluginval \
  --strictness-level 5 --sample-rates 44100,48000,96000,192000 \
  --block-sizes 32,256,1024 --output-dir build/pluginval \
  --validate 'build/MDA_Piano_artefacts/Release/VST3/mda Piano.vst3'
```

All five validation logs end in `SUCCESS`. Copies of the final summary and logs are in `docs/evidence/` when packaged.

Leslie's standalone test was also compiled and executed with Apple's UndefinedBehaviorSanitizer:

```sh
clang++ -std=c++20 -fsanitize=undefined \
  -D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE \
  -g tests/LeslieDspTests.cpp -o build/leslie-dsp-ubsan
build/leslie-dsp-ubsan
```

AddressSanitizer could **not** run on this Mac's installed toolchains: Apple Clang 16 failed during sanitizer initialisation, and the installed LLVM 20 runtime deadlocked during its own initialisation before `main`. The LLVM process was stopped. These are not passing ASan results; the available UBSan run and stress tests are the executed safety evidence. A successful ASan run with a compatible current toolchain remains a follow-up.

## Test-first evidence

Journeys and acceptance came from Richard's requested five-plugin revival and vintage UI choice, recorded in `docs/PLAN.md`.

- **RED, missing Leslie implementation:** compiling `tests/LeslieDspTests.cpp` failed because the required `shared/LeslieDsp.h` did not exist. The implemented DSP now passes its native stress/reset tests and UBSan run. The earlier commit mentioning sanitizer tests was made before the ASan runtime failure was fully inspected; it must not be read as evidence of a successful ASan run.
- **RED, program recall:** the original Piano processor test printed `FAIL selected factory program is saved`, while edited parameter values passed. The original VST3 builds failed factory-program recall and the compact-panel acceptance check. An initial host test used a nonrepresentable 31.4% value on 1%-step parameters; it was corrected to 31% before accepting RED evidence, since legal parameter quantisation is not a defect.
- **GREEN:** `RevivalState.h` stores the selected factory program alongside the APVTS values and restores the index without reapplying its factory values. The five custom panels replace the generic editors, and all 11 final tests pass. Test files are `tests/ProcessorTests.cpp`, `tests/PluginTests.cpp` and `tests/LeslieDspTests.cpp`.

No line-coverage percentage is claimed. The tests exercise the shipping VST3 binaries, factory programs, host state, GUI creation and the new Leslie delay limits; they are not a complete proof of the inherited DSP's correctness.

## Scope and limits

- Four instrument audio algorithms and factory tables derive from the pinned Hollemans JUCE port. Exact equivalence to the supplied 2015 Intel VST2 binaries has **not** been established by an audio comparison.
- Leslie retains the original rotary calculations and adds a sample-rate-sized delay ring with full wraparound indexing. Its three speed programs are new convenience presets using the original default rotary settings.
- The new VST3 identities do not migrate old VST2 project instances or import FXP/FXB banks.
- The delivered architecture is arm64. Intel/Rosetta, other macOS versions and universal-binary operation were not tested.
- Bundles are ad hoc signed for local use. Developer ID signing, notarisation, an official Steinberg SDK-validator run and public distribution testing were not performed.
- JUCE 9 ignores the inherited splash-screen compile definition and prints a harmless build warning; no splash screen is enabled.
