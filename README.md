# Kindle Tetris

Native Tetris for jailbroken Kindles (built and aimed at the Oasis 3 / KOA3, ARMv7 soft-float, static).
Launched as a scriptlet from the library. Touch buttons plus the page-turn buttons.

## Install (one line, in KTerm, Wi-Fi on)

```sh
curl -4fsSL https://raw.githubusercontent.com/awiqu/kindle-app/master/install.sh | sh
```

Then open **Tetris** in the library (the scriptlet can take a few seconds to appear).

Remove:

```sh
curl -4fsSL https://raw.githubusercontent.com/awiqu/kindle-app/master/install.sh | sh -s uninstall
```

Without Wi-Fi: copy `dist/tetris`, `launcher/run.sh` to `/mnt/us/extensions/tetris/` and
`launcher/Tetris.sh` to `/mnt/us/documents/` over USB.

## Controls

| Control | Action |
| --- | --- |
| `<` `>` | move |
| `ROT` / forward page button | rotate |
| `DOWN` | soft drop (+1 point per row) |
| `DROP` / back page button | hard drop (+2 points per row) |
| `PAUSE` | pause / resume |
| `QUIT` | tap twice within 3 s |

First launch asks for three taps on black squares to learn the touch orientation
(saved in `/mnt/us/extensions/tetris/calib.conf`). Taps are wrong later? Run
`sh /mnt/us/extensions/tetris/run.sh --calibrate` in KTerm, or delete `calib.conf`.

## How it works

`Tetris.sh` (scriptlet) starts `run.sh` in a detached session. `run.sh` stops the Kindle UI
(`stop lab126_gui`), runs `tetris`, then restarts the UI (also on crash or signal).
Log: `/mnt/us/extensions/tetris/tetris.log`. High score: `highscore` next to it.
If the screen ever stays blank: `start lab126_gui`, or hold power to reboot.

The game draws with [FBInk](https://github.com/NiLuJe/FBInk) (statically linked), with fast
partial refreshes and a full flashing refresh every few pieces and on line clears.

## Build and test

```sh
./build.sh                                  # needs python3, git, make; output in dist/
gcc -std=c11 -Wall -Wextra -o /tmp/test_core test/test_core.c && /tmp/test_core
gcc -std=gnu11 -o /tmp/render_test test/render_test.c && /tmp/render_test   # writes PGM screenshots
```

`dist/tetris` and `dist/SHA256SUMS` are committed; the installer checks the hash.
Rebuild and commit both after changing `src/`.
