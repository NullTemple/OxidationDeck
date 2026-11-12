# The Oxidation Deck // Master Bus Edition (MZOD-1)
### Mauve Zone Boutique DSP Apparatus

The **Oxidation Deck (MZOD-1)** is an analog-modeled, four-stage mastering and bus saturation console built with JUCE and C++20. Designed in the signature **Mauve Zone** aesthetic, MZOD-1 combines precision analog emulation, tube asymmetry, dynamic tape magnetization, dual-topology compression, and an ITU-R BS.1770-4 compliant True-Peak brickwall limiter.

---

## 🎛️ Four-Deck Console Architecture

### Deck 1: Preamp & Line Stage (Nonlinear Coloration & Bloom)
* **11 Analog Circuit Topologies across 4 Families:**
  * **Tubes (Glass):** Triode Class-A (12AX7), Pentode Push-Pull (EL34), Starved Triode.
  * **Transformers (Core):** Iron Core (Neve), Nickel Core (API), Steel Inductor (Pultec).
  * **Solid-State:** Class-A Silicon, Germanium (TG style), FET Driver (1176 sheen).
  * **Mauve Zone Exotics:** Zener Soft-Clipper, Oxide Rot & Flux Breakdown (chaotic wavefolding decay).
* **Controls:**
  * `DRIVE`: $0.0 \text{ dB}$ to $+24.0 \text{ dB}$ with input level-dependent harmonic generation.
  * `BIAS`: $0\%$ to $100\%$ even-harmonic symmetry skewing with zero-input quiescent DC offset cancellation (`evaluateBiasNull`).
  * `PRE-TILT`: $\pm 6.0 \text{ dB}$ spectral weighting filter before the nonlinear transfer curve.
  * `PARALLEL`: $0\%$ to $100\%$ linear wet/dry blend.
  * `AUTO-GAIN`: Intelligent loudness compensation algorithm.
* **CRT Analyzer:** Audio-reactive 5-bar harmonic spectrum display (H1–H5) and live transfer-curve oscilloscope.

### Deck 2: Master Bus Dynamics
* **Dual Topology:**
  * **VCA Mode:** Punchy, clean, tight program glue with feed-forward VCA detection.
  * **Vari-Mu Mode:** Tube-like soft-knee feedback compression with program-dependent attack and recovery.
* **Stereo Linking:** $80\%/20\%$ master bus linked detection prevents center-image drift while preserving wide stereo transients.
* **Controls:** `THRESHOLD` ($-30 \text{ to } +10 \text{ dB}$), `RATIO` (1.5:1, 2:1, 4:1, 10:1), `ATTACK` (0.1 to 30 ms), `RELEASE` (100 ms to Auto), `SC-HPF` (30, 60, 85, 120, 160, 220 Hz), and `LISTEN` sidechain monitor.
* **VU Metering:** Dual high-ballistics stereo GR needle meters with peak-hold transient catch.

### Deck 3: Magnetic Tape Transport & Saturation
* **Speeds:** 15 IPS (rich low-end head bump at $38 \text{ Hz}$) and 30 IPS (extended treble air with head bump at $55 \text{ Hz}$).
* **Controls:**
  * `SATURATION`: Non-linear magnetic core compression.
  * `BIAS CAL`: High-frequency saturation and headroom contouring ($\pm 3.0 \text{ dB}$).
  * `WOW/FLUTTER`: Dual-speed capstan mechanical drift and stochastic tape transport flutter.
  * `OXIDE WEAR`: High-frequency tape loss and physical playback head degradation.
* **Visuals:** Animated spinning tape reels and audio-reactive flux core glow.

### Deck 4: True-Peak Brickwall Limiter & Loudness Telemetry
* **ITU-R BS.1770-4 True-Peak Limiter:**
  * Fractional lookahead delay line with distance-weighted lookahead attack slopes.
  * Sub-sample parabolic inter-sample peak detection.
  * Zero-overshoot guaranteed ceiling clamping.
  * 4 Release Modes: Fast (25 ms), Medium (80 ms), Slow (250 ms), Adaptive dual-time constant.
* **Loudness Radar & Telemetry:** Real-time Integrated LUFS, Short-Term LUFS, True-Peak dBTP, Loudness Range (LRA), and ISP clip tracking with clickable reset.
* **Speaker Fuse:** Safety hard-limit guard at $+6.0 \text{ dBFS}$ prevents speaker blowout under extreme drive conditions.

---

## ⚡ Global Engine & Master Bus Routing

* **6-Way Signal Chain Reordering:**
  Dynamically swap the processing order of Decks 1–3 on the fly with seamless micro-crossfades:
  1. `PRE ▸ COMP ▸ TAPE`
  2. `PRE ▸ TAPE ▸ COMP`
  3. `COMP ▸ PRE ▸ TAPE`
  4. `COMP ▸ TAPE ▸ PRE`
  5. `TAPE ▸ PRE ▸ COMP`
  6. `TAPE ▸ COMP ▸ PRE`
  *(Deck slots physically rearrange on the UI, updating their `01 ·`, `02 ·`, `03 ·` slot badges)*.
* **Multirate Oversampling:** Polyphase half-band IIR engine selectable between $2\times$, $4\times$, and $8\times$ with full host delay compensation (PDC).
* **M/S Matrix:** Instant Mid/Side processing toggle with accurate stereo decode.
* **Vector-Scaled GUI:** Fixed aspect-ratio UI that scales smoothly across all display resolutions (compact laptops to 4K monitors) with fine velocity mode (`Ctrl`/`Cmd`) and double-click reset on all knobs.

---

## 🛠️ Building

### Requirements
* **JUCE 7/8**
* **CMake 3.22+**
* C++20 compliant compiler (MSVC 2022 / Clang / GCC)

### Build Commands (Windows / Visual Studio)
```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The compiled binaries will be located at:
* **VST3:** `build/MZOD1_artefacts/Release/VST3/The Oxidation Deck.vst3`
* **Standalone:** `build/MZOD1_artefacts/Release/Standalone/The Oxidation Deck.exe`

---

## 📜 Credits & License
Created by **Mauve Zone** (Alex / NullTemple). All rights reserved.
