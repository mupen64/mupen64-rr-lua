
from pathlib import Path
import caseconverter as cc
import itertools as itr
import re

FOLDER = Path("C:\\Users\\jacky\\Documents\\Code\\C++\\mupen64-rr-lua\\src\\Core")
HEADERS2 = [
    ('alloc.hpp', 'Alloc.hpp'),
    ('cheats.hpp', 'Cheats.hpp'),
    ('Memory/pif_lut.hpp', 'Memory/PifLut.hpp'),
    ('Memory/dma.hpp', 'Memory/DMA.hpp'),
    ('Memory/flashram.hpp', 'Memory/Flashram.hpp'),
    ('Memory/memory.hpp', 'Memory/Memory.hpp'),
    ('Memory/pif.hpp', 'Memory/Pif.hpp'),
    ('Memory/savestates.hpp', 'Memory/Savestates.hpp'),
    ('Memory/summercart.hpp', 'Memory/Summercart.hpp'),
    ('Memory/tlb.hpp', 'Memory/TLB.hpp'),
    ('R4300/ops.hpp', 'R4300/Ops.hpp'),
    ('R4300/cop1_helpers.hpp', 'R4300/Cop1Helpers.hpp'),
    ('R4300/disasm.hpp', 'R4300/Disasm.hpp'),
    ('R4300/exception.hpp', 'R4300/Exception.hpp'),
    ('R4300/interrupt.hpp', 'R4300/Interrupt.hpp'),
    ('R4300/macros.hpp', 'R4300/Macros.hpp'),
    ('R4300/r4300.hpp', 'R4300/R4300.hpp'),
    ('R4300/recomp.hpp', 'R4300/Recomp.hpp'),
    ('R4300/recomph.hpp', 'R4300/Recomph.hpp'),
    ('R4300/rom.hpp', 'R4300/Rom.hpp'),
    ('R4300/timers.hpp', 'R4300/Timers.hpp'),
    ('R4300/tracelog.hpp', 'R4300/Tracelog.hpp'),
    ('R4300/vcr.hpp', 'R4300/VCR.hpp'),
    ('R4300/x86/assemble.hpp', 'R4300/x86/Assemble.hpp'),
    ('R4300/x86/gcop1_helpers.hpp', 'R4300/x86/Gcop1Helpers.hpp'),
    ('R4300/x86/regcache.hpp', 'R4300/x86/RegCache.hpp')
]
SOURCES2 = [
    ('alloc.cpp', 'Alloc.cpp'),
    ('cheats.cpp', 'Cheats.cpp'),
    ('Memory/pif_lut.cpp', 'Memory/PifLut.cpp'),
    ('Memory/dma.cpp', 'Memory/DMA.cpp'),
    ('Memory/flashram.cpp', 'Memory/Flashram.cpp'),
    ('Memory/memory.cpp', 'Memory/Memory.cpp'),
    ('Memory/pif.cpp', 'Memory/Pif.cpp'),
    ('Memory/savestates.cpp', 'Memory/Savestates.cpp'),
    ('Memory/summercart.cpp', 'Memory/Summercart.cpp'),
    ('Memory/tlb.cpp', 'Memory/TLB.cpp'),
    ('R4300/pure_interp.cpp', 'R4300/PureInterp.cpp'),
    ('R4300/cop0.cpp', 'R4300/Cop0.cpp'),
    ('R4300/cop1.cpp', 'R4300/Cop1.cpp'),
    ('R4300/cop1_d.cpp', 'R4300/Cop1D.cpp'),
    ('R4300/cop1_helpers.cpp', 'R4300/Cop1Helpers.cpp'),
    ('R4300/cop1_l.cpp', 'R4300/Cop1L.cpp'),
    ('R4300/cop1_s.cpp', 'R4300/Cop1S.cpp'),
    ('R4300/cop1_w.cpp', 'R4300/Cop1W.cpp'),
    ('R4300/disasm.cpp', 'R4300/Disasm.cpp'),
    ('R4300/exception.cpp', 'R4300/Exception.cpp'),
    ('R4300/interrupt.cpp', 'R4300/Interrupt.cpp'),
    ('R4300/r4300.cpp', 'R4300/R4300.cpp'),
    ('R4300/recomp.cpp', 'R4300/Recomp.cpp'),
    ('R4300/regimm.cpp', 'R4300/RegImm.cpp'),
    ('R4300/rom.cpp', 'R4300/Rom.cpp'),
    ('R4300/special.cpp', 'R4300/Special.cpp'),
    ('R4300/timers.cpp', 'R4300/Timers.cpp'),
    ('R4300/tracelog.cpp', 'R4300/Tracelog.cpp'),
    ('R4300/bc.cpp', 'R4300/BC.cpp'),
    ('R4300/vcr.cpp', 'R4300/VCR.cpp'),
    ("R4300/x86/assemble.cpp", "R4300/x86/Assemble.cpp"),
    ("R4300/x86/gbc.cpp", "R4300/x86/GBc.cpp"),
    ("R4300/x86/gcop0.cpp", "R4300/x86/GCop0.cpp"),
    ("R4300/x86/gcop1.cpp", "R4300/x86/GCop1.cpp"),
    ("R4300/x86/gcop1_d.cpp", "R4300/x86/GCop1D.cpp"),
    ("R4300/x86/gcop1_helpers.cpp", "R4300/x86/GCop1Helpers.cpp"),
    ("R4300/x86/gcop1_l.cpp", "R4300/x86/GCop1L.cpp"),
    ("R4300/x86/gcop1_s.cpp", "R4300/x86/GCop1S.cpp"),
    ("R4300/x86/gcop1_w.cpp", "R4300/x86/GCop1W.cpp"),
    ("R4300/x86/gr4300.cpp", "R4300/x86/GR4300.cpp"),
    ("R4300/x86/gregimm.cpp", "R4300/x86/GRegImm.cpp"),
    ("R4300/x86/gspecial.cpp", "R4300/x86/GSpecial.cpp"),
    ("R4300/x86/gtlb.cpp", "R4300/x86/GTLB.cpp"),
    ("R4300/x86/regcache.cpp", "R4300/x86/RegCache.cpp"),
    ("R4300/x86/rjump.cpp", "R4300/x86/RJump.cpp"),
]
HEADER_REPLS = [(
    re.compile(rf'#include ([<"]){re.escape(name)}([>"])', flags=re.I), 
    f"#include \\1{sub}\\2"
) for name, sub in HEADERS2]

with open(FOLDER / "CMakeLists.txt", "r", encoding="utf-8") as f:
    contents = f.read()

for old, new in itr.chain(HEADERS2, SOURCES2):
    contents = contents.replace(old, new)

with open(FOLDER / "CMakeLists.txt", "w", encoding="utf-8") as f:
    f.write(contents)


for path, dirnames, filenames in FOLDER.walk():
    for filename in filenames:
        entry = path / filename

        if (not entry.suffix in [".h", ".hpp", ".c", ".cpp"]):
            continue

        with open(entry, "r", encoding="utf-8") as f:
            contents = f.read()
        for pattern, repl in HEADER_REPLS:
            contents = pattern.sub(repl, contents)
        with open(entry, "w", encoding="utf-8") as f:
            f.write(contents)

for old, new in itr.chain(HEADERS2, SOURCES2):
    (FOLDER / old).rename(FOLDER / new)