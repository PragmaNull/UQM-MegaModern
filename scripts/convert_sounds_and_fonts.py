"""Convert the sound bank (.snd) and font metrics (kerndat.fnt) files of a content directory to JSON.

A .snd file has one sound file name on each line. The script writes
<name>.snd.json next to it:

    {"version": 1, "sounds": ["getcrew.wav", "shipdies.wav"]}

and changes the SNDRES references in the .rmp.json files to the new file.

A kerndat.fnt file has the line "name leading char_space kern_amount
vert_align", and then a line "hex_char left right" for each kerned character.
The script writes kerndat.fnt.json next to it:

    {
      "version": 1,
      "leading": 36, "charSpace": 4, "kernAmount": 0, "vertAlign": 12,
      "kerning": {
        " ": [3, 3],
        "!": [3, 3]
      }
    }

With --delete it removes the old files.

Usage: convert_sounds_and_fonts.py CONTENT_DIR [--delete]
"""

import argparse
import json
import re
import sys
from pathlib import Path

VERSION = 1
MAX_UNICODE = 0xFFFF
MAX_KERN = 3
HEADER = re.compile(r"^\s*(\S+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(-?\d+)")
KERN = re.compile(r"^\s*([0-9a-fA-F]+)\s+(-?\d+)\s+(-?\d+)")


def write_text(path, text):
    path.write_bytes(text.encode("utf-8"))


def convert_sound_bank(path):
    sounds = []
    for number, line in enumerate(path.read_text(encoding="utf-8").split("\n"), 1):
        if line.strip():
            sounds.append(line.split()[0])
    write_text(path.with_name(path.name + ".json"), json.dumps({"version": VERSION, "sounds": sounds}, indent=2) + "\n")


def kern_value(value, where):
    if not 0 <= value <= MAX_KERN:
        print(f"note: {where}: kern value {value} is now {MAX_KERN}, as the game used it.", file=sys.stderr)
        return MAX_KERN
    return value


def convert_font_metrics(path):
    """Returns False if the game ignored the file, because its first line is not a valid header."""
    lines = path.read_text(encoding="utf-8").split("\n")
    header = HEADER.match(lines[0]) if lines else None
    if not header:
        print(f"note: {path}: the game ignored this file, because its first line is not valid. "
              "The script does not convert it.", file=sys.stderr)
        return False
    kerning = {}
    for number, line in enumerate(lines[1:], 2):
        match = KERN.match(line)
        if not match:
            continue
        ch = int(match.group(1), 16)
        where = f"{path}:{number}"
        if ch == 0 or ch >= MAX_UNICODE:
            print(f"note: {where}: dropped the character {ch:#x}, which the game cannot kern.", file=sys.stderr)
            continue
        kerning[chr(ch)] = [kern_value(int(match.group(2)), where), kern_value(int(match.group(3)), where)]
    fields = {"leading": int(header.group(2)), "charSpace": int(header.group(3)),
              "kernAmount": int(header.group(4)), "vertAlign": int(header.group(5))}
    text = f'{{\n  "version": {VERSION},\n'
    text += "".join(f'  "{name}": {value},\n' for name, value in fields.items())
    entries = [f"    {json.dumps(ch, ensure_ascii=False)}: {json.dumps(value)}" for ch, value in kerning.items()]
    text += '  "kerning": {\n' + ",\n".join(entries) + "\n  }\n}\n"
    write_text(path.with_name(path.name + ".json"), text)
    return True


def convert_references(content):
    for path in sorted(content.rglob("*.rmp.json")):
        if ".git" in path.parts:
            continue
        text = path.read_bytes().decode("utf-8")
        new = re.sub(r'("type": "SNDRES",\s*"data": "[^"]+\.snd)(")', r"\1.json\2", text)
        if new != text:
            write_text(path, new)
            print(f"changed {path}")


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("content", type=Path)
    parser.add_argument("--delete", action="store_true", help="remove the converted files")
    args = parser.parse_args()

    content = args.content.resolve()
    banks = [path for path in sorted(content.rglob("*.snd")) if ".git" not in path.parts]
    fonts = [path for path in sorted(content.rglob("kerndat.fnt")) if ".git" not in path.parts]
    for path in banks:
        convert_sound_bank(path)
    converted = banks + [path for path in fonts if convert_font_metrics(path)]
    print(f"converted {len(banks)} sound banks and {len(converted) - len(banks)} font metrics files")
    convert_references(content)
    if args.delete:
        for path in converted:
            path.unlink()
        print(f"removed {len(converted)} files")


if __name__ == "__main__":
    main()
