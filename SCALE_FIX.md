# Multi-Monitor Windows VST3 Scaling & Mouse Hit-Testing Fix Guide

This guide documents the permanent fix for the Windows multi-monitor VST3 bug where dragging a JUCE plugin window to another monitor with a different resolution or DPI scale factor causes knobs, sliders, and buttons to stop registering mouse clicks. Both **Mauve Zone Derive** (`C:\Users\xaris\Derive`) and **The Oxidation Deck** (`c:\Users\xaris\OxidationDeck`) use this exact architecture.

---

## 1. Root Cause Summary

1. **Primary Culprit — JUCE `9.0.4` (`C:/JUCE`) Windows VST3 Multi-Monitor Regression:**
   - `C:/JUCE` on this system is **JUCE `9.0.4`**, which suffers from a Windows VST3 per-monitor DPI / `HWNDComponentPeer` coordinate desynchronization bug when moving a plugin editor across monitors with mixed DPI or resolution.
   - **JUCE `8.0.4`** (already cached locally at `C:/Users/xaris/Derive/build/_deps/juce-src`) does not exhibit this regression.
2. **Secondary Culprit — `setResizable` / `ComponentBoundsConstrainer` & Split Hit-Testing:**
   - Calling `setResizable(true, true)`, `setResizeLimits(...)`, `getConstrainer()->setFixedAspectRatio(...)`, or querying `Desktop::getDisplays().getPrimaryDisplay()` to clamp bounds inside `AudioProcessorEditor` conflicts with JUCE's VST3 wrapper (`IPlugView::onSize` / `checkSizeConstraint`) during Windows `WM_DPICHANGED` events.
   - Splitting painting (`Editor::paint` with manual `g.addTransform(...)`) and child hit-testing (`canvas.setInterceptsMouseClicks(false, true)`) across parent and child components further breaks mouse-to-widget mapping.

---

## 2. Fix Part 1: Build System — Switch from `C:/JUCE` (`9.0.4`) to JUCE `8.0.4`

### For CMake Projects (`CMakeLists.txt`)

Replace `add_subdirectory("C:/JUCE" ...)` with JUCE `8.0.4` resolution (reusing the local `Derive` cache when present so builds stay instant and offline-capable, with `FetchContent` fallback), and set `EDITOR_WANTS_KEYBOARD_FOCUS FALSE` in `juce_add_plugin(...)`:

```cmake
# Use JUCE 8.0.4 (matching Derive) to avoid the JUCE 9.0.4 Windows VST3 multi-monitor DPI hit-test bug.
if(EXISTS "C:/Users/xaris/Derive/build/_deps/juce-src/CMakeLists.txt")
    add_subdirectory("C:/Users/xaris/Derive/build/_deps/juce-src" "${CMAKE_CURRENT_BINARY_DIR}/JUCE" EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(MyPluginTarget
    # ... existing plugin metadata ...
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)
```

Before reconfiguring and building, always clear stale CMake cache files that still point to `C:/JUCE` (`9.0.4`):

```powershell
Remove-Item -Path build\CMakeCache.txt -Force -ErrorAction SilentlyContinue
Remove-Item -Path build\CMakeFiles, build\JUCE -Recurse -Force -ErrorAction SilentlyContinue
& "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -B build -G "Visual Studio 18 2026" -A x64
& "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release
```

---

### For Projucer Projects (`.jucer` — e.g., `Reverberator`, `mzHarmonizer`, `mzOpticalSynthesizer`)

- **Option A (Recommended):** Replace `.jucer` / Visual Studio `.sln` generation with a clean `CMakeLists.txt` using the JUCE `8.0.4` configuration above (`Derive` and `OxidationDeck` pattern).
- **Option B (Keep Projucer):**
  1. In the `.jucer` file, change `pluginEditorRequiresKeys="1"` (if set) to `pluginEditorRequiresKeys="0"`.
  2. Update all `<MODULEPATHS>` / `<MODULEPATH>` entries from `C:/JUCE/modules` to `C:/Users/xaris/Derive/build/_deps/juce-src/modules` (or check out tag `8.0.4` if maintaining a separate `8.0.4` JUCE clone).
  3. Re-save the `.jucer` project with a JUCE `8.0.4` Projucer build (or regenerate/update `JuceLibraryCode/include_juce_*.cpp` and Visual Studio include paths so nothing includes `C:/JUCE/modules` `9.0.4` headers).
  4. Perform a **Clean + Release Rebuild** of the VST3 and Standalone targets.

---

## 3. Fix Part 2: `PluginEditor.h` — Unified `ContentPanel` Architecture

Keep the outer `AudioProcessorEditor` as a thin frame hosting a single `ContentPanel` child component laid out strictly in fixed base logical coordinates (`kBaseWidth` x `kBaseHeight`), paired with an explicit `scaleBox` (`75% / 100% / 125%`).

