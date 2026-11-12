# THE OXIDATION DECK // MASTER BUS EDITION (MZOD-1)
## High-Level Architecture & Technical Specification

---

## 1. Executive Manifesto & Product Vision

**The Oxidation Deck // Master Bus Edition (MZOD-1)** is a boutique, mastering-grade end-of-chain processor designed to bridge uncompromising digital precision with the organic warmth, mechanical friction, and harmonic depth of analog hardware. 

Engineered specifically for the **Master Bus, Drum Bus, and Submix Stems**, the MZOD-1 integrates four quintessential analog mastering stages into a single cohesive, gain-calibrated ecosystem:

```
[ INPUT (STEREO / M/S) ]
          │
          ▼
┌─────────────────────────────────────────────────────────────────────────┐
│ STAGE 1: HARMONIC LINE STAGE (11 Pro-Grade Console Flavors)             │
│ Tubes (Triode, Pentode, Starved) · Iron/Nickel Transformers · Germanium │
└────────────────────────────────────┬────────────────────────────────────┘
                                     │
                                     ▼
┌─────────────────────────────────────────────────────────────────────────┐
│ STAGE 2: MASTER BUS DYNAMICS (VCA & Vari-Mu Cohesion)                   │
│ SSL-Style VCA Punch · Fairchild-Style Tube Bloom · Sidechain HPF        │
└────────────────────────────────────┬────────────────────────────────────┘
                                     │
                                     ▼
┌─────────────────────────────────────────────────────────────────────────┐
│ STAGE 3: THE OXIDATION TAPE MACHINE (Unified Magnetic & Mechanical)     │
│ Jiles-Atherton Hysteresis · 15/30 IPS · Capstan Wow/Flutter · Oxide Rot │
└────────────────────────────────────┬────────────────────────────────────┘
                                     │
                                     ▼
┌─────────────────────────────────────────────────────────────────────────┐
│ STAGE 4: TRUE-PEAK BRICKWALL LIMITER & LOUDNESS TELEMETRY               │
│ ITU-R BS.1770-4 Inter-Sample Lookahead · EBU R128 LUFS · Amber Scope    │
└────────────────────────────────────┬────────────────────────────────────┘
                                     │
                                     ▼
[ LOSSLESS MASTER OUTPUT ]
```

### Design Philosophy
1. **True Physical Modeling, Not Static Polynomials:** No generic static waveshapers or cosmetic approximations. Magnetic tape is modeled with non-linear differential hysteresis equations (Jiles-Atherton); transformers are modeled with frequency-dependent core flux saturation.
2. **Mastering-Grade Phase & Dynamic Integrity:** Pristine oversampling ($4\times / 8\times$ polyphase half-band) with $>96\text{ dB}$ stopband attenuation eliminates digital aliasing while preserving transient punch and low-end phase coherence.
3. **The Mauve Zone Aesthetic:** Heavy industrial studio console styling (Otari, Nagra, Buchla cues) rendered in brushed dark tungsten, amber/ochre phosphor graticules, and tactile bakelite controls.

---

## 2. End-to-End Signal Flow & Routing Matrix

The audio pipeline executes with sample-accurate latency compensation and zero-latency dry/wet crossfading:

```
Audio In ──► [Input Trim & M/S Matrix]
                 │
                 ├──► [4× / 8× Polyphase Oversampling Up-Sampler]
                 │        │
                 │        ├──► [Stage 1: Harmonic Line Stage (11 Flavors)]
                 │        │        │
                 │        │        ├──► [Stage 2: Bus Dynamics (VCA / Vari-Mu)]
                 │        │        │        │
                 │        │        │        └──► [Stage 3: Oxidation Tape Machine]
                 │        │        │                 (Hysteresis + Transport Wow/Flutter)
                 │        │        │
                 │        └──► [Decimation Down-Sampler & Anti-Aliasing Filter]
                 │
                 ├──► [Stage 4: True-Peak Lookahead Limiter (1ms - 5ms buffer)]
                 │        │
                 │        └──► [ITU-R BS.1770 LUFS / True-Peak Telemetry Bus]
                 │
                 └──► [Master Ceiling & Output Trim] ──► Audio Out
```

---

## 3. Stage 1: Harmonic Line Stage (The 11 Pro Flavors)

Stage 1 models the active electronic circuitry of vintage consoles, preamps, and mastering line amps. It features **11 distinct component-level flavors** organized into four physical chambers:

