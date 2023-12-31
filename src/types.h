#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int16_t int16;
typedef int8_t int8;
typedef union val16 {
    struct {
        u8 lsb;
        u8 msb;
    };
    u16 val;
} val16;

static inline u8 u16_lsb(u16 *val) { return ((u8 *)val)[0]; };
static inline u8 u16_msb(u16 *val) { return ((u8 *)val)[1]; };
static inline void bit_set(u8 *number, u8 n) { *number |= (1 << n); }
static inline void bit_clear(u8 *number, u8 n) { *number &= ~(1 << n); }
static inline bool bit_read(u8 val, u8 idx) { return (val >> idx) & 1; }
static inline void bit_write(u8 *val, u8 idx, bool bit_val) {
    if (bit_val)
        bit_set(val, idx);
    else
        bit_clear(val, idx);
}

#endif
