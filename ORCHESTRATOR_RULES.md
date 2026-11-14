# Orchestrator Token-Saving & Delegation Protocol (Argon -> Junior / Hydrogen)

Use this file as a project rule (`.agents/rules/orchestrator-protocol.md` or `ORCHESTRATOR_RULES.md`) across projects to prevent the Planning Model (Orchestrator / Argon) from burning quota on work that belongs on the Execution Model (Junior / Hydrogen) or lightweight subagents.

---

## 1. Core Mandate: Zero Heavy Investigation on the Planning Model
- **Every tool call in Plan Mode re-bills the entire conversation context at Planning Model rates.**
- When Plan Mode is active, **DO NOT** read more than **1–3 project files** (or a single targeted `grep_search`) before calling `exit_plan_mode`.
- **NEVER** read third-party framework or library internals (e.g., `JUCE/modules/*`, `node_modules/*`, SDK headers) on the Planning Model.
- If broad codebase exploration or multi-file inspection is required first, either:
  1. Hand off immediately via `exit_plan_mode` with a high-level behavioural/architectural plan that lets Junior (`Hydrogen`) inspect the exact symbol names on disk, **OR**
  2. Delegate read-only discovery to a `research` subagent with `Model: "flash"` / `"flash_lite"` so exploration runs in a cheap, isolated context window.

---

## 2. Prevent `enter_plan_mode` Bounce-Backs (Single-Pass Hand-Off)
A hand-back (`enter_plan_mode`) from Junior to the Orchestrator doubles Planning Model token usage because the Orchestrator has to re-read everything Junior just loaded. To prevent bounce-backs:
1. **Never guess symbol or parameter names from memory or compacted summaries.**
   - If you have not read a file in the current turn, **do not hardcode guessed variable names, struct fields, or function signatures** in the plan.
   - Instead, specify the **file path, the logical section/function to inspect, and the exact invariant/algorithmic change** so Junior uses the actual symbol names on disk without triggering a "plan assumed something that is not so" hand-back.
2. **Pre-authorize local implementation details in the plan:**
   - Explicitly state in the plan: *"Use the exact member and parameter ID names present in `Source/...` on disk; do not call `enter_plan_mode` for minor symbol-name differences or helper signatures."*
3. **Reserve `Re-plan if:` strictly for true blockers:**
   - Only list genuine architectural blockers in `Re-plan if:` (e.g., missing dependencies, unresolvable build failures after 2 attempts, or ambiguous product requirements that require user input).

---

## 3. Strict Division of Labor

| Task Type | Assigned Model / Agent | Allowed Actions on Orchestrator (Argon) |
| :--- | :--- | :--- |
| **Routine Edits, Bug Fixes, UI Tweaks, Builds, Git** | **Junior (`Hydrogen`)** via `exit_plan_mode` | **0–2 file reads max**, then immediate `exit_plan_mode`. |
| **Multi-File Codebase Search / Surveying** | **`research` Subagent (`Model: "flash"`)** or **Junior** | Single `invoke_subagent` call or immediate `exit_plan_mode`. |
| **DSP / Architecture Design & Root-Cause Analysis** | **Orchestrator (`Argon`)** + **Junior (`Hydrogen`)** | Read only the specific 1–2 DSP/processor files involved; formulate the math/architecture; hand all edits, builds, smoke tests, and commits to Junior. |
| **Direct Questions / Explanations** | **Orchestrator (`Argon`)** | Answer directly in text with **zero tool calls** (do not call `exit_plan_mode` when no file/command action is needed). |

---

## 4. Context Hygiene Rules
1. **Batch Parallel Tool Calls:** If the Orchestrator must read 2 files, read them in a **single parallel turn**, never sequentially across multiple turns.
2. **Keep Plans Compact & High-Signal:** Do not paste hundreds of lines of unchanged boilerplate code into `exit_plan_mode`. Specify only the target lines/blocks and the exact delta.
3. **Recommend `/compact` or Fresh Threads at Milestones:** When a thread accumulates a large history (e.g., after completing a major feature or release), remind the user to run `/compact` or start a fresh thread before beginning unrelated debugging so accumulated context is not re-billed on every step.
