"""Convert the string table (STRTAB) .txt files of a content directory to JSON.

A string table file has a "#(id)" line before each string:

    #(ALGOLITES 1)
     ---- REPORT FROM SURFACE ----

The script writes <name>.json next to each <name>.txt file:

    {
      "version": 1,
      "strings": [
        {"id": "ALGOLITES 1", "text": [" ---- REPORT FROM SURFACE ----"]}
      ]
    }

"text" is an array of lines, and a string with no lines has no "text". The
game finds the strings by their position; "id" only names them. The script
converts the files that STRTAB resources name and the files that slideshow
"CALL" commands name, and it changes those references to the new files.
With --delete it removes the old files. Run it after
convert_conversations.py, which converts the conversation .txt files.

Usage: convert_string_tables.py CONTENT_DIR [--delete]
"""

import argparse
import json
import re
from pathlib import Path

from _content import content_files, list_json, write_text

VERSION = 1
HEADER = re.compile(r"#[^(]*\((.*)\)")
CALL = re.compile(r"^(\s*CALL\s+)(\S+\.txt)(\s*)$", re.IGNORECASE)
STRTAB_DATA = re.compile(r'("type": "STRTAB",\s*"data": "[^"]+)\.txt(")')


def parse_strings(path):
    """The strings of a string table file. Emulates the game: "#" lines without "(" and ")" are comments."""
    strings = []
    body = None
    for line in path.read_text(encoding="utf-8").split("\n"):
        if line.startswith("#"):
            match = HEADER.match(line)
            if match:
                body = []
                strings.append({"id": match.group(1), "body": body})
        elif body is not None:
            body.append(line)
    for entry in strings:
        body = entry.pop("body")
        # The game removes the line breaks at the end of the text. A string with no lines has no text.
        if body:
            entry["text"] = "\n".join(body).rstrip("\r\n").split("\n")
    return strings


def calls(strings):
    for entry in strings:
        for line in entry.get("text", []):
            match = CALL.match(line)
            if match:
                yield match.group(2)


def convert_call(line, tables):
    match = CALL.match(line)
    if match and match.group(2) in tables:
        return match.group(1) + match.group(2)[:-len(".txt")] + ".json" + match.group(3)
    return line


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("content", type=Path)
    parser.add_argument("--delete", action="store_true", help="remove the converted .txt files")
    args = parser.parse_args()

    content = args.content.resolve()
    rmps = content_files(content, "*.rmp.json")
    pending = []
    for rmp in rmps:
        for value in json.loads(rmp.read_text(encoding="utf-8"))["resources"].values():
            if value["type"] == "STRTAB" and value["data"].endswith(".txt"):
                pending.append(value["data"])

    tables = {}
    while pending:
        name = pending.pop()
        if name in tables or not (content / name).exists():
            continue
        tables[name] = parse_strings(content / name)
        pending.extend(calls(tables[name]))

    for name, strings in tables.items():
        for entry in strings:
            if "text" in entry:
                entry["text"] = [convert_call(line, tables) for line in entry["text"]]
        out = (content / name).with_suffix(".json")
        write_text(out, list_json(VERSION, "strings", strings))
    print(f"converted {len(tables)} string tables")

    for rmp in rmps:
        text = rmp.read_bytes().decode("utf-8")
        new = STRTAB_DATA.sub(r"\1.json\2", text)
        if new != text:
            write_text(rmp, new)
            print(f"changed {rmp}")

    if args.delete:
        for name in tables:
            (content / name).unlink()
        print(f"removed {len(tables)} files")


if __name__ == "__main__":
    main()
