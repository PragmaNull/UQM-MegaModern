"""Convert the slideshow (cutscene) files of a content directory from string tables to typed commands.

In the string table of a slideshow, each string is a command line, such as
"DIMS 320 240" or "DRAW 3 10 20", and the id of the string is a comment. The
script changes the files that the "slides.*" resources name, and the files
that their CALL commands name, to:

    {
      "version": 1,
      "commands": [
        {"comment": "Clear entire screen first; set size...", "op": "dims", "width": 320, "height": 240},
        {"op": "draw", "frame": 3}
      ]
    }

The script reads each command as the game did, and removes the commands that
the game ignored. It changes the "slides.*" resources from STRTAB to STRING.
Run it after convert_string_tables.py.

Usage: convert_slideshows.py CONTENT_DIR
"""

import argparse
import json
import re
import sys
from pathlib import Path

VERSION = 1
MAX_FADE_IN_LINES = 15
FADE_IN_BUFFER = 511
INT = re.compile(r"\s*([+-]?\d+)")
HEX = re.compile(r"\s*[+-]?(?:0[xX])?([0-9a-fA-F]+)")
WORD = re.compile(r"\s*(\S+)")


class Ignored(Exception):
    """The game ignored the command."""


def ints(text, count):
    """The first 'count' integers of 'text' and the rest of it, as scnlib reads "{} {} ..."."""
    values = []
    for _ in range(count):
        match = INT.match(text)
        if not match:
            return None, text
        values.append(int(match.group(1)))
        text = text[match.end():]
    return values, text


def needs_ints(text, count):
    values, rest = ints(text, count)
    if values is None:
        raise Ignored(f"needs {count} numbers")
    return values, rest


def until_new_line(text):
    """The result of scanUntilNewLine, with its use of an absolute end index as the count."""
    start = len(text) - len(text.lstrip(" \t"))
    if start == len(text):
        return ""
    end = text.find("\n", start)
    return text[start:] if end < 0 else text[start:start + end]


