#include "Common.hpp"
#include "N64.hpp"
#include "Types.hpp"

u8 *DMEM;
u8 *IMEM;
u64 TMEM[512];
u8 *RDRAM;
u32 RDRAMSize = 0x800000;

N64Regs REG;
