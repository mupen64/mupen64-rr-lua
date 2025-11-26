/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Azimer, Bobby Smiles).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "common.h"

/*
 * Ultra64 data types for working with RCP stuff--useful for HLE
 */
#include "my_types.h"

/* Audio commands: ABI 1 */
/*
#define	A_SPNOOP				0
#define	A_ADPCM					1
#define	A_CLEARBUFF				2
#define	A_ENVMIXER				3
#define	A_LOADBUFF				4
#define	A_RESAMPLE				5
#define A_SAVEBUFF				6
#define A_SEGMENT				7
#define A_SETBUFF				8
#define A_SETVOL				9
#define A_DMEMMOVE              10
#define A_LOADADPCM             11
#define A_MIXER					12
#define A_INTERLEAVE            13
#define A_POLEF                 14
#define A_SETLOOP               15
*/
#define ACMD_SIZE 32
/*
 * Audio flags
 */

#define A_INIT 0x01
#define A_CONTINUE 0x00
#define A_LOOP 0x02
#define A_OUT 0x02
#define A_LEFT 0x02
#define A_RIGHT 0x00
#define A_VOL 0x04
#define A_RATE 0x00
#define A_AUX 0x08
#define A_NOAUX 0x00
#define A_MAIN 0x00
#define A_MIX 0x10

/*
 * The bit mask for each ABI command op-code seems to be:
 * (command & 0xFF00000000000000) >> 56 # w0[31..24], but actually the
 * repeating pattern seems to be that there are only 32 ABI commands.
 * Either doc is lazy, or the potential of 256 commands is a reserved thing.
 */
#define NUM_ABI_COMMANDS 32

/*
 * number of elements in SIMD processor
 * In the RSP's case, this is always 8 elements per vector.
 */
#define N 8

/*
 * Sometimes the audio HLE code has variables named "k0", after the MIPS GPR.
 *
 * With some compilers (like GCC with -masm=intel), this creates assembler
 * error messages because k0 conflicts with the assembly language syntax.
 *
 * Just a little bit of name-mangling should fix this collision.
 */
#define k0 GPR_k0

extern u32 t9, k0;

extern u8* DMEM;
extern u8* IMEM;
extern u8* DRAM;

extern u32 UCData, UDataLen;

void SaveSettings(void);
void LoadSettings(void);

void RspDump();

// ABI Functions
void ADDMIXER();
void ADPCM();
void ADPCM2();
void ADPCM3();
void CLEARBUFF();
void CLEARBUFF2();
void CLEARBUFF3();
void DMEMMOVE();
void DMEMMOVE2();
void DMEMMOVE3();
void DUPLICATE2();
void ENVMIXER();
void ENVMIXER2();
void ENVMIXER3();
void ENVMIXER_GE();
void ENVSETUP1();
void ENVSETUP2();
void FILTER2();
void HILOGAIN();
void INTERL2();
void INTERLEAVE();
void INTERLEAVE2();
void INTERLEAVE3();
void LOADADPCM();
void LOADADPCM2();
void LOADADPCM3();
void LOADBUFF();
void LOADBUFF2();
void LOADBUFF3();
void MIXER();
void MIXER2();
void MIXER3();
void MP3();
void MP3ADDY();
void POLEF();
void RESAMPLE();
void RESAMPLE2();
void RESAMPLE3();
void SAVEBUFF();
void SAVEBUFF2();
void SAVEBUFF3();
void SEGMENT();
void SEGMENT2();
void SETBUFF();
void SETBUFF2();
void SETLOOP();
void SETLOOP2();
void SETLOOP3();
void SETVOL();
void SETVOL3();
void SPNOOP();
void UNKNOWN();

// Buffer Space
extern u8 BufferSpace[0x10000];
extern short hleMixerWorkArea[256];
extern u32 SEGMENTS[0x10]; // 0x0320
extern u16 AudioInBuffer, AudioOutBuffer, AudioCount;
extern u16 AudioAuxA, AudioAuxC, AudioAuxE;
extern u32 loopval; // Value set by A_SETLOOP : Possible conflict with SETVOLUME???
extern bool isMKABI;
extern bool isZeldaABI;

/*
 * Each vector accumulator element is 48 bits, but that is a technical
 * advancement.  Only 32 bits at a time can be accessed during clamping.
 */
extern s32 acc[32][N];
extern s16 acc_clamped[N];

#include <emmintrin.h>

/*
 * RSP hardware has two types of saturated arithmetic:
 *     1.  signed clamping from 32- to 16-bit elements
 *     2.  unsigned clamping from 32- to 16-bit elements
 * (Note that no audio microcode for the RSP has ever been seen encountering
 * unsigned fractions or addends, so we only need to emulate signed clamps.)
 *
 * Accumulators are 48-bit, but only 32-bit-segment intervals at one time are
 * involved in the clamp.  The upper and lower bounds for signed and unsigned
 * clamping match the ANSI C language minimum requirements for the types
 * `short` and `unsigned short`:  -32768 <= x <= +32767 and 0 <= x <= +65535.
 *
 * A "slice" is an off-set portion of the accumulator:  high, middle, or low.
 */
#ifdef PREFER_MACRO_FUNCTIONS
#define sats_over(slice) (((slice) > +32767) ? +32767 : (slice))
#define sats_under(slice) (((slice) < -32768) ? -32768 : (slice))
#else
extern INLINE s32 sats_over(s32 slice);
extern INLINE s32 sats_under(s32 slice);
#endif

#ifdef PREFER_MACRO_FUNCTIONS
#define pack_signed(slice) sats_over(sats_under(slice))
#else
extern s16 pack_signed(s32 slice);
#endif

#ifdef PREFER_MACRO_FUNCTIONS
#define vsats128(vd, vs)            \
    {                               \
        vd[0] = pack_signed(vs[0]); \
        vd[1] = pack_signed(vs[1]); \
        vd[2] = pack_signed(vs[2]); \
        vd[3] = pack_signed(vs[3]); \
    }
#else
extern void vsats128(s16* vd, s32* vs); /* Clamp vectors using SSE2. */
#endif

/*
 * Basically, if we can prove that, on the C implementation in question:
 *     1.  Integers are negative if all bits are set (or simply the MSB).
 *     2.  Shifting a signed integer to the right shifts in the sign bit.
 * ... then we are able to easily staticize [un-]signed clamping functions.
 */
#if (~0 < 0)
#if ((~0 & ~1) >> 1 == ~0)
#define TWOS_COMPLEMENT_NEGATION
#endif
#endif

/*
 * There are two basic ways to copy an RSP vector to another RSP vector in
 * emulated memory:  with alignment and without alignment.  Forcing 128-bit
 * alignment requirements is a cycle or two faster on older CPUs, but on
 * modern hardware there is no reason to force memory alignment constrictions
 * or to use MOVDQA/MOVAPS and risk unaligned memory access seg. faults.
 */
extern void copy_vector(void* vd, const void* vs);

/*
 * Unfortunately, as much of the RSP analysis had to work around some early
 * byte order tricks in zilmar's RSP interpreter, several cases of audio HLE
 * will have extra endianness adjustments to them, sometimes redundantly.
 *
 * For example, reads and writes might frequently XOR each 16-bit address
 * when moving to and from a location, to maintain little-endian vectors.
 *
 * This function will act as a temporary solution for optimizing away most
 * of that scenario until the memory layout is improved more permanently in
 * later changes.
 */
extern void swap_elements(void* vd, const void* vs);
