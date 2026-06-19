#!/usr/bin/env python3
"""
Generates docs/lua/api.md from src/Lua/api.lua.

Parses LuaCATS/EmmyLua annotations and emits Markdown suitable for the
SvelteKit website. Run from the repository root:

    python tools/generate_lua_docs.py
"""

import re
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
API_LUA_PATH = REPO_ROOT / "src" / "Lua" / "api.lua"
OUTPUT_PATH = REPO_ROOT / "docs" / "lua" / "api.md"


def slugify(text: str) -> str:
    return re.sub(r"[^a-z0-9\s-]", "", text.lower()).strip().replace(" ", "-")


def escape_markdown(text: str) -> str:
    return text.replace("|", "\\|")


def strip_comment_markers(line: str) -> str:
    return re.sub(r"^---? ?", "", line)


def parse_type_and_desc(annotation: str):
    match = re.match(r"^@(\w+)\s+(.*)$", annotation)
    if not match:
        return {"kind": annotation, "rest": ""}
    return {"kind": match.group(1), "rest": match.group(2)}


def parse_param(rest: str):
    match = re.match(r"^(\w+\??)\s+(\S+)(?:\s+(.*))?$", rest)
    if not match:
        return {"name": rest, "type": "", "desc": ""}
    return {"name": match.group(1), "type": match.group(2), "desc": match.group(3) or ""}


def parse_return(rest: str):
    match = re.match(r"^(\S+)\s*(?:#\s*(.*))?$", rest)
    if not match:
        return {"type": rest, "desc": ""}
    return {"type": match.group(1), "desc": match.group(2) or ""}


def parse_class(rest: str):
    match = re.match(r"^(\S+)(?:\s+(.*))?$", rest)
    if not match:
        return {"name": rest, "desc": ""}
    return {"name": match.group(1), "desc": match.group(2) or ""}


def parse_alias(rest: str):
    match = re.match(r"^(\S+)\s+(.*)$", rest)
    if not match:
        return {"name": rest, "type": "", "desc": ""}
    return {"name": match.group(1), "type": match.group(2), "desc": ""}


def parse_field(rest: str):
    match = re.match(r"^(\w+\??)\s+(\S+)(?:\s+(.*))?$", rest)
    if not match:
        return {"name": rest, "type": "", "desc": ""}
    return {"name": match.group(1), "type": match.group(2), "desc": match.group(3) or ""}


def parse_function_signature(line: str):
    match = re.match(r"^function\s+([\w.]+)\s*\((.*)\)\s*end", line)
    if not match:
        return None
    return {"name": match.group(1), "args": match.group(2).strip()}


def friendly_module_name(name: str) -> str:
    mapping = {
        "emu": "Emulator",
        "memory": "Memory",
        "debugger": "Debugger",
        "wgui": "GUI (wgui)",
        "d2d": "Direct2D (d2d)",
        "input": "Input",
        "joypad": "Joypad",
        "movie": "Movie",
        "savestate": "Savestate",
        "iohelper": "IO Helper",
        "avi": "AVI",
        "hotkey": "Hotkey",
        "action": "Action",
        "clipboard": "Clipboard",
        "global": "Global",
    }
    return mapping.get(name, name.capitalize())


def flush_pending(item_type: str, item: dict, pending_description: list, pending_annotations: list):
    if pending_description:
        item["description"] = " ".join(pending_description).strip()
        pending_description.clear()
    if pending_annotations:
        for ann in pending_annotations:
            parsed = parse_type_and_desc(ann)
            kind = parsed["kind"]
            rest = parsed["rest"]
            if kind == "param":
                item.setdefault("params", []).append(parse_param(rest))
            elif kind == "return":
                item.setdefault("returns", []).append(parse_return(rest))
            elif kind == "deprecated":
                item["deprecated"] = rest or True
            elif kind == "nodiscard":
                item["nodiscard"] = True
            elif kind == "field":
                item.setdefault("fields", []).append(parse_field(rest))
        pending_annotations.clear()


