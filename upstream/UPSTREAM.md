# Upstream

`game/`, `src/` (except the files listed below) and `portable/` come from
**[Goan114/th075](https://github.com/Goan114/th075)** — the TH075 (東方萃夢想
ver1.11) reimplementation by [@SteinsGateON](https://space.bilibili.com/34714121),
MIT-licensed (`LICENSE-upstream-MIT.txt`), published on their behalf — at commit
`7c6127c7c71f89c8a790ba692d465bff75891096` ("Import TH075 1.0.4 portable
source", 2026-10-15):

| Here | Upstream |
| :--- | :--- |
| `game/` | `th075_web/cpp/game/` |
| `src/` | `th075_web/cpp/sdl/` (host: scenes, battle loading, story, saves, replays, audio, text) |
| `portable/sdl/`, `portable/input/` | `portable/sdl/`, `portable/input/` (renderer, shaders, miniaudio / stb headers, keyboard map) |

Not taken: `th075_web/cpp/sdl/BrowserRuntime.cpp` (replaced by `src/main.cpp`),
`GraphicsProbe.cpp` (browser test harness), the launcher, the asset preparation
scripts and the `th08_web`…`th15_web` helper folders.

Switch port files (new): `src/main.cpp`, `src/Platform.*`, `src/SdlCompat.hpp`,
`src/BgmStream.*`, `src/MiniaudioConfig.hpp`, `scripts/`, `tests/`,
`CMakeLists.txt`, `.github/`.

Changes to upstream files:

* `game/` — in [`switch-port.patch`](switch-port.patch): `GlyphBank` gets a
  `source` callback used when no baked GDI glyph bank is loaded, so the host
  can render glyphs on demand; the `../../../portable/` include path became
  `../portable/`. The game code is otherwise untouched; it is already 64-bit
  clean (all 189 files compile for AArch64 without changes).
* `portable/sdl/` — in [`switch-port.patch`](switch-port.patch): the same
  SDL3/WebGL2 → SDL2/GLES 3 (Switch Mesa) port as th15-switch (upstream's
  renderer is byte-identical to TH15's apart from the new `LuminanceMultiply`
  combiner, kept): window and context, `EXT_clip_control` through the GLES
  loader, aspect-correct pillarboxed present, `packed` (a reserved GLSL ES word
  Mesa rejects) renamed.
* `src/` host files (edited in place, marked "Switch port"):
  - `FileResources` reads the original `th075.dat` / `th075b.dat` in place with
    upstream's `DataArchive` index reader, instead of fetching pre-extracted,
    zlib-packed files; `/save/...` paths map to the SD save folder.
  - `AudioDevice` uses SDL2 queued audio and streams music from the original
    `th075bgm.dat` WAVs, with the loop points upstream's `prepare-music.py`
    reads from each WAV (cue = loop start, adtl/ltxt = loop length, the fixed
    loop for cue 59, no loop for the ending theme), instead of OGG files and
    `catalog.bin`.
  - `FontDevice` renders MS Gothic glyphs with FreeType on first use (GGO_GRAY4
    levels, cell-height sizing like GDI's `CreateFont`), instead of the
    pre-baked `original-glyphs.bin`; a baked bank in the data folder still wins.
  - `GraphicsDevice` drops the browser canvas hints and window sizing;
    `Application` opens the archives first and makes the save folders;
    `toggle_fullscreen` does nothing (always full screen).

Line endings were normalised to LF.

When upstream updates: copy the new `th075_web/cpp/game` and `portable/` over
these folders, re-apply `switch-port.patch`, and merge `th075_web/cpp/sdl`
changes into `src/` by hand.