```cpp
class MyPluginAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit MyPluginAudioProcessorEditor(MyPluginAudioProcessor&);
    ~MyPluginAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    static constexpr int kBaseWidth  = 1220; // replace with plugin's native logical width
    static constexpr int kBaseHeight = 570;  // replace with plugin's native logical height

    struct ContentPanel final : public juce::Component
    {
        explicit ContentPanel(MyPluginAudioProcessorEditor& o) : owner(o) {}
        void paint(juce::Graphics& g) override { owner.paintContent(g); }
        void resized() override { owner.resizedContent(); }
        void mouseDown(const juce::MouseEvent& e) override { owner.mouseDownContent(e); }
        MyPluginAudioProcessorEditor& owner;
    };

    void paintContent(juce::Graphics&);
    void resizedContent();
    void mouseDownContent(const juce::MouseEvent&);
    void applyUiScale(float newScale);

    MyPluginAudioProcessor& processorRef;
    ContentPanel contentPanel { *this };
    float uiScale = 1.0f;
    juce::ComboBox scaleBox;

    // ... plugin controls and attachments ...
};
```

*(Omit `mouseDownContent` / `mouseDown` if your editor has no clickable custom-painted regions on the panel background.)*

---

## 4. Fix Part 3: `PluginEditor.cpp` — Constructor, `resized()`, `paint()`, and `applyUiScale()`

### Checklist

1. Add `addAndMakeVisible(contentPanel);` in the editor constructor.
2. Add **every** child widget (`knobs`, `sliders`, `buttons`, `combos`, sub-panels) to `contentPanel.addAndMakeVisible(...)` — never directly to `AudioProcessorEditor`.
3. Retain `contentPanel`'s default hit-testing (`setInterceptsMouseClicks(true, true)`); **never** call `setInterceptsMouseClicks(false, true)`.
4. **Remove** all calls to `setResizable(...)`, `setResizeLimits(...)`, `getConstrainer()->setFixedAspectRatio(...)`, and `Desktop::getDisplays().getPrimaryDisplay()` size clamping.
5. Wire `scaleBox` (`75%`, `100%`, `125%`) to `applyUiScale(float newScale)`.
6. End the constructor with `setSize(kBaseWidth, kBaseHeight);`.
7. Inside `paintContent(...)`, `resizedContent()`, and `mouseDownContent(...)`, work strictly in unscaled coordinates `(0, 0, kBaseWidth, kBaseHeight)` — **never** call manual `g.addTransform(...)` or multiply/divide `MouseEvent::position` by `uiScale`.
8. Call `contentPanel.repaint()` whenever redrawing panel chrome (e.g., from `timerCallback()` or `parameterChanged()`), instead of calling `repaint()` on the outer editor.

### Code Template

```cpp
// In Constructor:
addAndMakeVisible(contentPanel);

scaleBox.addItem("75%", 1);
scaleBox.addItem("100%", 2);
scaleBox.addItem("125%", 3);
scaleBox.setSelectedId(2, juce::dontSendNotification);
scaleBox.setJustificationType(juce::Justification::centred);
scaleBox.setTooltip("UI Scale Factor");
scaleBox.onChange = [this]() {
    switch (scaleBox.getSelectedId())
    {
        case 1: applyUiScale(0.75f); break;
        case 2: applyUiScale(1.0f);  break;
        case 3: applyUiScale(1.25f); break;
        default: break;
    }
};
contentPanel.addAndMakeVisible(scaleBox);

// ... add all other controls via contentPanel.addAndMakeVisible(...) ...

setSize(kBaseWidth, kBaseHeight);

// Scaling & Layout Methods:
void MyPluginAudioProcessorEditor::applyUiScale(float newScale)
{
    uiScale = newScale;
    setSize(static_cast<int>(std::round(static_cast<float>(kBaseWidth) * uiScale)),
            static_cast<int>(std::round(static_cast<float>(kBaseHeight) * uiScale)));
    resized();
}

void MyPluginAudioProcessorEditor::resized()
{
    contentPanel.setTransform(juce::AffineTransform::scale(uiScale));
    contentPanel.setBounds(0, 0, kBaseWidth, kBaseHeight);
    resizedContent();
}

void MyPluginAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff08060c)); // outer background fill only
}
```

---

## 5. Copy-Paste Prompt for Other Plugin Repositories

Paste this prompt into another plugin project chat (e.g., `Reverberator`, `mzHarmonizer`, `mzOpticalSynthesizer`) to apply the exact fix:

```text
Follow C:\Users\xaris\OxidationDeck\SCALE_FIX.md strictly to fix Windows multi-monitor VST3 mouse hit-testing and UI scaling in this plugin:
1. Switch the build from C:/JUCE (9.0.4) to JUCE 8.0.4 (using C:/Users/xaris/Derive/build/_deps/juce-src when available, or FetchContent GIT_TAG 8.0.4) and set EDITOR_WANTS_KEYBOARD_FOCUS FALSE. If this repo currently uses a .jucer project, either migrate cleanly to CMake with JUCE 8.0.4 or repoint all Projucer module paths to JUCE 8.0.4 with pluginEditorRequiresKeys="0".
2. Refactor PluginEditor.h/.cpp to the Derive ContentPanel + applyUiScale architecture (75% / 100% / 125% scaleBox), removing all setResizable, setResizeLimits, fixed aspect ratio constrainers, getPrimaryDisplay() clamping, and manual Graphics::addTransform / mouse inverse scaling.
3. Clear any stale CMake cache/JUCE build dirs, build Release (VST3 + Standalone), smoke-test Standalone, and report exact build/test outputs.
```
