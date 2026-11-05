# Fixes for UQM-MegaMod

This file lists bugs in the UQM-MegaMod code that this project found and fixed. They can go back to [UQM-MegaMod](https://github.com/JHGuitarFreak/UQM-MegaMod).

The list contains only bugs that are in MegaMod at the fork commit `18e1f3d1`. It does not contain bugs that this project caused and then fixed. The MegaMod locations are the C files of MegaMod. The fixes here are in C++ and use different types, so each fix must be written again for the C code.

"Checked" means that the bug is still in the MegaMod `master` branch of November 2026. The other bugs were not checked against `master`.

## Memory leaks

### Conversation string tables do not free their name entries

- **MegaMod:** `FreeStringTable` in `src/libs/strings/strings.c`. Checked.
- **Problem:** a conversation string table keeps a name index (`nameIndex`), but `FreeStringTable` frees only the string data, the string array and the table.
- **Fix here:** `11976d04`.

### String table resources free only the table

- **MegaMod:** `FreeResourceData` in `src/libs/resource/getres.c`, used for the string table resource types in `src/libs/strings/sresins.c`.
- **Problem:** the string table resources are released with `FreeResourceData`, which frees the table structure but not its entries.
- **Fix here:** `11232794`. The string table resources are released with `FreeStringTable`.

### The conversation loader leaks its name pool

- **MegaMod:** `_GetConversationData` in `src/libs/strings/getstr.c`.
- **Problem:** the buffer that holds the phrase names is not freed.
- **Fix here:** `11232794`.

### The palette of an indexed frame copy leaks

- **MegaMod:** `TFB_DrawCanvas_ExtractPalette` in `src/libs/graphics/sdl/canvas.c` and its caller in `src/libs/graphics/drawable.c`.
- **Problem:** the palette that is extracted for a copy of an indexed frame is not freed.
- **Fix here:** `fba825db`.

### Unused subtitle pages leak

- **MegaMod:** the subtitle handling in `src/libs/sound/trackplayer.c`.
- **Problem:** subtitle pages that are never shown are not freed.
- **Fix here:** `98cb8a7a`.

### A video that does not load leaks its reference

- **MegaMod:** `PlayLegacyVideo` in `src/libs/video/legacyplayer.c`.
- **Problem:** when the video does not load, the function returns without freeing the reference that it allocated.
- **Fix here:** `e6fba89f`.

### The Lua string interpolation leaks a buffer for each expression

- **MegaMod:** `luaUqm_comm_stringInterpolate` in `src/uqm/lua/luacomm.c`. Checked.
- **Problem:** the function allocates a buffer with `HMalloc` for each `<% %>` expression, and the buffer is not freed.
- **Fix here:** `79aaf8e9`.

### Each loaded game leaks the race queue

- **MegaMod:** `LoadFleetInfo` in `src/uqm/globdata.c`, called when a game loads (`src/uqm/load.c`).
- **Problem:** `LoadFleetInfo` makes the race queue again without freeing the old table.
- **Fix here:** `9c96630d`.

## Data races

### The input key counters and key bindings

- **MegaMod:** `ImmediateInputState` (`src/uqm/controls.h`, `src/uqm/gameinp.c`) and the VControl binding tables (`src/libs/input/sdl/vcontrol.c`).
- **Problem:** the main thread changes the key counters while the game thread copies and resets them, and the main thread walks the binding tables while the game thread can rebind keys. Nothing synchronizes these accesses.
- **Fix here:** `efb23ad4`. The counters are atomic and use a compare-exchange loop, and a recursive mutex guards the binding tables.

### The debug hook

- **MegaMod:** `debugHook` in `src/uqm/starcon.c`. Checked.
- **Problem:** the game thread reads `debugHook` and then sets it to `NULL` in a separate step, while another thread can set it.
- **Fix here:** `739b8349`. The hook is taken and cleared in one atomic exchange.

## HD content at SD scale

### A subtitle word wider than the text box hangs the game

- **MegaMod:** `getLineWithinWidth` in `src/uqm/comm.c`.
- **Problem:** if the first word of a line is wider than the text box, the line stays empty and the text loop does not advance.
- **Fix here:** `783cf302`. A first word that is too wide stays on its line.

### The oscilloscope buffer overflows

- **MegaMod:** `scope_data[128]` in `src/uqm/oscill.c`. Checked.
- **Problem:** the buffer has a fixed size of 128 bytes, but the scope frame can be wider.
- **Fix here:** `783cf302`. The buffer has the width of the scope frame.

## Other

### The music file name hash sign-extends bytes

- **MegaMod:** `crc32b` in `src/libs/math/sqrt.c`.
- **Problem:** the loop sign-extends bytes above 127, so file names that are not ASCII get a different hash from the standard CRC-32. ASCII names are not affected.
- **Fix here:** `3a518697`. The hash uses the CRC-32 of zlib.
