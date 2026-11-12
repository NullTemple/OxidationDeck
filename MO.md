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

