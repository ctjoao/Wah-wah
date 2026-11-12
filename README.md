# Solo Wah – VST3 wah pedal with a mouse-controlled pedal

A wah-wah effect plug-in for REAPER (or any VST3 host) tuned for expressive lead-guitar solos
in the style of Kirk Hammett's wah playing: a high-Q, wide-sweep, slightly driven wah.

**The pedal on screen is the control.** Drag it up/down or use the mouse wheel to rock it.
Everything is also a normal automatable parameter, so you can record the sweep in REAPER.

## The interface

| Control | What it does |
|---|---|
| **Pedal** (the big treadle) | Drag down / scroll down = toe down = bright. Drag up / scroll up = heel down = dark. Shift-drag = fine. Double-click = centre. |
| **Voicing** | *Classic* (450 Hz – 1.9 kHz, Q 4), *Lead (KH-style)* (380 Hz – 2.3 kHz, Q 5.5, hotter peak – the default), *Vintage* (500 Hz – 1.5 kHz, Q 3, more low-end body) |
| **Resonance** | Q of the filter, 0.5x – 2x the voicing's value. More = more vocal "wacka". |
| **Range** | Width of the sweep, 50 % – 150 %. |
| **Drive** | Up to +18 dB of soft clipping *before* the filter (real wahs love being hit by a fuzz/overdrive; this simulates that interaction). |
| **Mix / Output** | Dry/wet blend (100 % = classic wah) and output trim ±12 dB. |

Frequencies above are the centre of the resonant peak at heel and toe. They are modelled on typical
wah behaviour, not measured from any specific pedal.

### Using it in REAPER
1. Put it **before** your amp sim/overdrive for the classic "wah into the amp" sound (or after for a "wah after distortion" sound – try both).
2. To record a sweep: in the plug-in window click REAPER's **Param** button → pick **Pedal** → **Show track envelope**, set the envelope to Write/Touch, and rock the on-screen pedal.
3. **Hardware expression pedal:** route MIDI CC 11 (expression) or CC 4 (foot controller) to the plug-in's MIDI input – it moves the on-screen pedal. You can also use **Param → Pedal → Learn** to bind any MIDI controller.

## Getting the .vst3 file

A VST3 is a compiled binary, so it has to be built for your operating system. Two routes:

### Route A – no tools needed (recommended): build on GitHub for free
1. Create a new GitHub repository and upload everything in this folder (keep the `.github` folder).
2. Open the **Actions** tab → *Build Solo Wah (VST3)* → wait ~10 minutes.
3. Download the artifact for your OS (Windows / macOS / Linux). It contains `Solo Wah.vst3`.

### Route B – build locally
Needs CMake 3.22+ and a C++17 compiler (Visual Studio 2022 on Windows, Xcode on macOS, GCC/Clang + `libasound2-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libfreetype6-dev libfontconfig1-dev libgl1-mesa-dev` on Linux).

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target SoloWah_VST3
```
JUCE 8.0.4 is downloaded automatically. The build copies the plug-in to your system VST3 folder.

### Install location
| OS | Folder |
|---|---|
| Windows | `C:\Program Files\Common Files\VST3\` |
| macOS | `/Library/Audio/Plug-Ins/VST3/` (or `~/Library/Audio/Plug-Ins/VST3/`) |
| Linux | `~/.vst3/` |

Then in REAPER: **Options → Preferences → Plug-ins → VST → Re-scan**, and add **Solo Wah** from the FX browser.
macOS: if the system blocks an unsigned download, run `xattr -dr com.apple.quarantine "Solo Wah.vst3"`.

## How it works
`Source/WahDSP.h` – a Zavalishin TPT state-variable filter. The pedal position moves the centre frequency
exponentially between the voicing's heel and toe frequencies (6 ms smoothing for zipper-free motion). Output =
unity-normalised band-pass × peak gain + a little low-pass for heel-position body, preceded by `tanh` drive.
`test/dsp_test.cpp` checks the sweep, the measured-vs-analytic response, stability and bypass:
`g++ -std=c++17 -O2 test/dsp_test.cpp -o dsp_test && ./dsp_test`.

`tools/snapshot.cpp` (CMake option `-DSOLOWAH_BUILD_TOOLS=ON`) renders the real editor to PNG and runs a
mouse/MIDI self-test – this is how the preview screenshots were made.
