# Changelog — Touhou 7.5 Switch port

## Touhou 7.5: Immaterial and Missing Power — Switch Port r1

First release (display version `1.11-r1`).

* The full game on SteinsGateON's C++ reimplementation (ver1.11), compiled natively for ARM64.
* Reads the original `th075.dat`, `th075b.dat` and `th075bgm.dat` in place; BGM streamed with the original loop points.
* Japanese text drawn with FreeType from `msgothic.ttc` (or the Switch's font) on first use — no long first launch.
* Two players: the Joy-Cons (handheld, grip or detached pair) are player 1, any other controller such as a Pro Controller is player 2; with no Joy-Cons, the first Pro Controller is player 1.
