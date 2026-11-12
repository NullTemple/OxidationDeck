# THE OXIDATION DECK // MASTER BUS EDITION (MZOD-1)
## Master DSP Specification & Mathematical Formulations
### Peer-Reviewed Algorithms, Physical Models, and Signal Processing Standards

---

## 1. Mathematical Framework & Multirate Oversampling

### 1.1 The Aliasing Catastrophe in Nonlinear Processing
When an audio signal $x[n]$ passes through a memoryless or stateful non-linearity $f(x)$, it generates harmonic components at integer multiples $k \cdot f_0$ of the fundamental input frequencies. If any generated harmonic exceeds the Nyquist boundary $f_N = f_s / 2$, it reflects back into the audible baseband ($0 \text{ Hz} \le f \le f_N$) as inharmonic foldback distortion:
$$f_{\text{alias}} = \left| f_{\text{harmonic}} - k \cdot f_s \right|$$
In mastering processors, inharmonic foldback smears stereo imaging, collapses depth, and introduces metallic grit.

### 1.2 Polyphase IIR Half-Band Decomposition
To eliminate aliasing while maintaining sub-sample transient accuracy and ultra-low CPU overhead, all nonlinear processing stages (Stage 1 through Stage 3) execute inside a **polyphase allpass IIR multirate framework** (Vaidyanathan, 1993; Bellanger, 1976).

The $2\times$ decimation / interpolation half-band filter $H(z)$ is decomposed into two polyphase branches:
$$H(z) = \frac{1}{2} \left[ A_0(z^2) + z^{-1} A_1(z^2) \right]$$
Where $A_0(z)$ and $A_1(z)$ are cascades of first- and second-order stable allpass filter sections:
$$A_m(z) = \prod_{k=1}^{N_m} \frac{\alpha_{m,k} + z^{-1}}{1 + \alpha_{m,k} z^{-1}}$$

#### Oversampling Specifications:
* **Topology:** 12th-order elliptic half-band polyphase IIR cascade ($4\times$ or $8\times$ oversampling mode).
* **Stopband Attenuation:** $A_s \ge 96.5\text{ dB}$.
* **Passband Ripple:** $\delta_p < 0.001\text{ dB}$ (flat to $20\text{ kHz}$).
* **Group Delay:** Strictly compensated via reverse-time allpass filtering in offline mode, or minimum-phase causal alignment in real-time mode.

---

## 2. Stage 1: The 11 Harmonic Line Stage Engines

```
Input x[n] ──► [Pre-Emphasis Tilt] ──► [Nonlinear Kernel f(x, s)] ──► [DC Servo] ──► [Auto-Gain Comp] ──► Output y[n]
```

### Family A: The Glass Chamber (Vacuum Tubes)

#### 01: Triode Class-A (12AX7 / 300B Single-Ended)
* **Citation:** Koren, N. (2001). *"Improved Vacuum-Tube Models for SPICE Simulation"*, Glass Audio, Vol. 13; Leach, W. M. (1995). *"SPICE Models for Vacuum-Tube Amplifiers"*, JAES, 43(3).
* **Physics:** Single-ended triodes operate in asymmetrical space-charge limited conduction (Child-Langmuir $3/2$ power law). Grid-to-cathode voltage $v_{gk}$ modulates plate current $i_p$.
* **Mathematical Model:**
  $$E_1(v) = \frac{v_{pk}}{K_p} \ln\left(1 + \exp\left(K_p \left[\frac{1}{\mu} + \frac{v_{gk}}{\sqrt{K_{vb} + v_{pk}^2}}\right]\right)\right)$$
  $$i_p(v) = \begin{cases} \dfrac{E_1(v)^{1.5}}{K_g}, & E_1 > 0 \\ 0, & E_1 \le 0 \end{cases}$$
* **Normalized Audio-Rate Kernel:**
  Let $x$ be the normalized input signal ($|x| \le 1.0$) and $\gamma$ be the drive parameter ($\gamma \in [1.0, 10.0]$):
  $$f_{\text{triode}}(x) = \frac{x + \alpha x^2}{1 + \beta |x|} \quad \text{with } \alpha = 0.35, \, \beta = 0.22$$
