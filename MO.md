# Modus Operandi (MO)
## Project: Mauve Zone Oxidation Deck (MZOD-1)

---

### 1. Project Philosophy & Core Values

* **0 Excess Code Policy:**
  * Zero dead code, zero unneeded dependencies, zero premature abstractions.
  * Every DSP class and function must strictly justify its existence in acoustic character and CPU efficiency.
  * Clean, readable, and highly maintainable modern C++ (C++20/JUCE).
  * Professional grade DSP code, pristine.
* **Stochastic Over Naive Random:**
  * No flat uniform white noise (`rand()`) for analog modeling.
  * Analog non-idealities (clock drift, thermal instability, splice modulation) are modeled using structured stochastic processes (Brownian walks, Ornstein-Uhlenbeck bounded processes, 1/f noise, and leaky integrators).

---

### 2. Team Roster & Roles

| Role | Agent / Person | Responsibility |
| :--- | :--- | :--- |
| **Project Lead & QA** | **Alex / Mauve Zone** | Project vision, Epic/Story/Task breakdown, task assignment, design trade-off sign-offs, final QA testing and acceptance before any task is marked **Done**. |
| **Junior** | **Antigravity (AI)** | Active in-IDE pair programmer. Conducts Prep Sessions, implements clean C++ DSP & JUCE code, self-verifies, and maintains all living documentation. |
| **Big Daddy** | **Argon 4** | High-leverage DSP architecture, deep math formulation, critical algorithmic roadblocks (consulted selectively). |
| **Zombie** | **Jules** | Headless repository bot for batch tasks, renames, and repo maintenance. |
| **The Tourist** | **Gemini Web** | External reference, creative ideation, and comparative analysis without direct codebase context. |

---

### 3. Task Lifecycle & SDLC Workflow

Every unit of work follows a strict 4-step pipeline:

```
[1. Task Assignment] ──► [2. Prep Session] ──► [3. Implementation] ──► [4. QA Verification & Sign-off]
       (PM)                 (Junior + PM)             (Junior)                     (PM)
```

1. **Task Assignment (PM):**
   * The PM selects and assigns a discrete Task from `ROADMAP.md`.
2. **Prep Session (Junior & PM):**
   * *Before any code is written*, Junior produces a thorough technical brief addressing:
     * Scope & intent.
     * Dependencies & signal graph impacts.
     * Architectural edge cases & traps (denormals, buffer bounds, multithreading, aliasing, phase cancellation).
     * Exposed parameters, nominal ranges, and audio-rate behavior.
     * Design options requiring PM decision.
   * Implementation begins **only** after the PM signs off on the prep brief.
3. **Implementation & Self-Verification (Junior):**
   * Focused implementation strictly adhering to the 0 Excess Code policy.
   * Verification against build criteria and sanity checks.
   * Test cases for verification of code.
4. **QA Verification & Done (PM):**
   * PM tests and inspects the result against acceptance criteria.
   * Upon PM approval:
     * Task is marked `Done` in `ROADMAP.md`.
     * Detailed entry is recorded in `CHANGELOG.md`.
     * `TOPOGRAPHY.svg` and `TOPOGRAPHY.md` are updated if signal routing or parameters changed.

---

### 4. Living Documentation Standards

* **`MO.md`:** Governing processes, team definitions, and engineering rules (this document).
* **`ROADMAP.md`:** Current state of Epics, Stories, and Tasks with clear status indicators and effort estimations tracked in **model quota / usage %** rather than clock hours:
  * `[ ] Backlog`
  * `[~] Prep / In Discussion`
  * `[>] In Progress`
  * `[?] In QA`
  * `[x] Done`
* **`CHANGELOG.md`:** Reverse-chronological record of every change, referencing tasks and component impacts.
* **`TOPOGRAPHY.svg` & `TOPOGRAPHY.md`:** Vector-based visual schematic of signal flow, nodes, modulation paths, and nominal values.

---

### 5. Multi-Monitor DPI & VST3 UI Scaling Standard (`Derive` ContentPanel Pattern)

To guarantee rock-solid mouse hit-testing across Windows monitors with mixed DPI settings and screen resolutions, all Mauve Zone JUCE plugins standardize on the **`Derive` `ContentPanel` + `applyUiScale` architecture**:

1. **Avoid Constrainer-Based Window Resizing in VST3:**
   * Do **not** use `setResizable(true, true)`, `setResizeLimits(...)`, `setFixedAspectRatio(...)`, or `getPrimaryDisplay()` auto-shrinking in `AudioProcessorEditor`. During cross-monitor dragging (`WM_DPICHANGED` / VST3 `IPlugViewContentScaleSupport`), host DPI updates conflict with JUCE bounds constrainers and break mouse hit-testing against `HWND` client coordinates.
2. **Unified `ContentPanel` Coordinate Space:**
   * Keep the outer `AudioProcessorEditor` as a lightweight host frame (`paint` only fills `spaceBg`) and host all custom painting (`paintContent`), child controls (`resizedContent`), and canvas click handlers (`mouseDownContent`) inside a single `ContentPanel` child component with default hit-testing (`setInterceptsMouseClicks(true, true)`—never `false, true`).
   * Reset `contentPanel.setTransform(juce::AffineTransform())` before `contentPanel.setBounds(0, 0, kBaseWidth, kBaseHeight)`, then apply `contentPanel.setTransform(juce::AffineTransform::scale(uiScale))` and invoke `resizedContent()`.
3. **Explicit User UI Scale Selector (`75% / 100% / 125%`):**
   * Provide a `scaleBox` selector in the top utility bar that drives `applyUiScale(float newScale)` (`setSize(std::round(kBaseWidth * uiScale), std::round(kBaseHeight * uiScale)); resized();`), allowing users to downscale cleanly on smaller displays while leaving per-monitor OS DPI scaling intact.

