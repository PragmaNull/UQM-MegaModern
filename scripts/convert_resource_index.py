"""Convert the resource index files of a content directory from the old text format to JSON.

The old format has one "key = TYPE:data" line for each resource. The new file
has a "type" and a "data" field for each resource. The data is a string, a
number, a truth value, or an object with named parts:

    comm.arilou.dialogue = CONVERSATION:base/comm/arilou/arilou.json:base/comm/arilou/

    "comm.arilou.dialogue": {"type": "CONVERSATION",
                             "data": {"text": "base/comm/arilou/arilou.json", "clips": "base/comm/arilou/"}}

The script writes <name>.rmp.json next to each <name>.rmp file, and
<name>.key.json next to each <name>.key file. With --delete it removes the old
files. Run convert_conversations.py first: it reads the old .rmp files.

Usage: convert_resource_index.py CONTENT_DIR [FILE ...] [--delete]
Without FILE, it converts uqm.rmp, menu.key, uqm.key and every addons/*/*.rmp in CONTENT_DIR.
"""

import argparse
import json
import sys
from pathlib import Path

VERSION = 1
PARTS = {
    "CONVERSATION": ["text", "clips", "timing"],
    "3DOVID": ["video", "audio", "speech", "loop"],
}


def write_text(path, text):
    path.write_bytes(text.encode("utf-8"))


def parse_line(line):
    """The key and value of a line, or None for a comment or a blank line. Emulates the game's parser."""
    line = line.strip()
    if not line or line.startswith("#"):
        return None
    key, equals, value = line.partition("=")
    if not equals:
        return None
    return key.strip(), value.split("#")[0].strip()


def data_of(type_name, text, where):
    if type_name in ("INT32", "SHIP"):
        return int(text)
    if type_name == "FLT32":
        return float(text)
    if type_name == "bool":
        return text.lower() == "true"
    if type_name in PARTS:
        if type_name == "CONVERSATION" and text.split(":")[0].endswith(".txt"):
            print(f"warning: {where}: run convert_conversations.py first.", file=sys.stderr)
        fields = {}
        for name, part in zip(PARTS[type_name], text.split(":")):
            if part:
                fields[name] = int(part) if name == "loop" else part
        return fields
    return text


def convert(path):
    resources = {}
    for number, line in enumerate(path.read_text(encoding="utf-8").split("\n"), 1):
        where = f"{path}:{number}"
        if line.strip().startswith("#") and "=" in line:
            print(f"note: {where}: dropped the commented-out entry '{line.strip()}'", file=sys.stderr)
        entry = parse_line(line)
        if entry is None:
            continue
        key, value = entry
        type_name, colon, text = value.partition(":")
        if not colon:
            type_name, text = "UNKNOWNRES", value
        resources[key] = {"type": type_name, "data": data_of(type_name, text, where)}
    out = path.with_name(path.name + ".json")
    write_text(out, json.dumps({"version": VERSION, "resources": resources}, indent=2, ensure_ascii=False) + "\n")
    print(f"wrote {out}")


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("content", type=Path)
    parser.add_argument("files", type=Path, nargs="*")
    parser.add_argument("--delete", action="store_true", help="remove the converted files")
    args = parser.parse_args()

    content = args.content.resolve()
    files = [f.resolve() for f in args.files] or [
        *(content / name for name in ("uqm.rmp", "menu.key", "uqm.key")), *sorted(content.glob("addons/*/*.rmp"))]
    for path in files:
        if not path.exists():
            continue
        convert(path)
        if args.delete:
            path.unlink()
            print(f"removed {path}")


if __name__ == "__main__":
    main()