def color(text):
    match = HEX.match(text)
    if not match:
        raise Ignored("needs a color")
    value = int(match.group(1), 16)
    return [(value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF]


def resolution(op, sd_name, hd_name):
    return {sd_name: {"resolution": "sd"}, hd_name: {"resolution": "hd"}}.get(op, {})


def leading_ints(text, max_count):
    """The most integers, up to 'max_count', that text starts with, as the game tried "{} {} ..." with fewer each time."""
    for count in range(max_count, 0, -1):
        values, _ = ints(text, count)
        if values is not None:
            return values
    return []


def draw(args):
    first = WORD.match(args)
    if first and first.group(1).upper() == "SIS":
        # The numbers follow SIS, as in the original game. The game skipped the first number by mistake from
        # commit 130757f5 on, so it drew the SIS at the wrong place.
        numbers = leading_ints(args[first.end():], 4)
        if not numbers:
            raise Ignored("DRAW SIS needs numbers")
        command = {"op": "drawSis"}
    else:
        values = leading_ints(args, 5)
        if not values:
            raise Ignored("DRAW needs numbers")
        command = {"op": "draw", "frame": values[0]}
        numbers = values[1:]
    # numbers: x, y, scale, angle. With x only, the game draws at 0, 0.
    if len(numbers) >= 2:
        command["x"], command["y"] = numbers[0], numbers[1]
    if len(numbers) >= 3:
        command["scale"] = numbers[2]
    if len(numbers) >= 4 and numbers[3] != 0:
        command["angle"] = numbers[3]
    return command


def convert_command(line):
    """The JSON command of a command line, or None if it is empty."""
    match = WORD.match(line)
    if not match:
        return None
    op = match.group(1).upper()
    args = line[match.end():]
    args = args[len(args) - len(args.lstrip(" \t")):]

    if op == "DIMS":
        (width, height), _ = needs_ints(args, 2)
        return {"op": "dims", "width": width, "height": height}
    if op in ("FONT", "FONT1X", "FONT4X"):
        values, rest = ints(args, 1)
        if values is None or not 0 <= values[0] < 5:
            raise Ignored("bad font index")
        command = {"op": "font", **resolution(op, "FONT1X", "FONT4X"), "index": values[0]}
        file = until_new_line(rest)
        if file:
            command["file"] = file
        return command
    if op in ("ANI", "ANI1X", "ANI4X"):
        return {"op": "ani", **resolution(op, "ANI1X", "ANI4X"), "file": args}
    if op == "MUSIC":
        return {"op": "music", "file": args}
    if op == "CALL":
        return {"op": "call", "file": args}
    if op == "DITTY":
        return {"op": "ditty", "ship": args}
    timed = {"WAIT": "wait", "SPINWAIT": "spinWait", "SYNC": "sync", "DSYNC": "deltaSync",
             "FTC": "fadeToColor", "FTB": "fadeToBlack", "FTW": "fadeToWhite"}
    if op in timed:
        (ms,), _ = needs_ints(args, 1)
        return {"op": timed[op], "ms": ms}
    simple = {"WAITDITTY": "waitDitty", "RESYNC": "resync", "TFO": "textFadeOut", "SAVEBG": "saveBackground",
              "RESTBG": "restoreBackground", "BATCH": "batch", "UNBATCH": "unbatch", "CLS": "clearScreen",
              "GETRECT": "getRect"}
    if op in simple:
        return {"op": simple[op]}
    colors = {"BGC": "backgroundColor", "TC": "textColor", "TBC": "textBackColor", "TFC": "textFadeColor"}
    if op in colors:
        return {"op": colors[op], "color": color(args)}
    if op == "TVA":
        return {"op": "textAlign", "align": {"T": "top", "M": "middle"}.get(args[:1].upper(), "bottom")}
    if op == "TE":
        return {"op": "textEffect", "effect": "traced" if args[:1].upper() == "T" else "plain"}
    if op == "TEXT":
        (x, y), rest = needs_ints(args, 2)
        return {"op": "text", "x": x, "y": y, "text": until_new_line(rest)}
    if op == "TEXTSPIN":
        (x, y), rest = needs_ints(args, 2)
        return {"op": "textSpin", "x": x, "y": y, "text": rest}
    if op == "SPINSTAT":
        (filled, empty), rest = needs_ints(args, 2)
        return {"op": "spinStat", "filled": filled, "empty": empty, "text": until_new_line(rest)}
    if op == "TFI":
        text = args[:FADE_IN_BUFFER]
        lines = text.split("\n")
        if text.endswith("\n"):
            lines.pop()
        if not text:
            raise Ignored("TFI needs text")
        return {"op": "textFadeIn", "lines": lines[:MAX_FADE_IN_LINES]}
    if op == "DRAW":
        return draw(args)
    if op in ("LINE", "LINESPIN"):
        (x1, y1, x2, y2), _ = needs_ints(args, 4)
        return {"op": "line" if op == "LINE" else "lineSpin", "x1": x1, "y1": y1, "x2": x2, "y2": y2}
    if op == "STATBOX":
        (x, y), _ = needs_ints(args, 2)
        return {"op": "statBox", "x": x, "y": y}
    if op == "MOVIE":
        (fps, first, last), _ = needs_ints(args, 3)
        if fps <= 0 or first < 0 or last < first:
            raise Ignored("bad MOVIE values")
        return {"op": "movie", "fps": fps, "from": first, "to": last}
    if op == "ANIMATE":
        (first, last, loops, ms, fps), _ = needs_ints(args, 5)
        return {"op": "animate", "first": first, "last": last, "loops": loops, "ms": ms, "fps": fps}
    if op == "NOOP":
        raise Ignored("NOOP")
    raise Ignored("unknown command")


def convert_slideshow(path):
    strings = json.loads(path.read_text(encoding="utf-8"))["strings"]
    commands = []
    for index, entry in enumerate(strings):
        line = "\n".join(entry.get("text", []))
        try:
            command = convert_command(line)
        except Ignored as reason:
            if str(reason) != "NOOP":
                print(f"note: {path}: string {index} '{line[:60]}': the game ignored it ({reason}).", file=sys.stderr)
            continue
        if command is None:
            continue
        commands.append({"comment": entry["id"], **command} if entry["id"].strip() else command)
    return commands


def slideshow_json(commands):
    text = ",\n".join("    " + json.dumps(command, ensure_ascii=False) for command in commands)
    return f'{{\n  "version": {VERSION},\n  "commands": [\n{text}\n  ]\n}}\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("content", type=Path)
    args = parser.parse_args()

    content = args.content.resolve()
    rmps = [path for path in sorted(content.rglob("*.rmp.json")) if ".git" not in path.parts]
    pending = []
    for rmp in rmps:
        for key, value in json.loads(rmp.read_text(encoding="utf-8"))["resources"].items():
            if key.startswith("slides.") and value["type"] == "STRTAB":
                pending.append(value["data"])

    done = {}
    while pending:
        name = pending.pop()
        if name in done or not (content / name).exists():
            continue
        done[name] = convert_slideshow(content / name)
        pending.extend(command["file"] for command in done[name] if command["op"] == "call")
    for name, commands in done.items():
        (content / name).write_bytes(slideshow_json(commands).encode("utf-8"))
    print(f"converted {len(done)} slideshows")

    for rmp in rmps:
        text = rmp.read_bytes().decode("utf-8")
        new = re.sub(r'("slides\.[^"]+": \{\s*"type": )"STRTAB"', r'\1"STRING"', text)
        if new != text:
            rmp.write_bytes(new.encode("utf-8"))
            print(f"changed {rmp}")


if __name__ == "__main__":
    main()