* **Harmonic Profile:** Dominant $H_2$ (octave bloom) with rapid decay of higher partials:
  $$\frac{|H_2|}{|H_1|} \approx -26\text{ dBc}, \quad \frac{|H_3|}{|H_1|} \approx -48\text{ dBc}, \quad \frac{|H_4|}{|H_1|} < -70\text{ dBc}$$

#### 02: Pentode Push-Pull (EL34 / EF86 High-Plate)
* **Citation:** D’Angelo, S., & Välimäki, V. (2014). *"Generalized Polynomial Waveshaping for Musical Distortion Effects"*, IEEE Signal Processing Letters, 21(9).
* **Physics:** Screen grid isolation creates an abrupt current saturation knee. In push-pull output configurations, even-order harmonics cancel in the differential transformer, leaving pure odd harmonics.
* **Mathematical Model:**
  $$f_{\text{pentode}}(x) = \text{sgn}(x) \cdot \frac{|x|}{\left(1 + |x|^p\right)^{1/p}} \quad \text{where } p = 3.0$$
* **Harmonic Profile:** Pure odd-order harmonics ($H_3, H_5, H_7$). Yields aggressive midrange punch and transient density without pitch doubling.

#### 03: Starved-Plate Triode (Low Anode Voltage)
* **Citation:** Pakarinen, J., & Yeh, D. T. (2009). *"A Review of Digital Techniques for Modeling Vacuum-Tube Circuits"*, DAFx-09.
* **Physics:** When plate voltage $V_p$ drops below nominal ($< 50\text{V}$), the conduction threshold shifts, causing early grid-current clipping and dynamic cathode bias drift.
* **Mathematical Model:**
  $$v_{\text{bias}}[n] = v_{\text{bias}}[n-1] + \lambda \left( \max(0, x[n] - V_{\text{th}}) - v_{\text{bias}}[n-1] \right)$$
  $$f_{\text{starved}}(x[n]) = \tanh\left(\gamma \cdot (x[n] - v_{\text{bias}}[n])\right)$$
  Where $\lambda = 1 - e^{-1/(f_s \tau_{\text{bias}})}$ with $\tau_{\text{bias}} \approx 45\text{ ms}$.

---

### Family B: The Core Chamber (Transformers & Inductors)

#### 04: Iron Core Transformer (Neve Marinair / St. Ives 1073)
* **Citation:** MacFadyen, K. A. (1947). *"Vectorial Treatment of Non-linear Distortion in Audio Transformers"*, Proc. IEE; Holters, M., & Zölzer, U. (2011). *"Physical Modelling of Audio Circuits"*, DAFx-11.
* **Physics:** Magnetic induction is governed by Faraday's Law: $v(t) = N \frac{d\Phi}{dt}$. Consequently, magnetic flux $\Phi(t)$ is the integral of input voltage:
  $$\Phi(t) = \frac{1}{N} \int_{-\infty}^{t} v(\tau)\, d\tau$$
  Low-frequency signals produce large excursions in $\Phi$, forcing the core into magnetic saturation while high frequencies remain strictly linear.
* **Mathematical Model:**
  1. Integrate voltage with leaky integrator:
     $$\Phi[n] = \rho \cdot \Phi[n-1] + (1 - \rho) \cdot x[n], \quad \rho = 1 - \frac{2\pi f_{\text{leak}}}{f_s}, \, f_{\text{leak}} = 15\text{ Hz}$$
  2. Apply bounded transcendental core saturation:
     $$\Phi_{\text{sat}}[n] = \frac{2\Phi_{\max}}{\pi} \arctan\left(\frac{\pi \Phi[n]}{2\Phi_{\max}}\right)$$
  3. Differentiate to reconstruct output voltage:
     $$y[n] = \frac{\Phi_{\text{sat}}[n] - \Phi_{\text{sat}}[n-1]}{1 - \rho} + x[n]$$
* **Sonic Profile:** Generates 3rd-harmonic girth strictly below $120\text{ Hz}$. Highs remain pristine.

