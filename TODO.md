# TODO

## General

- Add unit tests for new features.
- Add imgui.

## Cleanup of src/libs

- **UTF-8 paths:** paths on Windows use the ANSI code page in `libs/file/paths.cpp` and `libs/io`. Use UTF-8 paths in both.
- **Raw pointers in C structs:** some C structs own raw pointers made with `new`. Make them RAII when the struct is no longer cleared with `memset`:
  - `PLANET_ORBIT` buffers (`SOLARSYS_STATE` is cleared with `memset`, and `PLANET_DESC` holds an orbit by value).
  - The queue tables in `displist.h`.
  - Font character data, which the draw thread deletes.
  - String tables, color maps and sound banks that are freed with `delete`:
    - `race_strings` in `SHIP_INFO`, `FLEET_INFO` and the other ship structs. These structs are queue elements without constructors, and `RACE_DESC` is copied by value.
    - `ship_sounds` in `DATA_STUFF`.
    - `AlienColorMap` and `ConversationPhrases` in `LOCDATA`. `CommData` is copied from the race templates.
    - `HangarColorMap` in `MENU_STATE`, which is cleared with `memset`.
    - `sphereMap` in `PLANET_ORBIT`, which is in the `PLANET_DESC` arrays.
  - Global and static sound banks and color maps (`MenuSounds`, `GameSounds`, `LanderSounds`, `hypercmaps`, `SunCMap` and others). They can become RAII only after `exit()` no longer runs while the game thread runs (see "Exit code 127"). If they become RAII before that, their destructors can run during `exit()`, after the audio system has shut down.

Keep these, because they are large and in use: `graphics`, `sound`, `video`, `input`, `io`, `resource`, `strings`, `luauqm`, `network` (`NETPLAY` is on), `callback` (netplay and `uqm/sis.cpp` use the alarms and async calls), and `math`. Do not change the RNG: seeded games must give the same results.

## Graphics

- **MSL shaders:** the MSL output of `shaders.bat` is not tested yet.
- **Shader header check:** add a check in the build that the shader headers match the `.hlsl` files.
- **GPU backend setting:** add a setting for the GPU backend. Now only the `SDL_GPU_DRIVER` environment variable selects it.
- **Gamma slider without the GPU shaders:** when `TFB_SetGamma` fails (CPU path, `--renderer` other than `gpu`, or GPU Scaling off), `gamma_HandleEventSlider` treats the failure as the end of the range. Each key press moves the slider to its minimum or maximum, and that value (for example 0.4) is saved. Disable or hide the slider when gamma correction is not available.

## Stability

- **Exit code 127:** with the settings.json of one developer, the game exits with code 127 after the input script quits (`ProcessUtilityKeys` calls `exit(EXIT_SUCCESS)` while the game thread runs). The default settings exit with 0. The graphics section alone exits with 0, and the full settings with `resolutionFactor` 0 still exit with 127, so the cause is in a non-graphics section. Not bisected further. Also check whether `TFB_PreQuit` (it releases the renderer before `SDL_Quit`) is involved, because the game thread can still draw at that time.
- **Crash:** an intermittent crash happens in about 1 of 15–50 scripted runs, mostly `setup-hd`. The cause is unknown, and the crash happened before the io changes.
- **Threads:**
  - `TFB_SoundSource::sample` has no `UQM_GUARDED_BY`, because the sound effect channels use it without the stream lock.
  - The mixer lock order (`src_mutex`, `buf_mutex`, `act_mutex`) is only a comment. Clang can check it with `UQM_ACQUIRED_AFTER` annotations and `-Wthread-safety-beta`.
- **Busy waits:** the `TaskSwitch()` loops (input rebinding, init, starcon, util, battle and others) sleep for 1 ms in each pass. Change them to waits on a condition.

## Testing

- **Gameplay recording:** it was reverted in `28da13c9`. Do it again later with a different design.

## Content data

Problems that the conversions to JSON found in the content repository.

Open:

- **Missing images:** these images do not exist. They were the last frames of their animations, so the game did not load them, and the converter removed the frames. Make sure that no code uses these frame indexes:
  - `addons/mm-hd/comm/vux/vux.ani.json`: `vux-103.png` to `vux-110.png` (8 frames).
  - `base/cutscene/spins/accessdata.ani.json` and `addons/mm-hd/cutscene/spins/accessdata.ani.json`: `accessdata-008.png` and `accessdata-009.png`.
  - `addons/mm-hd/ui/custom-border/bevel/custom-bevel.ani.json`: `bevel-067.png`.
  - `addons/3do-mode-sd/ui/playmenu.ani.json`: `playmenu-070.png`.
- **Missing animation files:** these GFXRES resources name files that do not exist:
  - `comm.chmmr.red.graphics` in `mm-hd.rmp.json`: `addons/mm-hd/comm/chmmr/chmmr-red.ani.json`.
  - `graphics.flagshipskeleton.red` in `uqm.rmp.json` and `mm-hd.rmp.json`: `fs_skeleton-red.ani.json`. Only `fs_skeleton-red.png` exists.
- **Missing script:** `comm.robot.script` names `base/comm/robot/robot.lua`, which does not exist.
- **Missing resource:** the starbase conversation asks for `comm.commander.lowpower.music`, which no resource map defines.
- **Missing credits music:** `music.credits` in `uqm.rmp.json` names `base/cutscene/credits/credits.ogg`, which content commit `7eebe3d10` removed. The credits play without music unless the volasaurus remix pack is loaded.
- **Removed comments:** JSON has no comments, so the conversions removed the commented-out entries: about 80 joystick bindings in `menu.key` and `uqm.key`, and 4 entries in the `.rmp` files. They are in the git history of the content repository.
- **HD pkunk font:** look at a Pkunk conversation in HD to make sure that the repaired font metrics (see below) look correct.

Repaired:

- **HD pkunk font metrics:** `addons/mm-hd/fonts/pkunk.fon/kerndat.fnt` had only `pkunk.fon 44`, so the game ignored it. The new `kerndat.fnt.json` has the SD values multiplied by 4 (`44 4 4 24`, as for the slylandro font, which has the same SD values) and the SD kerning.
- **DRAW SIS position:** from commit `130757f5` on, the slideshow code skipped the first number of `DRAW SIS x y scale angle`, so the ending drew the flagship at the wrong place and without its rotation. The JSON slideshows have the numbers as the original game used them. Look at the ending to make sure that it looks correct.
- **Kern value out of range:** `base/fonts/micro.thin.fon` had a kern value of 4 for `,`. The game clamped it to 3, and the JSON file has 3.