### Chamber A: The Glass Chamber (Vacuum Tubes)
* **01: Triode Class-A (12AX7 / 300B Single-Ended):**
  * *Transfer Equation:* Asymmetric polynomial $y(x) = x - \alpha x^2 - \beta x^3$ with DC-blocking servo.
  * *Acoustic Profile:* Strong **2nd-order harmonic bloom** (octave up). Widens perceived stereo depth, softens sharp transients, and adds silky warmth to vocals and acoustic instruments.
* **02: Pentode Push-Pull (EL34 / EF86 High-Plate):**
  * *Transfer Equation:* Symmetrical cubic soft-clipper $y(x) = \frac{x}{\sqrt{1 + x^2}}$ with adjustable knee.
  * *Acoustic Profile:* Dominant **3rd-order harmonics**. Adds aggressive midrange presence, density, and snap to snares, guitars, and modern synth mixes.
* **03: Starved-Plate Triode (Low-Voltage Anode):**
  * *Transfer Equation:* Asymmetrical soft-knee with dynamic grid-bias drift.
  * *Acoustic Profile:* Dark, vintage, smoky compression. Early harmonic onset with gentle high-frequency rounding.

### Chamber B: The Core Chamber (Audio Transformers & Inductors)
Transformers saturate non-linearly based on frequency: low frequencies saturate first because magnetic flux density is inversely proportional to frequency ($B \propto \frac{V}{f}$).
* **04: Iron Core Transformer (Neve Marinair / St. Ives 1073 style):**
  * *Model:* Frequency-weighted flux integration with core saturation threshold.
  * *Acoustic Profile:* **Low-end "girth" and authority.** Sub-bass and kicks bloom with rich 3rd-order harmonics while treble remains open and unclipped.
* **05: Nickel Core Transformer (Jensen / API 2520 style):**
  * *Model:* High-permeability core model with steep flux saturation knee.
  * *Acoustic Profile:* Ultra-clean low end with crisp, punchy transient snap and airy high-frequency sheen. Preserves transient attack.
* **06: Steel Core / Passive Inductor (Pultec EQP-1A style):**
  * *Model:* Inductive hysteresis with series reactance ($L \cdot \frac{di}{dt}$).
  * *Acoustic Profile:* Syrupy, resonant, slightly sluggish transient rounding. Tames harsh digital cymbals and binds the lower-midrange.

### Chamber C: The Solid-State Chamber (Semiconductors)
* **07: Class-A Discrete Silicon (API / Neve 1272 BJT Line Amp):**
  * *Model:* Bipolar junction transistor transfer function with exponential forward conduction.
  * *Acoustic Profile:* Fast, forward, punchy 3rd-harmonic drive. Tightens basslines and gives kicks immediate impact without low-end bloom.
* **08: Germanium Transistor (Chandler TG / Vintage EMI Console):**
  * *Model:* Shockley diode equation with ultra-low forward voltage ($V_f \approx 0.3\text{V}$) and thermal drift emulation.
  * *Acoustic Profile:* Uniquely warm, velvety, and organic. Soft-knee compression that rounds off harsh digital frequencies.
* **09: FET Line Driver (Field Effect Transistor / 1176 Line Stage):**
  * *Model:* Square-law drain-source conduction characteristic ($I_D \propto (V_{GS} - V_{th})^2$).
  * *Acoustic Profile:* Tube-like even harmonics combined with instantaneous solid-state slew rates.

### Chamber D: The Mauve Zone Exotics
* **10: Abbey Road Zener Diode Soft-Clipper (TG12413 style):**
  * *Model:* Dual back-to-back Zener diode breakdown curves.
  * *Acoustic Profile:* The mastering secret weapon for competitive loudness. Transparently shaves off microscopic transient spikes without dulling high-frequency air.
* **11: Oxide Rot & Flux Breakdown (Mauve Zone Signature):**
  * *Model:* Non-linear wavefolding combined with micro-stochastic voltage flutter.
  * *Acoustic Profile:* Gentle tape-head oxidation and circuit distress. At low drive, it adds a tactile, grainy patina; at high drive, it enters decaying tape-loop disintegration.

### Stage 1 Control Interface
* `FLAVOR`: Selector for the 11 circuit models.
* `DRIVE`: Precision input gain (-12 dB to +24 dB, 0.1 dB increments).
* `AUTO-GAIN`: Compensates output level inversely to drive, isolating tone from volume bias.
* `BIAS / TILT`: Adjusts DC-offset/asymmetry to morph between pure odd and rich even harmonics.
* `PRE-TILT`: Frequency tilt filter (Low Boost / High Cut $\leftrightarrow$ Flat $\leftrightarrow$ High Boost / Low Cut) prior to non-linear saturation.
* `MIX`: Parallel dry/wet blend (0% - 100%).

---

## 4. Stage 2: Master Bus Dynamics (The Glue Engine)

