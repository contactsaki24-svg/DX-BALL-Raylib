# DX-Ball 🎮 (Raylib Edition)

*A classic brick-breaker arcade game, rebuilt in pure C with a dual soundtrack system.*

## Overview

This project is a recreation of the classic **DX-Ball / Breakout** formula,
built in **C using [raylib](https://www.raylib.com/)**. It doubles as a demo
of a lightweight **dual music-mode** system: switch between a calm
"Professional" soundtrack and a nostalgic "Bangla Fun" soundtrack live,
mid-game, with a single keypress.

The entire game lives in one struct/array-based translation unit
(`dxball.c`) — no dynamic allocation, no external game frameworks beyond
raylib itself.

## Key Features

- **Dual Music Mode** — press `M` at any time to swap the background music
  between a Professional/Classical set and a Bangla Nostalgia/Fun set; the
  correct track for the current screen (menu, gameplay, pause, victory,
  game over) is selected automatically in both modes.
- **Dynamic paddle physics** — bounce angle depends on where the ball hits
  the paddle, not just a fixed reflection.
- **Three brick types** — standard (1-hit), multi-hit (2–3 hits, visibly
  lightens as it takes damage), and unbreakable barrier bricks.
- **Two power-ups** — Expand Paddle and Multi-Ball, dropped randomly from
  destroyed bricks.
- **Progressive difficulty** — 3 levels, each adding tougher bricks and more
  barriers.
- **Full HUD** — live score, lives, level counter, pause overlay, and a
  music-mode indicator.
- **Juice** — brick-break particle bursts, rounded paddle, animated menus.

## Controls

| Key | Action |
| --- | --- |
| `←` / `→` | Move paddle |
| `SPACE` | Launch the ball |
| `M` | Toggle Music Mode (Professional ⇄ Bangla Fun) |
| `P` | Pause / Resume |
| `ENTER` | Confirm on menu / game-over / victory / level-clear screens |

## Project Structure

```
DX-Ball-Raylib/
├── .vscode/                     # Pre-wired VS Code build & debug tasks
│   ├── tasks.json
│   ├── launch.json
│   └── c_cpp_properties.json
├── raylib/                      # Bundled raylib (headers + static lib)
│   ├── include/
│   └── lib/
├── assets/
│   └── audio/                   # Music + SFX (see assets/audio/README.md)
├── dxball.c                     # The entire game
├── raylib_template.code-workspace
└── README.md
```

## Build & Run

1. **Open the folder itself** in VS Code (`File > Open Folder…`, select this
   top-level folder) — or double-click `raylib_template.code-workspace`.
   Don't open `dxball.c` on its own; VS Code needs the folder open to find
   `.vscode/tasks.json` and `.vscode/launch.json`.
2. If prompted to install the recommended extension, click **Install** (this
   gets you the C/C++ extension).
3. Open `dxball.c` in the editor so it's the active file.
4. **Build:** `Ctrl+Shift+B` (Windows/Linux) or `Cmd+Shift+B` (Mac) → *Run
   Build Task*. This compiles `dxball.c` against the bundled raylib and
   produces `dxball.exe` in the project root.
5. **Run with the debugger:** open the Run and Debug panel
   (`Ctrl+Shift+D` / `Cmd+Shift+D`) and press the green ▶ (or `F5`).
   Breakpoints work here.

> The bundled `.vscode` config and `raylib/lib/libraylib.a` in this template
> target **Windows** (MinGW/gdb toolchain, `-lopengl32 -lgdi32 -lwinmm`). If
> you're building on macOS/Linux, swap in the matching raylib static library
> for your platform and adjust `tasks.json`'s linker flags accordingly.

## Architecture

Single file, organized into four sections:

| Module | Covers | Approx. lines |
| --- | --- | --- |
| Constants, types & level data | Structs (`Paddle`, `Ball`, `Brick`, `PowerUp`, `Particle`), enums, tunables | ~110 |
| Physics & game logic | Paddle/ball movement, collision & reflection, level building, power-up & particle logic | ~255 |
| Rendering & HUD | Bricks, paddle, balls, power-ups, particles, score/lives/level/mode display | ~75 |
| Audio & main loop | Dual-music track selection, state machine, input handling | ~210 |

`dxball.c` comes to **~660 lines** including comments and section banners —
more than a bare-minimum estimate, but every feature in the brief (multi-ball,
particles, a full menu/pause/game-over/victory state machine, and the
mode-aware music selector) is implemented in full rather than stubbed.

## Credits & Licensing Note

Gameplay code, procedural level layouts, and the placeholder audio in
`assets/audio/` were written for this project. The placeholder tracks are
**not** the commercial songs referenced during planning — see
`assets/audio/README.md` before using this repo in a public presentation or
pushing it to a public GitHub repo.
