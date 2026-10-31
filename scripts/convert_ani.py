"""Convert the animation (.ani) files of a content directory to JSON.

An old .ani file has one line for each frame:

    image transparent_color colormap_index hotspot_x hotspot_y

The script writes <name>.ani.json next to each <name>.ani file:

    {
      "version": 1,
      "defaults": {"transparency": 0, "hotspot": [11, 11]},
      "frames": [
        {"image": "asteroid-big-000.png"},
        {"image": "asteroid-big-001.png", "colormap": 1}
      ]
    }

"transparency" is "png" (use the transparency of the PNG file), "none", or the
index of the transparent color. "colormap" is a colormap index, or null for
none. "defaults" holds the values that most frames have, and each frame has
only the values that are different. The script then changes the references to
the .ani files in the .rmp.json files and in the text files, such as the
slideshow scripts. With --delete it removes the old files.

Usage: convert_ani.py CONTENT_DIR [--delete]
"""

import argparse
import json
import re
import sys
from collections import Counter
from pathlib import Path

VERSION = 1
FALLBACK = {"transparency": "png", "colormap": None, "hotspot": [0, 0]}
LINE = re.compile(r"^\s*(\S+)\s+(-?\d+)\s+(-?\d+)\s+(-?\d+)\s+(-?\d+)")
REFERENCE = re.compile(r"(?<!\S)(\S+\.ani)(?=\s|$)", re.MULTILINE)


def write_text(path, text):
    path.write_bytes(text.encode("utf-8"))


def frame_fields(match):
    transparent, colormap = int(match.group(2)), int(match.group(3))
    if transparent < -2 or colormap < -1:
        raise ValueError(f"unknown value in '{match.group(0)}'")
    return {
        "image": match.group(1),
        "transparency": {-2: "png", -1: "none"}.get(transparent, transparent),
        "colormap": None if colormap == -1 else colormap,
        "hotspot": [int(match.group(4)), int(match.group(5))],
    }


def parse_ani(path):
    """The frames of an .ani file. Emulates the game: the frames end at the first line that is not a frame."""
    frames = []
    lines = path.read_text(encoding="utf-8").split("\n")
    for number, line in enumerate(lines, 1):
        match = LINE.match(line)
        if not match:
            if any(rest.strip() for rest in lines[number - 1:]):
                print(f"warning: {path}:{number}: the game ignored this line and the lines after it.", file=sys.stderr)
            break
        frames.append(frame_fields(match))
    return frames


def most_common(frames, name):
    counts = Counter(json.dumps(frame[name]) for frame in frames)
    return json.loads(counts.most_common(1)[0][0])


def ani_json(frames):
    defaults = {}
    for name, fallback in FALLBACK.items():
        value = most_common(frames, name) if frames else fallback
        if value != fallback:
            defaults[name] = value
    lines = []
    for frame in frames:
        out = {"image": frame["image"]}
        for name, fallback in FALLBACK.items():
            if frame[name] != defaults.get(name, fallback):
                out[name] = frame[name]
        lines.append("    " + json.dumps(out, ensure_ascii=False))
    text = f'{{\n  "version": {VERSION},\n'
    if defaults:
        text += f'  "defaults": {json.dumps(defaults, ensure_ascii=False)},\n'
    text += '  "frames": [\n' + ",\n".join(lines) + "\n  ]\n}\n"
    return text


def remove_missing_images(path, frames):
    """Removes the frames at the end that have no image, as the game did. Other frames with no image stay empty."""
    while frames and not (path.parent / frames[-1]["image"]).exists():
        print(f"note: {path}: removed the last frame, because there is no image '{frames[-1]['image']}'.",
              file=sys.stderr)
        frames.pop()
    for frame in frames:
        if not (path.parent / frame["image"]).exists():
            print(f"warning: {path}: there is no image '{frame['image']}'. The frame stays empty.", file=sys.stderr)


def convert_references(content, converted):
    """Changes the .ani paths in the .rmp.json files and the text files to .ani.json paths."""
    known = {path.relative_to(content).as_posix().lower() for path in converted}

    def replace_path(reference):
        return reference + ".json" if reference.replace("\\", "/").lower() in known else reference

    for path in sorted(content.rglob("*")):
        if ".git" in path.parts or not path.is_file() or path.suffix.lower() not in (".json", ".txt"):
            continue
        text = path.read_bytes().decode("utf-8", errors="surrogateescape")
        if ".ani" not in text:
            continue
        if path.name.endswith(".rmp.json"):
            new = re.sub(r'("data": ")([^"]+\.ani)(")', lambda m: m.group(1) + m.group(2) + ".json" + m.group(3), text)
        else:
            new = REFERENCE.sub(lambda m: replace_path(m.group(1)), text)
        if new != text:
            path.write_bytes(new.encode("utf-8", errors="surrogateescape"))
            print(f"changed {path}")


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("content", type=Path)
    parser.add_argument("--delete", action="store_true", help="remove the converted .ani files")
    args = parser.parse_args()

    content = args.content.resolve()
    converted = [path for path in sorted(content.rglob("*.ani")) if ".git" not in path.parts]
    for path in converted:
        frames = parse_ani(path)
        remove_missing_images(path, frames)
        write_text(path.with_name(path.name + ".json"), ani_json(frames))
    print(f"wrote {len(converted)} .ani.json files")
    convert_references(content, converted)
    if args.delete:
        for path in converted:
            path.unlink()
        print(f"removed {len(converted)} .ani files")


if __name__ == "__main__":
    main()