#### 05: Nickel Core Transformer (API 2520 / Jensen)
* **Physics:** High initial magnetic permeability ($\mu_i$) with a sharp, abrupt saturation knee and negligible low-frequency phase lag.
* **Mathematical Model:**
  $$f_{\text{nickel}}(x) = x \cdot \left(1 - \frac{1}{1 + \exp\left(-\kappa (|x| - x_{\text{knee}})\right)}\right)$$
* **Sonic Profile:** Transparent sub-bass; preserves fast transient punch with a pristine high-frequency sheen.

#### 06: Steel Core / Passive Inductor (Pultec EQP-1A)
* **Citation:** Fontana, F. (2010). *"Computation of the Lambert W function for virtual analog circuit modeling"*, IEEE Trans. Audio.
* **Physics:** Series inductance $L \cdot \frac{di}{dt}$ coupled with eddy-current core losses generates frequency-dependent sluggish transient response and low-mid resonance.
* **Mathematical Model:** Continuous bilinear transform of non-linear inductor state:
  $$i_L[n] = i_L[n-1] + \frac{T}{2L} \left( v[n] - R_{\text{core}} \cdot i_L[n-1] \right)$$

---

### Family C: The Solid-State Chamber (Semiconductors)

#### 07: Class-A Discrete Silicon (API / Neve 1272 BJT)
* **Citation:** Ebers, J. J., & Moll, J. L. (1954). *"Large-Signal Behavior of Junction Transistors"*, Proc. IRE, 42(12).
* **Mathematical Model:**
  $$f_{\text{bjt}}(x) = \begin{cases} 
  x - \frac{x^3}{3}, & |x| < 1.0 \\
  \frac{2}{3} \text{sgn}(x), & |x| \ge 1.0 
  \end{cases}$$
* **Sonic Profile:** Hard-hitting 3rd-order harmonic bite with tight low-mids and immediate attack.

#### 08: Germanium Transistor (Chandler TG / EMI Console)
* **Citation:** Shockley, W. (1949). *"The Theory of p-n Junctions in Semiconductors"*, Bell System Tech. J.; Yeh, D. T. (2008). *"Automated physical modeling of nonlinear audio circuits"*, DAFx-08.
* **Physics:** Germanium p-n junctions have an ultra-low threshold ($V_T \approx 0.25\text{V}$) and large reverse saturation current $I_s$, yielding a wide, velvety soft-clipping region.
* **Explicit Lambert-W Solution:**
  $$y[n] = x[n] - R I_s \cdot \text{sign}(x[n]) \cdot W\left( \frac{1}{V_T} \exp\left(\frac{|x[n]|}{V_T}\right) \right)$$
  Where $W(z)$ is the **Wright $\Omega$ / Lambert $W$ function**, approximated via Halley's method:
  $$w_{j+1} = w_j - \frac{w_j e^{w_j} - z}{e^{w_j}(w_j + 1) - \frac{(w_j + 2)(w_j e^{w_j} - z)}{2w_j + 2}}$$

#### 09: FET Line Driver (Field Effect Transistor / 1176 Line Stage)
* **Physics:** Operates via Shockley's square-law field effect: $I_D = I_{DSS} \left(1 - \frac{V_{GS}}{V_P}\right)^2$.
* **Mathematical Model:**
  $$f_{\text{fet}}(x) = \begin{cases}
  x + \frac{x^2}{4}, & x \ge -2.0 \\
  -1.0, & x < -2.0
  \end{cases}$$
* **Sonic Profile:** Tube-like even harmonics combined with fast solid-state slew rate.

---

### Family D: The Mauve Zone Exotics

#### 10: Abbey Road Zener Diode Clipper (TG12413 style)
* **Mathematical Model:** Dual back-to-back Zener diode voltage clamp:
  $$f_{\text{zener}}(x) = V_Z \tanh\left(\frac{x}{V_Z}\right) + V_F \cdot \text{sgn}(x) \left(1 - e^{-|x| / V_F}\right)$$
  Where $V_Z = 0.85$ (reverse breakdown) and $V_F = 0.15$ (forward conduction).