Stage 2 provides cohesive dynamic glue, leveling out micro-dynamics and tightening the mix before the tape machine.

### Topologies
1. **VCA Glue Mode (SSL 4000 G-Master Style):**
   * Feed-forward VCA design with hard/soft knee.
   * Ratios: `1.5:1` (gentle mastering), `2:1` (classic mix glue), `4:1` (tight punch).
   * Fast, punchy transient capture that clamps down on stray peaks while preserving rhythmic drive.
2. **Vari-Mu Tube Mode (Fairchild 670 / Manley Style):**
   * Feedback topology with continuously variable ratio that increases with gain reduction depth (from 1.1:1 up to 8:1).
   * Soft-knee transition providing rich, velvety, musical compression that blooms around sustained passages.

### Dynamic Control Matrix
* `THRESHOLD`: -30.0 dB to +10.0 dB.
* `ATTACK`: 0.1 ms, 0.3 ms, 1.0 ms, 3.0 ms, 10.0 ms, 30.0 ms.
* `RELEASE`: 100 ms, 300 ms, 600 ms, 1.2 s, and **AUTO** (dual-stage program-dependent release: rapid recovery for fast transients, slow recovery for sustained bass).
* `SC-HPF`: Internal sidechain high-pass filter (Off, 30 Hz – 250 Hz, 12 dB/octave) to prevent heavy bass from pumping the master bus.
* `SC-LISTEN`: Dedicated audition bus to monitor sidechain filter response.
* `PARALLEL MIX`: Internal clean parallel blend with zero phase distortion.

---

## 5. Stage 3: The Oxidation Tape Machine

Stage 3 models a complete, high-end 2-track mastering tape machine, combining electromagnetic tape physics with electro-mechanical transport behavior.

### 1. Magnetic Hysteresis Engine
Rather than using static waveshapers, the engine implements the differential **Jiles-Atherton model** of magnetic hysteresis:
$$\frac{dM}{dH} = (1 - c)\frac{M_{an} - M}{\delta k - \alpha(M_{an} - M)} + c \frac{dM_{an}}{dH}$$
* Accurately reproduces magnetic domain alignment, saturation limits, and residual flux memory.
* High-frequency self-erasure: as tape drive increases, high frequencies gently self-compress.
* Low-Frequency Head Bump: Resonant low-end shelf modeled after reproduce head pole-piece geometry (peaks around 55 Hz at 30 IPS, 38 Hz at 15 IPS).

### 2. Tape Formulations & Speeds
* **1/2" 30 IPS (Hi-Fi Master Deck - Studer/ATR):** Ultra-wide frequency response (up to 28 kHz), minimal head-bump, high headroom, extremely low noise floor.
* **1/4" 15 IPS (Fat/Punchy Analog Reel):** Pronounced head-bump, warm low-mids, smooth high-frequency rolloff, musical tape compression.
* **Type II Chrome Cassette (Lo-Fi / Vintage):** Saturated, grittier tape character with prominent hiss, high-frequency compression, and narrow stereo image.

### 3. Electro-Mechanical Transport Physics
* **Capstan Wow & Flutter:** Dual decoupled stochastic LFOs:
  * *Wow:* Slow 0.5 Hz – 2.0 Hz rotational drift caused by motor capstan eccentricity.
  * *Flutter:* Rapid 5.0 Hz – 15.0 Hz jitter caused by tape pack friction and guide vibration.
* **Oxide Wear & Dissipation:** High-frequency dissipation filter simulating aged or flaking magnetic particles over prolonged tape use.
* **Tape Bias Adjustment:**
  * *Overbias (+3 dB):* Smoother, rounder highs, reduced distortion, punchy lows.
  * *Underbias (-2 dB):* Crisper high-frequency edge with subtle intermodulation grit.

---

## 6. Stage 4: True-Peak Brickwall Limiter & Loudness Telemetry

The final guardian guarantees that the master complies with modern streaming delivery requirements without compromising dynamic punch.

### Limiter Architecture
* **True-Peak Detection:** Compliant with **ITU-R BS.1770-4**, employing $4\times$ oversampling to detect inter-sample peaks that clip digital-to-analog converters.
* **Lookahead Delay:** Precision 2.0 ms lookahead buffer allowing zero-splatter gain reduction on aggressive transients.
* **Program-Adaptive Release:** Dual-stage auto-release ensures transparent recovery from brief peaks while maintaining stability during sustained bass notes.
* **Ceiling Control:** -0.1 dBFS to -1.0 dBFS (standard -1.0 dBTP for Spotify/Apple Music master delivery).

