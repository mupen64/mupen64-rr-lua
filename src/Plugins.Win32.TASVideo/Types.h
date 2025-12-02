#ifndef TYPES_H
#define TYPES_H

using u8 = uint8_t; /* unsigned  8-bit */
using u16 = uint16_t; /* unsigned 16-bit */
using u32 = uint32_t; /* unsigned 32-bit */
using u64 = uint64_t; /* unsigned 64-bit */

using s8 = int8_t; /* signed  8-bit */
using s16 = int16_t; /* signed 16-bit */
using s32 = int32_t; /* signed 32-bit */
using s64 = int64_t; /* signed 64-bit */

using vu8 = volatile uint8_t; /* unsigned  8-bit */
using vu16 = volatile uint16_t; /* unsigned 16-bit */
using vu32 = volatile uint32_t; /* unsigned 32-bit */
using vu64 = volatile uint64_t; /* unsigned 64-bit */

using vs8 = volatile int8_t; /* signed  8-bit */
using vs16 = volatile int16_t; /* signed 16-bit */
using vs32 = volatile int32_t; /* signed 32-bit */
using vs64 = volatile int64_t; /* signed 64-bit */

using f32 = float; /* single prec floating point */
using f64 = double; /* double prec floating point */

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

#endif TYPES_H
