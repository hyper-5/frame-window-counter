# Frame Window Counter

A comprehensive frame window tracking, HUD customization, and $L^*$ (NaNDL Precision) calculation toolkit for **Geometry Dash** (Geode).

## Features

### Frame Window Tracking & Visuals
* **In-Game Markers:** Spawns animated markers on player positions during gameplay corresponding to specific frame actions and window tolerances.
* **Live HUD Statistics:** Customizable HUD overlay in the top-left corner tracking action counts across configured frame window ranges.
* **Custom Audio Effects:** Bind dedicated sound effects (`.ogg`, `.mp3`) to specific frame window presets.
* **Camera Compatibility:** Full world-to-screen matrix transformations ensuring markers properly track through rotations, zoom, and reversed gravity.

### Advanced Precision Calculator ($L^*$)
* **Multi-Dimensional Metrics:** Solves for theoretical level precision across 8 dimension combinations:
  * **Base** ($L^*$)
  * **Nerve** ($K_T$) — Time-dependent decay
  * **Fatigue** ($K_U$) — Input count-dependent decay
  * **CPS** ($K_C$) — High click-frequency penalty
  * **Composites:** N+F, N+C, F+C, and All (N+F+C)
* **Multi-Threaded Asynchronous Solver:** Fast root-finding algorithm using an error function (erfc) LUT lookup table.
* **Configurable Calculation Parameters:**
  * **Level TPS:** Target calculation rate (synced with macro FPS).
  * **Respawn Time (s):** Accounts for penalty time from death back to start.
  * **Target Solve Time (s):** Expected completion time benchmark (default: 86,400s / 24 hours).
  * **Model Constants:** Fine-tune $K_T$, $K_U$, and $K_C$ up to 16 decimal places with one-click default restore.
* **Dirty State Indication:** Automatically highlights $L^*$ values in red when frames or settings are modified without recalculating.

### Action Editor & Macro Tooling
* **Timeline Tracking:** Automatically highlights and scrolls to the active frame in real-time as the level plays.
* **Dual Player (1P/2P) Control:** Toggle individual action player assignments on the fly.
* **Import & Export:**
  * Import replay/macro files (`.gdr`, `.gdr2`, `.slc`, `.json`, `.fwc`).
  * Export custom binary `.fwc` and NaNDL `.json` project files.
* **Automated Safety Backups:** Background autosaves generated periodically to the Geode config directory.

---

## Project Structure

```text
src/
├── Audio/                          # Audio management & playback engine
│   ├── SoundManager.cpp
│   └── SoundManager.hpp
├── Data/                           # Global state, cache & type definitions
│   ├── State.cpp
│   ├── State.hpp
│   └── Types.hpp
├── Hook/                           # Geode game hooks & lifecycle modifications
│   ├── CCDirectorHook.cpp          # Scene loop, autosave & hotkey handler
│   ├── PauseLayerHook.cpp          # Pause menu UI integration
│   └── PlayLayerHook.cpp           # HUD rendering, marker spawning & game ticks
├── IO/                             # File importing/exporting logic (.fwc, .gdr)
│   ├── FileIO.cpp
│   └── FileIO.hpp
├── Math/                           # L* mathematical models & multi-threaded solver
│   ├── Calculator.cpp
│   └── Calculator.hpp
├── UI/                             # Custom Geode popups and editors
│   ├── AddFramePopup.cpp / .hpp
│   ├── FrameActionPopup.cpp / .hpp
│   ├── LabelPresetPopup.cpp / .hpp
│   ├── LStarCalcSettingsPopup.cpp / .hpp
│   ├── PrecisionSettingsPopup.cpp / .hpp
│   └── WindowPresetPopup.cpp / .hpp
└── Common.hpp                      # Global constant definitions & math helpers
```

---

## Hotkeys & Controls

| Input | Location | Action |
| :--- | :--- | :--- |
| **`O`** | Anywhere in-game (Windows) | Toggle Frame Action Editor & close active submenus |
| **Time Icon** | Pause Menu (Right side) | Open Frame Action Editor |

---

## How to Use

### 1. Frame Window Presets & Labels
1. Open the **Windows** or **Labels** popup from the bottom bar of the main editor.
2. Configure **Min/Max Frame Windows**, custom HUD label text, colors, and audio file triggers.
3. Enable **Show in HUD** to display a live tally during gameplay.

### 2. Importing & Editing Frames
1. Click **Import** in the main editor to load a macro or replay file (`.gdr`, `.gdr2`, `.slc`, `.json`, `.fwc`).
2. Edit frame numbers, frame window tolerances, or switch between **1P** and **2P** modes.
3. Use **Activate All** / **Inactivate All** to bulk toggle drawing and HUD inclusion.

### 3. Calculating Precision ($L^*$)
1. Click the **Options** gear icon in the top-right of the Frame Editor to open the **Precision Display** popup.
2. Toggle your desired metrics (Base, Nerve, Fatigue, CPS, etc.).
3. *(Optional)* Click the top-right gear in the Precision Display to open **L* Calc Settings** to configure TPS, respawn time, target solve time, or model constants.
4. Click **Start Calculation**. Once finished, cycle through metrics on the top status button or view live values in the bottom-left corner during level runs.

---

## Building from Source

### Prerequisites
* [Geode CLI](https://github.com/geode-sdk/cli) installed and configured.
* **Clang** compiler with C++20 support (LLVM / Clang-cl).
* CMake 3.27+.