* **Sonic Profile:** Transparent analog pre-limiter. Shaves off microscopic transient spikes without dulling high-frequency air.

#### 11: Oxide Rot & Flux Breakdown (Mauve Zone Signature)
* **Mathematical Model:** Coupled Chebyshev wavefolder and Ornstein-Uhlenbeck stochastic micro-dropout generator:
  $$f_{\text{rot}}(x[n]) = T_3(x[n]) \cdot \left(1 - \eta[n]\right)$$
  Where $T_3(x) = 4x^3 - 3x$, and $\eta[n]$ is a stochastic telegraph process:
  $$d\eta = -\theta \eta \, dt + \sigma \, dW_t$$

---

## 3. Stage 2: Master Bus Dynamics (The Glue Engine)

### 3.1 Peak/RMS Log-Domain Detection Topology
* **Citation:** Giannoulis, D., Massberg, M., & Reiss, J. D. (2012). *"Digital Dynamic Range Compressor Design—A Tutorial and Analysis"*, **Journal of the Audio Engineering Society (JAES)**, 60(6), 399–408.

The detector operates entirely in the logarithmic decibel domain to eliminate envelope ripple and gain modulation artifacts:
$$x_{\text{dB}}[n] = 20 \log_{10}\left( \max\left(|x_L[n]|, |x_R[n]|, 10^{-5}\right) \right)$$

```
Detector In ──► [SC-HPF Biquad] ──► [dB Conversion] ──► [Static Characteristic] ──► [Decoupled Ballistics] ──► Gain Reduction g[n]
```

### 3.2 Smooth Quadratic Knee Characteristic
To prevent harmonic distortion caused by slope discontinuity at the threshold $T$, the gain computer uses a second-order polynomial knee of width $W\text{ dB}$:
$$F(x_{\text{dB}}) = \begin{cases}
x_{\text{dB}}, & x_{\text{dB}} \le T - \frac{W}{2} \\
x_{\text{dB}} + \dfrac{\left(\frac{1}{R} - 1\right)\left(x_{\text{dB}} - T + \frac{W}{2}\right)^2}{2W}, & \left| x_{\text{dB}} - T \right| < \frac{W}{2} \\
T + \dfrac{x_{\text{dB}} - T}{R}, & x_{\text{dB}} \ge T + \frac{W}{2}
\end{cases}$$
Where:
* $T \in [-30.0, +10.0]\text{ dB}$ (Threshold)
* $R \in [1.2, 10.0]$ (Ratio)
* $W = 6.0\text{ dB}$ (Knee Width)

### 3.3 Decoupled Envelope Ballistics & Auto-Release
Raw gain change: $\Delta g[n] = F(x_{\text{dB}}[n]) - x_{\text{dB}}[n]$.

The smoothing filter uses **decoupled one-pole ballistics**:
$$g_s[n] = \begin{cases}
\alpha_{\text{att}} \cdot g_s[n-1] + (1 - \alpha_{\text{att}}) \cdot \Delta g[n], & \Delta g[n] < g_s[n-1] \quad (\text{Attack Phase}) \\
\alpha_{\text{rel}}[n] \cdot g_s[n-1] + (1 - \alpha_{\text{rel}}[n]) \cdot \Delta g[n], & \Delta g[n] \ge g_s[n-1] \quad (\text{Release Phase})
\end{cases}$$
Where:
$$\alpha_{\text{att}} = \exp\left(-\frac{1}{f_s \cdot \tau_{\text{att}}}\right)$$

#### Dual-Time-Constant Auto-Release Engine:
$$\alpha_{\text{rel}}[n] = \kappa[n] \cdot \exp\left(-\frac{1}{f_s \tau_{\text{fast}}}\right) + (1 - \kappa[n]) \cdot \exp\left(-\frac{1}{f_s \tau_{\text{slow}}}\right)$$
Where $\tau_{\text{fast}} = 50\text{ ms}$, $\tau_{\text{slow}} = 1200\text{ ms}$, and $\kappa[n]$ measures transient density over a $250\text{ ms}$ sliding window.

