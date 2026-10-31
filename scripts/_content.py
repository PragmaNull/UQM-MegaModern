"""Helpers for the scripts that convert the files of a content directory."""

import json


def write_text(path, text):
    """Writes UTF-8 text with LF line ends."""
    path.write_bytes(text.encode("utf-8"))


def write_json(path, data):
    write_text(path, json.dumps(data, indent=2, ensure_ascii=False) + "\n")


def list_json(version, key, items, fields=None):
    """The JSON text of {"version", **fields, key: items}, with each item and each field on one line."""
    text = f'{{\n  "version": {version},\n'
    text += "".join(f'  "{name}": {json.dumps(value, ensure_ascii=False)},\n' for name, value in (fields or {}).items())
    lines = ",\n".join("    " + json.dumps(item, ensure_ascii=False) for item in items)
    return text + f'  "{key}": [\n{lines}\n  ]\n}}\n'


def content_files(content, pattern):
    """The files of the content directory that match 'pattern', without the files of its git repository."""
    return [path for path in sorted(content.rglob(pattern)) if ".git" not in path.parts]
