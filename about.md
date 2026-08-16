# Frame Window Counter

A utility mod for Geometry Dash designed to track frame windows, spawn custom visual and audio markers during gameplay, and calculate NaNDL Precision values for macros.

---

## Features

* **Custom Markers & HUD:** Display custom text, colors, and audio effects on specific frames, with a live HUD counter for configured frame window ranges.
* **Macro Integration:** Import `.fwc`, `.gdr`, `.gdr2`, `.json` (NaNDL), or `.slc` (Silicate) macro files. Export setups as `.fwc` or NaNDL `.json`.

---

## How to Use

### 1. Accessing the Menu
* Click the time icon button on the right side of the Pause Menu.
* **Windows Hotkey:** Press **`O`** on your keyboard to quickly toggle the editor anywhere.

### 2. Label & Window Settings
1. Open **Labels** or **Windows** settings from the bottom menu of the editor.
2. Define **Min/Max Window** bounds, **HUD Text**, **Colors**, and optional **Audio Paths** (e.g., `.ogg` or `.mp3` files).
3. Check **Show in HUD** to display a live counter on the top-left of your screen during gameplay.

### 3. Frame Editor & Macros
1. Click **Import** to load a macro file (`.fwc`, `.gdr`, `.gdr2`, `.json`, `.slc`).
2. Adjust individual frame windows or toggle **1P/2P** modes without disturbing list order.
3. Toggle individual frames or use **Activate All** / **Inactivate All** for bulk edits.
4. Click **Export** to save your setups.

### 4. Precision Display (L*)
1. Click the **Options** icon in the top-right of the Frame Editor to open Precision Display settings.
2. Toggle any of the available formula modifiers:
   * **Base**
   * **Nerve**
   * **Fatigue**
   * **CPS**
   * **Combinations:** N+F, N+C, F+C, and All (N+F+C).
3. Calculations run asynchronously in the background. While processing, a progress percentage will be shown.
4. Once completed, click the top status button in the editor to cycle through final $L^*$ metrics, or view live values in the bottom-left corner during gameplay.

---

## Notes
* Automatic background backups are periodically created in your Geode `Autosaves` directory.