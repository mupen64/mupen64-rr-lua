#
#  Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
#
#  SPDX-License-Identifier: GPL-2.0-or-later
#

# Lists the translation units that take the longest to compile, based on build/.ninja_log.

from pathlib import Path

durations = {}
for line in (Path(__file__).parent.parent / "build" / ".ninja_log").read_text().splitlines()[1:]:
    start, end, _, output, *_ = line.split("\t")
    if output.endswith((".o", ".obj")):
        # Ninja appends an entry on every rebuild, so later entries win.
        durations[output] = int(end) - int(start)

total = sum(durations.values())
for output, ms in sorted(durations.items(), key=lambda kv: kv[1], reverse=True)[:25]:
    print(f"{ms / 1000:7.2f}s  {ms / total:5.1%}  {output}")
print(f"\n{len(durations)} TUs, {total / 1000:.1f}s total")
