#
#  Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
#
#  SPDX-License-Identifier: GPL-2.0-or-later
#

import sys
from pathlib import Path

HEADER_C = """\
/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
"""
HEADER_CMAKE = """\
#[===[
Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)

SPDX-License-Identifier: GPL-2.0-or-later
]===]
"""
HEADER_PYTHON = """\
#
#  Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
#
#  SPDX-License-Identifier: GPL-2.0-or-later
#
"""
HEADER_LUA = """\
--
-- Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
--
-- SPDX-License-Identifier: GPL-2.0-or-later
--
"""

def header_for(path: Path) -> str | None:
    # special: match CMakeLists.txt
    if path.name == "CMakeLists.txt":
        return HEADER_CMAKE

    match path.suffixes:
        case [".c" | ".cpp" | ".h" | ".hpp"]:
            return HEADER_C
        case [".qml"]:
            return HEADER_C
        case [".cmake"]:
            return HEADER_CMAKE
        case [".py"]:
            return HEADER_PYTHON
        case [".lua"]:
            return HEADER_LUA

    return None

def check_header(path: Path) -> bool:
    "Checks if the file header for this path is valid."
    header = header_for(path)
    if header is None:
        return True

    header_lines = header.splitlines()

    with open(path, "r") as file:
        for line_no, (file_line, header_line) in enumerate(zip(file, header_lines), start=1):
            file_line = file_line.rstrip("\n").lstrip("\uFEFF")
            if file_line != header_line:
                print(f"{path}:{line_no}: failed to match header")
                print(f"- {file_line}")
                print(f"+ {header_line}")
                return False

    return True

    

def main():
    if len(sys.argv) < 2:
        print(f"usage: {sys.argv[0]} <files>...")
        sys.exit(1)

    failed_list = []

    for str_path in sys.argv[1:]:
        path = Path(str_path)
        if not path.exists():
            raise FileNotFoundError(f"path {path} doesn't exist")

        passed = check_header(path)
        if not passed:
            failed_list.append(path)

    if len(failed_list) > 0:
        print("Header check failed!")
        sys.exit(-1)
    else:
        print("Header check succeeded")


if __name__ == "__main__":
    main()