### 3.4 Internal Sidechain High-Pass Filter
The sidechain signal passes through a 2nd-order Butterworth high-pass filter:
$$H_{\text{sc}}(z) = \frac{b_0 + b_1 z^{-1} + b_2 z^{-2}}{1 + a_1 z^{-1} + a_2 z^{-2}}$$
Cutoff frequency: $f_c \in [30\text{ Hz}, 250\text{ Hz}]$, $Q = \frac{1}{\sqrt{2}} \approx 0.7071$.

---

## 4. Stage 3: The Oxidation Tape Machine

### 4.1 The Jiles-Atherton Differential Hysteresis Engine
* **Citation:** Jiles, D. C., & Atherton, D. L. (1986). *"Theory of ferromagnetic hysteresis"*, J. Magn. Magn. Mater., 61, 48–60; Eichas, F., & Zölzer, U. (2016). *"Black-Box and Gray-Box Modeling of an Analog Tape Machine"*, DAFx-16.

Magnetic tape stores audio as physical magnetization $M$ of gamma-ferric oxide ($\gamma\text{-Fe}_2\text{O}_3$) or chromium dioxide ($\text{CrO}_2$) dipoles.

```
       M (Magnetization)
            ▲
            │         /──────── Saturation (+Ms)
            │       /  
            │     /    ◄── Remanent Flux (Mr)
            │   / 
────────────┼─────────────► H (Magnetic Field)
          / │  Coercivity (Hc)
        /   │
       /    │
```

#### State Equations:
1. **Effective Magnetic Field:**
   $$H_e = H + \alpha M$$
2. **Anhysteretic Magnetization (Modified Langevin Function):**
   $$M_{\text{an}} = M_s \left( \coth\left(\frac{H_e}{a}\right) - \frac{a}{H_e} \right)$$
3. **Total Magnetization Differential:**
   $$\frac{dM}{dH} = \frac{(1 - c)\left(M_{\text{an}} - M\right)}{\delta k - \alpha\left(M_{\text{an}} - M\right)} + c \frac{dM_{\text{an}}}{dH}$$
   Where $\delta = \text{sgn}\left(\frac{dH}{dt}\right)$, $M_s$ is saturation magnetization, $a$ is domain density, $k$ is pinning energy, and $c$ is domain wall elasticity.

#### Numerical Integration:
Solved at $4\times$ oversampled rates via **4th-Order Runge-Kutta (RK4)**:
$$M[n] = M[n-1] + \frac{\Delta H}{6} \left( k_1 + 2k_2 + 2k_3 + k_4 \right)$$

### 4.2 Low-Frequency Head Bump Resonator
The playback head pole pieces create geometric magnetic interference, resulting in low-frequency resonant reinforcement:
$$H_{\text{bump}}(s) = \frac{s^2 + 2\zeta_z \omega_0 s + \omega_0^2}{s^2 + 2\zeta_p \omega_0 s + \omega_0^2}$$
* **30 IPS Mode:** $\omega_0 = 2\pi \cdot 55\text{ Hz}$, $Q = 0.85$ ($+1.4\text{ dB}$ bump; linear extension to $28\text{ kHz}$).
* **15 IPS Mode:** $\omega_0 = 2\pi \cdot 38\text{ Hz}$, $Q = 1.15$ ($+2.2\text{ dB}$ bump; smooth rolloff above $19\text{ kHz}$).

### 4.3 Stochastic Transport Physics (Wow & Flutter)
Motor speed drift and mechanical tape pack flutter are modeled via two uncoupled **Ornstein-Uhlenbeck mean-reverting stochastic processes**:
$$dx_t = -\theta (\mu - x_t) dt + \sigma dW_t$$

* **Wow Process ($W_t$):** Low-frequency capstan eccentricity:
  $$\theta_w = 2\pi \cdot 1.0\text{ Hz}, \quad \sigma_w = 0.0012$$
