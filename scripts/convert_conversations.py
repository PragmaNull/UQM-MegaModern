"""Convert the conversation files of a content package from .txt and .ts to JSON.

A CONVERSATION resource names a text file, a voice clip directory and a
timestamp file:

    comm.arilou.dialogue = CONVERSATION:base/comm/arilou/arilou.txt:<clip dir>:<timestamps.ts>

The script writes each text file as a JSON file next to it, with the
timestamps in it when the same package has the text. A package that adds
timestamps to the text of another package gets one <package>-timing.json file
next to its .rmp file. The script then changes the .rmp lines to name the JSON
files. With --delete it removes the .txt and .ts files that it converted.

Usage: convert_conversations.py CONTENT_DIR [RMP_FILE ...] [--delete]
Without RMP_FILE, it converts uqm.rmp and every addons/*/*.rmp in CONTENT_DIR.
"""

import argparse
import json
import re
import sys
from pathlib import Path

RESOURCE = re.compile(r"^(\s*(\S+)\s*=\s*CONVERSATION:)(\S+)(.*)$")
HEADER = re.compile(r"#[^(]*\(([^)]*)\)(.*)")


def read_lines(path):
    return path.read_text(encoding="utf-8").split("\n")


def parse_text(path):
    """The entries of a .txt file: id, clip and the text lines, in file order.
    Reads the JSON file instead if the text file is converted already."""
    if not path.exists() and path.with_suffix(".json").exists():
        return json.loads(path.with_suffix(".json").read_text(encoding="utf-8"))["lines"]
    entries = []
    body = None
    for line in read_lines(path):
        if line.startswith("#"):
            match = HEADER.match(line)
            if not match or not match.group(1):
                continue
            body = []
            entries.append({"id": match.group(1), "clip": match.group(2).strip(), "body": body})
        elif body is not None:
            body.append(line)
    for entry in entries:
        body = entry.pop("body")
        # The game removes the line breaks at the end of the text. A block with no lines has no text.
        if body:
            entry["text"] = "\n".join(body).rstrip("\r\n").split("\n")
    return entries


def parse_timestamps(path, entries):
    """The timestamp strings of a .ts file by id. The lines must be in the order of the entries."""
    lines = [line for line in read_lines(path) if line.strip()]
    timestamps = {}
    for index, entry in enumerate(entries):
        line = lines[index] if index < len(lines) else ""
        position = line.find(entry["id"]) if line.startswith("#") else -1
        if position < 0:
            print(f"warning: {path}: no timestamps for '{entry['id']}' at line {index + 1}. "
                  "The game ignored all timestamps of this file.", file=sys.stderr)
            return {}
        value = line[position + len(entry["id"]) + 1:].strip()
        if value:
            timestamps[entry["id"]] = value
    return timestamps


def timing_fields(value):
    """The timing fields of a timestamp string, such as "100,200;;300"."""
    segments = [[int(number) for number in segment.split(",") if number] for segment in value.split(";")]
    fields = {"timing": segments[0]}
    if len(segments) > 1:
        fields["subclipTiming"] = segments[1:]
    return fields


def write_text(path, text):
    path.write_bytes(text.encode("utf-8"))


def write_json(path, data):
    write_text(path, json.dumps(data, indent=2, ensure_ascii=False) + "\n")


def text_json(entries, conversation, timestamps):
    lines = []
    for entry in entries:
        line = {"id": entry["id"]}
        if entry.get("clip"):
            line["clip"] = entry["clip"]
        if "text" in entry:
            line["text"] = entry["text"]
        if entry["id"] in timestamps:
            line.update(timing_fields(timestamps[entry["id"]]))
        lines.append(line)
    return {"version": 1, "conversation": conversation, "lines": lines}


def conversation_name(resource, text_path):
    match = re.fullmatch(r"comm\.([^.]+)\.dialogue", resource)
    return match.group(1) if match else text_path.stem


def convert_rmp(content, rmp, converted, deleted):
    package = rmp.parent
    overlay = {}
    overlay_path = package / f"{rmp.stem}-timing.json"
    out = []
    lines = rmp.read_text(encoding="utf-8").split("\n")
    for line in lines:
        match = RESOURCE.match(line)
        if not match or not match.group(3).split(":")[0].endswith(".txt"):
            out.append(line)
            continue
        prefix, resource, spec, rest = match.groups()
        parts = spec.split(":")
        text_path = content / parts[0]
        clip_dir = parts[1] if len(parts) > 1 else ""
        timestamp_path = content / parts[2] if len(parts) > 2 and parts[2] else None
        name = conversation_name(resource, text_path)
        entries = parse_text(text_path)
        timestamps = parse_timestamps(timestamp_path, entries) if timestamp_path else {}
        json_path = text_path.with_suffix(".json")
        owned = package in text_path.parents
        if text_path.exists() and (owned or json_path not in converted):
            write_json(json_path, text_json(entries, name, timestamps if owned else {}))
            converted.add(json_path)
            deleted.add(text_path)
        new_parts = [json_path.relative_to(content).as_posix(), clip_dir]
        if timestamp_path:
            if package in timestamp_path.parents:
                deleted.add(timestamp_path)
            if not owned and timestamps:
                overlay[name] = {id: timing_fields(value) for id, value in timestamps.items()}
                new_parts.append(overlay_path.relative_to(content).as_posix())
        out.append(prefix + ":".join(new_parts).rstrip(":") + rest)
    if overlay:
        write_json(overlay_path, {"version": 1, "conversations": overlay})
    if out != lines:
        write_text(rmp, "\n".join(out))
        print(f"converted {rmp}")


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("content", type=Path)
    parser.add_argument("rmps", type=Path, nargs="*")
    parser.add_argument("--delete", action="store_true", help="remove the converted .txt and .ts files")
    args = parser.parse_args()

    content = args.content.resolve()
    rmps = [rmp.resolve() for rmp in args.rmps] or [content / "uqm.rmp", *sorted(content.glob("addons/*/*.rmp"))]
    converted = set()
    deleted = set()
    for rmp in rmps:
        convert_rmp(content, rmp, converted, deleted)
    if args.delete:
        for path in sorted(deleted):
            if path.exists():
                path.unlink()
                print(f"removed {path}")


if __name__ == "__main__":
    main()
