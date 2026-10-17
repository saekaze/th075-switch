# Touhou 7.5: Immaterial and Missing Power — Nintendo Switch Port
*(東方萃夢想　～ Immaterial and Missing Power.)*

![Platform](https://img.shields.io/badge/Platform-Nintendo%20Switch-e60012?style=for-the-badge&logo=nintendoswitch&logoColor=white)
![Status](https://img.shields.io/badge/Status-Fully%20Playable-brightgreen?style=for-the-badge)
![License](https://img.shields.io/badge/Port%20Code-CC0%201.0-blue?style=for-the-badge)

A native homebrew port of Twilight Frontier and Team Shanghai Alice's 2004 fighting game **Touhou 7.5: Immaterial and Missing Power** for the **Nintendo Switch** (Horizon OS).

This build runs the C++ reimplementation by [SteinsGateON](https://space.bilibili.com/34714121), published at [Goan114/th075](https://github.com/Goan114/th075), compiled **natively for ARM64** (no wasm, no emulation) and driven by an SDL2 host on an OpenGL ES 3 context — no Linux, Box64 or Wine involved.

Companion to the [Touhou 6](https://github.com/saekaze/th06-switch), [7](https://github.com/saekaze/th07-switch), [8](https://github.com/saekaze/th08-switch), [9](https://github.com/saekaze/th09-switch), [10](https://github.com/saekaze/th10-switch), [11](https://github.com/saekaze/th11-switch) and [15](https://github.com/saekaze/th15-switch) Switch ports, with the same `touhou075.nro` + `sd:/switch/touhou/touhou075/` layout.

---

## 🆕 First release (1.11-r1)

* 🥊 The full game: Story (ten characters to their endings), Arcade, Versus (player or CPU), Training, Replays, Music Room, results and the card list.
* 🎮 **Local 2-player:** the Joy-Cons are player 1, a Pro Controller (or any other controller) is player 2.
* 🔊 BGM streamed straight out of your `th075bgm.dat` with the original loop points.
* 📁 Recommended folder: `sd:/switch/touhou/touhou075/`, next to the other Touhou ports.

---

## ✨ Key Features

* ⬛ **OLED-Friendly Pillarboxing:** the original 640×480 picture, drawn at 1280×960, is centred with pure black (`#000000`) bars and an aspect-correct upscale.
* 📦 **Reads Your Original Files:** `th075.dat`, the 1.11 patch's `th075b.dat` and `th075bgm.dat` are read in place — nothing is unpacked or converted.
* 🔊 **Full Audio:** sound effects, character voices and BGM (original 16-bit PCM and loop points).
* 🈂️ **Japanese Text Without Windows:** dialogue and menu text is drawn with FreeType from `msgothic.ttc` (or the Switch's built-in Japanese font), one character the first time it appears — **no slow first launch**.
* 🎮 **Two Players:** Joy-Cons and a Pro Controller, each with the same layout.
* 🌏 **Language-Aware Title:** hbmenu shows `東方萃夢想　～ Immaterial and Missing Power.` on consoles set to 日本語 and the romanised title everywhere else.
* 💾 **Saves Next to the Data:** `score.dat`, `config.dat`, replays (`replay/replay000.rep` …) and snapshots are written into the SD folder the game loaded from.

---

## 📥 Installation Guide

> ⚠️ **Disclaimer:** This repository contains **ONLY the homebrew engine code**. No game assets are distributed. You must legally own a copy of *Touhou 7.5: Immaterial and Missing Power* updated to **ver1.11** (Japanese).

### 1. SD Card File Structure

1. Ensure your Nintendo Switch is running custom firmware (Atmosphère CFW).
2. Download the latest `touhou075.nro` from the [Releases](../../releases) tab (or build from source).
3. Create the folder `sd:/switch/touhou/touhou075/` and copy the following into it:

```text
sd:/switch/touhou/touhou075/
    ├── touhou075.nro         # Nintendo Switch homebrew executable
    ├── th075.dat             # Main game archive
    ├── th075b.dat            # ver1.11 patch archive
    ├── th075bgm.dat          # Background music archive
    └── msgothic.ttc          # MS Gothic (C:\Windows\Fonts) - recommended
```

`th075bgm.dat` is optional (the game runs without music). Without `msgothic.ttc` the console's built-in Japanese font is used. The Chinese fan edition (`th075c.exe` / `th075c.dat`) is not supported; use the Japanese files.

**Recommended place: `sd:/switch/touhou/touhou075/`.** Other places still work: the port first looks in its own folder (wherever the NRO is), then for a `th075` / `touhou075` folder (any capitalisation) directly on the SD card, in `switch/`, `touhou/`, `switch/touhou/`, `games/` or `roms/`.

### 2. Launching

Run `touhou075.nro` from the **Homebrew Menu (hbmenu)**, **Sphaira launcher**, or a home screen forwarder. **Exit** on the title screen returns to hbmenu. If something is missing, the port shows what and where it looked, and `th075-switch.log` is written next to the saves.

---

## 🕹 Controls

| Nintendo Switch Button | Action |
| :--- | :--- |
| **Left Stick / D-Pad** | Move |
| **B** | A — weak attack / Confirm |
| **A** | B — strong attack / Cancel |
| **Y** or **L / ZL** | C — spell card |
| **X** or **R / ZR** | D — dash / graze |
| **+ (Plus)** | Pause (that player's start button) |
| **− (Minus)** | Snapshot (saved to `snapshot/` next to the game data) |

**Laid out like every Touhou Switch port:** B is the main button and confirms, A cancels.

**Players:** the Joy-Cons are player 1 — handheld, in a grip or both detached (a pair of Joy-Cons is always one player). Any other controller, such as a Pro Controller, is player 2. With no Joy-Cons connected, the first Pro Controller is player 1 and a second one player 2. Player 1 plays your side; in Versus, player 2 plays the other side.

---

## 🛠 Building from Source

### Automated Build (GitHub Actions)

`.github/workflows/build-switch.yml` compiles `touhou075.nro` inside the official `devkitpro/devkita64` container and uploads it as an artifact; tagging `v*` (or running the workflow with a `release_tag`) also publishes it as a release asset. A second job builds the same sources for Linux and runs the host tests.

### Local Build (Linux / macOS / WSL)

1. Install [devkitPro](https://devkitpro.org/wiki/Getting_Started) with `devkitA64` and `libnx`.
2. Install the required Switch portlibs:

   ```bash
   sudo dkp-pacman -Syu
   sudo dkp-pacman -S switch-dev switch-mesa switch-libdrm_nouveau switch-sdl2 switch-freetype switch-libpng switch-bzip2 switch-zlib switch-harfbuzz
   ```

3. Build:

   ```bash
   ./scripts/build_switch.sh
   ```

   The result is `build-switch/touhou075.nro`.

Desktop Linux test build and host tests (same sources; needs `libsdl2-dev libfreetype-dev libgles-dev`, and `xvfb` + a Japanese font such as `fonts-ipafont-gothic` for the full test set):

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build
xvfb-run -a ./build/th075_host_tests
./build/touhou075 /path/to/folder/with/th075.dat
```

---

## 📂 How It Works

The game logic is the architecture-independent C++ from upstream (`game/`), compiled natively for AArch64 with the upstream's own code-generation contract (`-ffp-contract=off -fno-strict-aliasing -fno-exceptions -fno-rtti`, plus `-fsigned-char` so `char` behaves as on the original x86). It is already 64-bit clean; the host in `src/` is upstream's SDL host with the browser parts replaced — see [`upstream/UPSTREAM.md`](upstream/UPSTREAM.md) and the exact [`switch-port.patch`](upstream/switch-port.patch).

| File | Purpose |
| :--- | :--- |
| `game/` | upstream TH075 game logic (fighters, scripts, collisions, story, menus, replays, …) |
| `portable/sdl/Renderer.*` | upstream semantic GLES renderer, ported SDL3/WebGL2 → SDL2/GLES 3 (same port as th15-switch) |
| `src/Application*.cpp` | upstream host: scenes, battle loading, story, saves, replays |
| `src/main.cpp` | Switch main loop and two-player controls: Joy-Cons = P1, other controller = P2 (replaces upstream `BrowserRuntime.cpp`) |
| `src/FileResources.*` | `th075.dat` / `th075b.dat` read in place (upstream fetched pre-extracted files) |
| `src/AudioDevice.*` / `src/BgmStream.*` | miniaudio mixer; BGM streamed from `th075bgm.dat` with the original loop points |
| `src/FontDevice.*` | text glyphs with FreeType on first use (upstream shipped pre-baked GDI glyphs) |
| `src/GraphicsDevice.*` / `src/TextureBank.hpp` | original images → renderer surfaces |
| `src/Platform.*` / `src/SdlCompat.hpp` | SD paths, files, log; the SDL3 calls of the upstream host on SDL2 |
| `platform/switch/icon.jpg` | 256×256 NRO icon (the original game cover) |
| `scripts/build_switch.sh` | one-shot Switch build |
| `scripts/nacp_lang.py` | Japanese NACP title slot |
| `scripts/gen_cp932.py` | CP932 → Unicode table for the glyph renderer |
| `tests/test_host.cpp` | host tests for the port layer |

The game ticks once per vsync (60 Hz); after a real stall it catches up by at most four ticks, like upstream's frame cadence. The CPU is set to the same 1785 MHz boost the other native ports use.

---

## 🤝 Credits & Acknowledgments

* **Twilight Frontier / ZUN / Team Shanghai Alice** — original creators of Immaterial and Missing Power.
* **[SteinsGateON](https://space.bilibili.com/34714121)** — the TH075 C++ reimplementation this port is built on, published by **[Goan114](https://github.com/Goan114/th075)**.
* **[N0zoM1z0](https://github.com/N0zoM1z0)** — the TH075 reverse-engineering reference the reimplementation names and checks its functions against.
* **Switchbrew & devkitPro Team** — the open-source `libnx` SDK and Switch toolchain.

**Licensing:** the Switch port code (`src/main.cpp`, `src/Platform.*`, `src/SdlCompat.hpp`, `src/BgmStream.*`, `src/MiniaudioConfig.hpp`, `scripts/`, `tests/`, build files) is CC0 1.0 (see `LICENSE`). Everything else is upstream code by SteinsGateON under the **MIT** licence — see [`upstream/LICENSE-upstream-MIT.txt`](upstream/LICENSE-upstream-MIT.txt) and [`upstream/THIRD-PARTY-NOTICES.txt`](upstream/THIRD-PARTY-NOTICES.txt). Bundled third-party headers keep their own licences (`portable/sdl/third_party/`).