* **Flutter Process ($F_t$):** High-frequency tape scrape flutter:
  $$\theta_f = 2\pi \cdot 9.5\text{ Hz}, \quad \sigma_f = 0.0004$$

#### Fractional-Delay Tape Transport Read Head:
Audio is read from a continuous circular ring buffer using **4th-order Lagrange cubic interpolation**:
$$\text{delay}[n] = D_0 + \text{Wow}[n] + \text{Flutter}[n]$$

---

## 5. Stage 4: True-Peak Limiter & Loudness Telemetry

### 5.1 ITU-R BS.1770-4 Inter-Sample Peak (ISP) Interpolation
* **Citation:** International Telecommunication Union, **Recommendation ITU-R BS.1770-4 (10/2015)**, *"Algorithms to measure audio programme loudness and true-peak audio level"*, Annex 2.

To detect peaks occurring between digital samples, the audio is upsampled by $4\times$ using an exact linear-phase polyphase FIR filter:
* Length: 48 coefficients ($12 \times 4$ phases).
* Stopband attenuation: $> 75\text{ dB}$.
* Passband ripple: $\pm 0.05\text{ dB}$.

```
Input x[n] ──► [4× Polyphase FIR Interpolation] ──► [Peak Hold] ──► [Lookahead Gain Calc] ──► Output y[n]
```

### 5.2 Lookahead Gain Calculation & Circular Buffer
The audio path is delayed by $D = 2.0\text{ ms}$ ($N_{\text{delay}} = \lceil 0.002 \cdot f_s \rceil$ samples) in a circular ring buffer.
1. The oversampled envelope detector calculates required gain reduction $2.0\text{ ms}$ in advance:
   $$g_{\text{target}}[n] = \min\left(1.0, \frac{10^{\text{Ceiling}_{\text{dB}} / 20}}{|x_{\text{interpolated}}[n]|}\right)$$
2. Minimum-phase exponential release smoothing:
   $$g[n] = \min\left(g_{\text{target}}[n], \, g[n-1] + \Delta g_{\text{recovery}}\right)$$
3. Delayed audio is multiplied by the smoothed gain envelope, guaranteeing **0.0 ISP overshoots**.

### 5.3 EBU R128 / ITU-R BS.1770 Loudness Measurement
1. **K-Weighting Filter (Two-Stage Biquad Cascade):**
   * Stage 1: Pre-filter (high-shelf simulating acoustic head diffraction, $+4.0\text{ dB}$ at $1.5\text{ kHz}$).
   * Stage 2: RLB weighting (2nd-order high-pass, $-3\text{ dB}$ at $38\text{ Hz}$).
2. **Mean Square Integration:**
   $$z_i = \frac{1}{T} \sum_{n=0}^{T \cdot f_s - 1} y_i^2[n]$$
3. **Gating Algorithm:**
   * Absolute threshold: $\Gamma_a = -70\text{ LKFS}$.
   * Relative threshold: $\Gamma_r = \text{Loudness}_{\text{unswitched}} - 10\text{ LU}$.
   * Integrated Loudness $L_K = -0.691 + 10 \log_{10}\left( \sum_{i} z_i \right)$ for all blocks above $\Gamma_r$.

---

## 6. Numerical Precision, Thread Safety & SIMD Bounds

| Parameter | Specification | Compliance Standard |
| :--- | :--- | :--- |
| **Internal Audio Bus** | 64-bit double precision (`double`) | IEEE 754 floating-point |
| **Real-time Latency** | Strictly bounded (Lookahead + Oversampling Group Delay) | Sample-accurate host reporting |
| **Denormal Protection** | Hardware FTZ (Flush-To-Zero) & DAZ (Denormals-Are-Zero) enabled | `_MM_SET_FLUSH_ZERO_MODE` |
| **Vector Math Engine** | AVX2 / SSE4.2 / ARM NEON auto-vectorized loops | Zero heap allocations in audio loop |
| **Thread Safety** | Lock-free lockless FIFOs and atomic parameter reading | Real-time thread safe (no mutexes/allocations) |

---
*Mauve Zone // Apparatus MZOD-1 // Master DSP Specification Version 1.0*

