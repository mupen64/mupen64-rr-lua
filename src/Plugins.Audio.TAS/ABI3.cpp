/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Azimer, Bobby Smiles).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "audiohle.h"

/*
FFT = Fast Fourier Transform
DCT = Discrete Cosine Transform
MPEG-1 Layer 3 retains Layer 2�s 1152-sample window, as well as the FFT polyphase filter for
backward compatibility, but adds a modified DCT filter. DCT�s advantages over DFTs (discrete
Fourier transforms) include half as many multiply-accumulate operations and half the
generated coefficients because the sinusoidal portion of the calculation is absent, and DCT
generally involves simpler math. The finite lengths of a conventional DCTs� bandpass impulse
responses, however, may result in block-boundary effects. MDCTs overlap the analysis blocks
and lowpass-filter the decoded audio to remove aliases, eliminating these effects. MDCTs also
have a higher transform coding gain than the standard DCT, and their basic functions
correspond to better bandpass response.

MPEG-1 Layer 3�s DCT sub-bands are unequally sized, and correspond to the human auditory
system�s critical bands. In Layer 3 decoders must support both constant- and variable-bit-rate
bit streams. (However, many Layer 1 and 2 decoders also handle variable bit rates). Finally,
Layer 3 encoders Huffman-code the quantized coefficients before archiving or transmission for
additional lossless compression. Bit streams range from 32 to 320 kbps, and 128-kbps rates
achieve near-CD quality, an important specification to enable dual-channel ISDN
(integrated-services-digital-network) to be the future high-bandwidth pipe to the home.

*/

// Disables the command because it's not used?
static void DISABLE()
{
}
static void WHATISTHIS()
{
}

p_func ABI3[NUM_ABI_COMMANDS] = {
DISABLE,
ADPCM3,
CLEARBUFF3,
ENVMIXER3,
LOADBUFF3,
RESAMPLE3,
SAVEBUFF3,
MP3,
MP3ADDY,
SETVOL3,
DMEMMOVE3,
LOADADPCM3,
MIXER3,
INTERLEAVE3,
WHATISTHIS,
SETLOOP3,
SPNOOP,
SPNOOP,
SPNOOP,
SPNOOP,
SPNOOP,
SPNOOP,
SPNOOP,
SPNOOP,
SPNOOP,
SPNOOP,
SPNOOP,
SPNOOP,
SPNOOP,
SPNOOP,
SPNOOP,
SPNOOP,
};
