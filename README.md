# MDA Revival for macOS

Five classic MDA plugins rebuilt as native Apple Silicon VST3s: **Piano, ePiano, DX10, JX10 and Leslie**. Version 0.1.0 is a local preview build with compact vintage interfaces, factory-program selection, automation, and an audition keyboard on the instruments.

## Use in Cubase

The build is installed for this Mac in `~/Library/Audio/Plug-Ins/VST3/`. Start Cubase in native Apple Silicon mode and search for **mda** in the instrument selector. Piano and ePiano are in **Sampler**, DX10 and JX10 in **Synth**; their vendor is **MDA Revival**. **mda Leslie** is an audio insert effect in **Modulation**. Each editor has a factory-program menu and previous/next buttons. Drag a fader to adjust it, double-click to reset it, and use the keyboard at the bottom to audition an instrument. **ALL OFF** clears voices and sustain.

To install from the archive, extract it and run **Install.command**, or copy the five bundles from its **VST3** folder into `~/Library/Audio/Plug-Ins/VST3/`. Restart Cubase to scan newly installed plugins. The installer preserves replaced copies in a dated backup folder.

If they do not appear immediately, Cubase 16's **Settings > Plug-ins** has a rescan button at the bottom of the plugin list. An explicit rescan was required and succeeded during this installation.

This package contains **arm64** binaries for Apple Silicon Macs. Build settings target macOS 11+, but the executed checks were on macOS 26.6.2. Intel Macs and Cubase running under Rosetta need an x86_64 or universal build. Source supports an optional universal build below; that architecture has not been validated in this preview.

These have new VST3 identities. They **do not automatically replace VST2 instances in old Cubase projects**, and do not yet import legacy FXP/FXB banks. The factory programs in the four JUCE instrument ports are retained, including the original ePiano placeholder slots. Leslie adds Slow Rotation, Fast Rotation and Stopped programs using the original rotary settings.

## Build

Requires CMake 3.22+, Ninja and Apple command-line build tools. No Projucer or paid JUCE licence is required for this AGPL source build.

```sh
python3 tools/bootstrap.py
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build -j 4
ctest --test-dir build --output-on-failure
python3 tools/package.py
```

For a universal build, use a separate build directory and `'-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64'`. Run the tests on each architecture before distributing it. The default build does not install anything automatically.

The source archive includes the exact JUCE checkout. `bootstrap.py` downloads the pinned JUCE commit only if it is missing and verifies an existing checkout instead of replacing it.

## Provenance and licence

The four instrument ports are based on [Matthijs Hollemans' JUCE conversions](https://github.com/hollance/mda-plugins-juce), commit `552702479a9eac534f3abbf6e878a5eab473e0cb`. Their original DSP and factory program tables are retained. Leslie derives from [Paul Kellett's rotary processor in Steinberg's MDA VST3 example](https://github.com/steinbergmedia/vst3_public_sdk/blob/master/samples/vst/mda-vst3/source/mdaLeslieProcessor.cpp), with safe delay allocation/indexing at high sample rates. This is an independent local revival, not an official MDA release.

JUCE is pinned to **9.0.5**, commit `d60b7a78030661574a615b586d9325ba7a7fa7dd`. The combined JUCE-linked builds and new integration/editor code are supplied under **AGPL-3.0-or-later**. The original MDA and Hollemans code retains its MIT notices. See [LICENSE.md](LICENSE.md) and `licenses/` for the applicable texts and JUCE's dependency inventory. The package includes complete corresponding source, including JUCE.

See [docs/VERIFICATION.md](docs/VERIFICATION.md) for the executed checks and limitations. These are ad hoc signed local development builds, not Developer ID signed or notarised distribution releases.