def main():
    source = API_LUA_PATH.read_text(encoding="utf-8")
    lines = source.splitlines()

    modules: dict[str, dict] = {}
    global_module = {"name": "global", "title": "Global", "functions": [], "aliases": [], "classes": []}
    mupen_enums: list[dict] = []
    standalone_aliases: list[dict] = []
    standalone_classes: list[dict] = []

    current_module = None
    pending_description: list[str] = []
    pending_annotations: list[str] = []
    last_region_label: str | None = None
    in_mupen_table = False
    mupen_depth = 0

    def get_current_module():
        return current_module if current_module is not None else global_module

    i = 0
    while i < len(lines):
        line = lines[i]
        trimmed = line.strip()

        if trimmed.startswith("Mupen = {"):
            in_mupen_table = True
            mupen_depth = 1
            pending_description.clear()
            pending_annotations.clear()
            i += 1
            continue

        if in_mupen_table:
            for ch in trimmed:
                if ch == "{":
                    mupen_depth += 1
                if ch == "}":
                    mupen_depth -= 1
            if mupen_depth <= 0:
                in_mupen_table = False
                pending_description.clear()
                pending_annotations.clear()

            if trimmed.startswith("---@enum"):
                rest = strip_comment_markers(trimmed).replace("@enum ", "", 1)
                mupen_enums.append({"name": rest, "values": []})
            elif mupen_enums and re.match(r"^\w+\s*=\s*\{", trimmed):
                enum_name = re.match(r"^(\w+)\s*=", trimmed).group(1)
                current_enum = next((e for e in mupen_enums if e["name"] == enum_name), mupen_enums[-1])
                depth = 1
                value_lines = []
                j = i + 1
                while j < len(lines) and depth > 0:
                    inner = lines[j].strip()
                    for ch in inner:
                        if ch == "{":
                            depth += 1
                            mupen_depth += 1
                        if ch == "}":
                            depth -= 1
                            mupen_depth -= 1
                    if depth > 0:
                        value_lines.append(lines[j])
                    j += 1

                current_enum["values"] = []
                for vl in value_lines:
                    vl = vl.strip()
                    if not vl or vl.startswith("--"):
                        continue
                    m = re.match(r"^(\w+)\s*=\s*([^,]+),?\s*(?:--\s*(.*))?$", vl)
                    if m:
                        current_enum["values"].append(
                            {"key": m.group(1), "value": m.group(2).strip(), "comment": m.group(3) or ""}
                        )
                    else:
                        current_enum["values"].append({"key": vl, "value": "", "comment": ""})
                i = j - 1

            i += 1
            continue

        module_decl = re.match(r"^(\w+)\s*=\s*\{\}\s*$", trimmed)
        if module_decl:
            name = module_decl.group(1)
            if name not in modules:
                modules[name] = {"name": name, "title": friendly_module_name(name), "functions": [], "aliases": [], "classes": []}
            i += 1
            continue

        if trimmed.startswith("--#region"):
            region_label = (last_region_label or trimmed.replace("--#region", "", 1).strip()).strip()
            if region_label.lower() == "global functions":
                current_module = global_module
            else:
                m = re.match(r"^(\w+)\s+functions?$", region_label, re.IGNORECASE)
                if m:
                    name = m.group(1).lower()
                    if name not in modules:
                        modules[name] = {"name": name, "title": friendly_module_name(name), "functions": [], "aliases": [], "classes": []}
                    current_module = modules[name]
            last_region_label = None
            pending_description.clear()
            pending_annotations.clear()
            i += 1
            continue

        if trimmed.startswith("--#endregion"):
            current_module = None
            pending_description.clear()
            pending_annotations.clear()
            i += 1
            continue

        if trimmed.startswith("---") and not trimmed.startswith("---@"):
            pending_description.append(strip_comment_markers(trimmed))
            i += 1
            continue

        # Region labels are plain comments on the line before --#region
        if trimmed.startswith("--") and not trimmed.startswith("---"):
            text = trimmed.lstrip("-").strip()
            if text:
                last_region_label = text
            i += 1
            continue

        if trimmed.startswith("---@"):
            ann = strip_comment_markers(trimmed)
            kind = re.match(r"^@(\w+)", ann).group(1)

            if kind == "class":
                rest = ann.replace("@class ", "", 1)
                cls = parse_class(rest)
                flush_pending("class", cls, pending_description, pending_annotations)
                get_current_module()["classes"].append(cls)
                standalone_classes.append(cls)
                pending_description.clear()
                pending_annotations.clear()
            elif kind == "alias":
                rest = ann.replace("@alias ", "", 1)
                alias = parse_alias(rest)
                flush_pending("alias", alias, pending_description, pending_annotations)
                get_current_module()["aliases"].append(alias)
                standalone_aliases.append(alias)
                pending_description.clear()
                pending_annotations.clear()
            elif kind == "field":
                current_class = None
                current_classes = get_current_module()["classes"]
                if current_classes:
                    current_class = current_classes[-1]
                elif standalone_classes:
                    current_class = standalone_classes[-1]
                if current_class:
                    flush_pending("field", current_class, pending_description, pending_annotations)
                    current_class.setdefault("fields", []).append(parse_field(ann.replace("@field ", "", 1)))
            else:
                pending_annotations.append(ann)

            i += 1
            continue

        func = parse_function_signature(trimmed)
        if func:
            mod_name = func["name"].split(".")[0] if "." in func["name"] else "global"
            target_module = modules.get(mod_name, global_module)
            item = {"name": func["name"], "args": func["args"], "params": [], "returns": []}
            flush_pending("function", item, pending_description, pending_annotations)
            target_module["functions"].append(item)
            pending_description.clear()
            pending_annotations.clear()
            i += 1
            continue

        i += 1

    out: list[str] = []
    out.append("# Mupen64 Lua API Reference")
    out.append("")
    out.append("This page documents the public Lua scripting API exposed by Mupen64.")
    out.append("")
    rel_path = Path("src/Lua/api.lua").as_posix()
    out.append(f"The API is defined in [`{rel_path}`](../../{rel_path}).")
    out.append("")

    all_modules = [global_module] + [m for m in modules.values() if m["functions"] or m["classes"] or m["aliases"]]

    out.append("## Table of Contents")
    out.append("")
    for mod in all_modules:
        out.append(f"- [{mod['title']}](#{slugify(mod['title'])})")
    if mupen_enums:
        out.append("- [Enums](#enums)")
    out.append("")

    for mod in all_modules:
        out.append(f"## {mod['title']}")
        out.append("")

        for alias in mod["aliases"]:
            out.append(f"### {alias['name']}")
            out.append("")
            if alias.get("description"):
                out.append(alias["description"])
                out.append("")
            out.append(f"**Type:** `{alias['type']}`")
            out.append("")

        for cls in mod["classes"]:
            out.append(f"### {cls['name']}")
            out.append("")
            if cls.get("description"):
                out.append(cls["description"])
                out.append("")
            if cls.get("fields"):
                out.append("| Field | Type | Description |")
                out.append("|-------|------|-------------|")
                for f in cls["fields"]:
                    out.append(f"| {escape_markdown(f['name'])} | `{escape_markdown(f['type'])}` | {escape_markdown(f['desc'])} |")
                out.append("")

        for fn in mod["functions"]:
            out.append(f"### {fn['name']}()")
            out.append("")
            if fn.get("deprecated"):
                reason = fn["deprecated"] if isinstance(fn["deprecated"], str) else "This function is deprecated."
                out.append(f"> [!CAUTION] Deprecated: {reason}")
                out.append("")
            if fn.get("nodiscard"):
                out.append("> [!NOTE] The return value of this function should not be ignored.")
                out.append("")
            if fn.get("description"):
                out.append(fn["description"])
                out.append("")
            out.append(f"**Signature:** `function {fn['name']}({fn['args']})`")
            out.append("")
            if fn.get("params"):
                out.append("| Parameter | Type | Description |")
                out.append("|-----------|------|-------------|")
                for p in fn["params"]:
                    out.append(f"| {escape_markdown(p['name'])} | `{escape_markdown(p['type'])}` | {escape_markdown(p['desc'])} |")
                out.append("")
            if fn.get("returns"):
                out.append("| Return | Type | Description |")
                out.append("|--------|------|-------------|")
                for r in fn["returns"]:
                    out.append(f"| `{escape_markdown(r['type'])}` | {escape_markdown(r['desc'])} |")
                out.append("")

    if mupen_enums:
        out.append("## Enums")
        out.append("")
        for en in mupen_enums:
            out.append(f"### {en['name']}")
            out.append("")
            out.append("| Name | Value | Description |")
            out.append("|------|-------|-------------|")
            for v in en["values"]:
                out.append(f"| {escape_markdown(v['key'])} | {escape_markdown(v['value'])} | {escape_markdown(v['comment'])} |")
            out.append("")

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    final = ["---", "title: Lua API Reference", "---", ""] + out
    OUTPUT_PATH.write_text("\n".join(final), encoding="utf-8")
    print(f"Generated {OUTPUT_PATH} ({len(final)} lines)")


if __name__ == "__main__":
    main()
