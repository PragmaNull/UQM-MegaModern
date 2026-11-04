"""Convert the credits file of a content directory from a string table to credits blocks.

In the string table, each string is "size alignment text": the font size, the
alignments of the columns, and lines of text with a tab between the columns:

    13 R/146,L/165 Fred Ford<TAB>Programming

The script changes the file that the "credits.credits" resource names to:

    {
      "version": 1,
      "blocks": [
        {"id": "...", "size": 13, "columns": [{"align": "right", "x": 146}, {"align": "left", "x": 165}],
         "lines": [["Fred Ford", "Programming"]]}
      ]
    }

A line with one column is a string. The script removes the strings that the game
did not show, such as the comments at the start of the file, and changes the
resource from STRTAB to STRING. Run it after convert_string_tables.py.

Usage: convert_credits.py CONTENT_DIR
"""

import argparse
import json
import re
import sys
from pathlib import Path

from _content import content_files, list_json, write_text

VERSION = 1
MAX_LINES = 50
MAX_COLUMNS = 5
ALIGN = {"L": "left", "C": "center", "R": "right"}
FIRST_TOKENS = re.compile(r"\s*([+-]?\d+)\s+(\S+)\s*")
COLUMN = re.compile(r"(.)(?:/([+-]?\d+))?")


def convert_block(entry, where):
    """The credits block of a string, or None if the game did not show it. Emulates the game."""
    text = "\n".join(entry.get("text", []))
    match = FIRST_TOKENS.match(text)
    if not text or not match or int(match.group(1)) <= 0:
        return None
    rest = text[match.end():]
    lines = rest.split("\n")
    if rest.endswith("\n"):
        lines.pop()
    if not rest:
        return None
    if len(lines) > MAX_LINES:
        print(f"note: {where}: the game showed only the first {MAX_LINES} lines.", file=sys.stderr)
        lines = lines[:MAX_LINES]
    columns = []
    for token in [token for token in match.group(2).split(",") if token][:MAX_COLUMNS]:
        column_match = COLUMN.match(token)
        column = {"align": ALIGN.get(column_match.group(1), "left")}
        if column_match.group(1) not in ALIGN:
            print(f"note: {where}: unknown alignment '{token}' is now left, as the game used it.", file=sys.stderr)
        elif column_match.group(2) is not None:
            column["x"] = int(column_match.group(2))
        columns.append(column)
    cells = [line.split("\t", MAX_COLUMNS - 1) for line in lines]
    block = {"id": entry["id"], "size": int(match.group(1)), "columns": columns or [{"align": "left"}],
             "lines": [line[0] if len(line) == 1 else line for line in cells]}
    return block


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("content", type=Path)
    args = parser.parse_args()

    content = args.content.resolve()
    for rmp in content_files(content, "*.rmp.json"):
        text = rmp.read_bytes().decode("utf-8")
        resource = json.loads(text)["resources"].get("credits.credits")
        if not resource or resource["type"] != "STRTAB":
            continue
        path = content / resource["data"]
        strings = json.loads(path.read_text(encoding="utf-8"))["strings"]
        blocks = [block for index, entry in enumerate(strings) if (block := convert_block(entry, f"{path}: string {index}"))]
        write_text(path, list_json(VERSION, "blocks", blocks))
        print(f"converted {path}: {len(blocks)} blocks")
        new = re.sub(r'("credits\.credits": \{\s*"type": )"STRTAB"', r'\1"STRING"', text)
        write_text(rmp, new)
        print(f"changed {rmp}")


if __name__ == "__main__":
    main()