### Telemetry Suite
* **EBU R128 Loudness Radar:**
  * Momentary LUFS (400ms window)
  * Short-Term LUFS (3s window)
  * Integrated LUFS (full session measurement)
  * Loudness Range (LRA)
* **True-Peak Max Meter:** Real-time peak hold in dBTP.
* **Gain Reduction History Scope:** Real-time scrolling amber phosphor oscilloscope showing limiter and compressor activity.

---

## 7. Technical Implementation Stack

| Layer | Technology | Specifications |
| :--- | :--- | :--- |
| **Language** | C++20 | ISO C++20 with strict memory safety & zero-allocation audio loop |
| **Framework** | JUCE 7/8 | VST3, CLAP, AU, Standalone |
| **Vector Math** | SIMD Intrinsics | SSE4.2, AVX2, and ARM Neon optimization for mathematical kernels |
| **Oversampling** | Polyphase Half-Band | `juce::dsp::Oversampling` (Selectable $2\times, 4\times, 8\times$, Linear-Phase) |
| **State Tree** | `APVTS` | Thread-safe `AudioProcessorValueTreeState` with non-locking atomics |
| **GUI Engine** | Custom OpenGL / Juce Graphics | 60 FPS vector HUD, amber phosphor shaders, rotating reel telemetry |

---

## 8. Hardware-Inspired GUI Layout

The MZOD-1 interface is structured across a rugged 19-inch 3U rack chassis:

```
┌──────────────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│  MAUVE ZONE // APPARATUS 02                   THE OXIDATION DECK                         [VST3 / CLAP / 64-BIT]  │
│  [BYPASS]  [PRESET: MASTER BUS - 30 IPS TUBE]                                           [OVERSAMPLE: 8× LINEAR]  │
├───────────────────┬───────────────────────────┬───────────────────────────┬──────────────────────────────────────┤
│  STAGE 1: LINE    │  STAGE 2: DYNAMICS        │  STAGE 3: TAPE MACHINE    │  STAGE 4: LIMITER & TELEMETRY        │
│  [FLAVOR SELECT]  │  [MODE: VCA | VARI-MU]    │  [SPEED: 15 | 30 IPS]     │  [CEILING: -1.0 dBTP]                │
│  (11 Models)      │                           │                           │                                      │
│  ( DRIVE )        │  ( THRESHOLD )            │  ( TAPE DRIVE )           │  ┌────────────────────────────────┐  │
│  ( BIAS  )        │  ( ATTACK )               │  ( BIAS CAL )             │  │ AMBER PHOSPHOR SCOPE           │  │
│  ( PRE-TILT )     │  ( RELEASE: AUTO )        │  ( WOW / FLUTTER )        │  │ GR History: [============]     │  │
│  ( DRY/WET )      │  ( SC-HPF: 85 Hz )        │  ( OXIDE WEAR )           │  │ LUFS-I: -14.1 | TP: -1.0 dBTP  │  │
│                   │  ( PARALLEL MIX )         │  ( TAPE SAT MIX )         │  └────────────────────────────────┘  │
├───────────────────┴───────────────────────────┴───────────────────────────┴──────────────────────────────────────┤
│  [IN METERS: L/R]            [M/S PROCESSING: STEREO]            [CLIP WARN: 0]           [OUT METERS: L/R]      │
└──────────────────────────────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 9. Development & Verification Roadmap

1. **Phase 1: DSP Saturation Kernel Library**
   * Code the 11 mathematical saturation curves with standalone unit tests.
   * Verify harmonic spectra (2nd, 3rd, 5th order) via FFT spectral analyzer.
   * Integrate $4\times / 8\times$ polyphase oversampling and measure aliasing rejection ($<-96\text{ dBFS}$).
2. **Phase 2: Master Dynamics & True-Peak Limiter**
   * Implement VCA and Vari-Mu envelope generators with auto-release state machines.
   * Build the ITU-R BS.1770-4 inter-sample peak detector and lookahead limiter ring buffer.
3. **Phase 3: The Unified Tape Engine**
   * Implement the Jiles-Atherton hysteresis state solver.
   * Model the 15/30 IPS head-bump IIR biquad filters and dual-LFO wow/flutter transport physics.
4. **Phase 4: JUCE AudioProcessor & State Architecture**
   * Build the complete APVTS parameter tree, preset management, and thread-safe audio callback.
5. **Phase 5: Mauve Zone Industrial UI**
   * Render the dark tungsten chassis, amber CRT phosphor scope, precision rotary knobs, and interactive tape visualizers.
6. **Phase 6: Audio Verification & Master Bus Benchmarking**
   * A/B testing on full mix stems against commercial reference masters.

---
*Mauve Zone // Boutique Instrument Laboratory // Apparatus MZOD-1*

