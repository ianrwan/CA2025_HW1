#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef uint8_t uf8;

static const uint32_t uf8_lower_bound[16] = 
{
    // 每個指數 e 對應的最小值 = (2^e - 1) * 16
    0,      // e=0: (1-1)*16 = 0
    16,     // e=1: (2-1)*16 = 16
    48,     // e=2: (4-1)*16 = 48
    112,    // e=3: (8-1)*16 = 112
    240,    // e=4: (16-1)*16 = 240
    496,    // e=5: (32-1)*16 = 496
    1008,   // e=6: (64-1)*16 = 1008
    2032,   // e=7: (128-1)*16 = 2032
    4080,   // e=8: (256-1)*16 = 4080
    8176,   // e=9: (512-1)*16 = 8176
    16368,  // e=10: (1024-1)*16 = 16368
    32752,  // e=11: (2048-1)*16 = 32752
    65520,  // e=12: (4096-1)*16 = 65520
    131056, // e=13: (8192-1)*16 = 131056
    262128, // e=14: (16384-1)*16 = 262128
    524272  // e=15: (32768-1)*16 = 524272
};

static inline unsigned clz(uint32_t x)
{
    int n = 32, c = 16;
    do {
        uint32_t y = x >> c;
        if (y) {
            n -= c;
            x = y;
        }
        c >>= 1;
    } while (c);
    return n - x;
}

/* Decode uf8 to uint32_t */
uint32_t uf8_decode(uf8 fl)
{
    uint32_t mantissa = fl & 0x0f;
    uint8_t exponent = fl >> 4;
    uint32_t offset = (0x7FFF >> (15 - exponent)) << 4;
    return (mantissa << exponent) + offset;
}

/* Encode uint32_t to uf8 */
uf8 uf8_encode_optimized(uint32_t value)
{
    if (value < 16)
        return value;
    
    uint8_t low = 1;
    uint8_t high = 15;
    uint8_t exponent = 0;
    
    while (low <= high) {
        uint8_t mid = (low + high) / 2;
        
        if (value >= uf8_lower_bound[mid])
        {
            exponent = mid;      
            low = mid + 1;       

        } else 
        {
            high = mid - 1;      
        }
    }
    
    uint32_t offset = uf8_lower_bound[exponent];

    uint8_t mantissa = (value - offset) >> exponent;
    
    return (exponent << 4) | mantissa;
}

/* Test encode/decode round-trip */
static bool test(void)
{
    int32_t previous_value = -1;
    bool passed = true;

    for (int i = 0; i < 256; i++) {
        uint8_t fl = i;
        int32_t value = uf8_decode(fl);
        uint8_t fl2 = uf8_encode_optimized(value);

        if (fl != fl2) {
            printf("%02x: produces value %d but encodes back to %02x\n", fl,
                   value, fl2);
            passed = false;
        }

        if (value <= previous_value) {
            printf("%02x: value %d <= previous_value %d\n", fl, value,
                   previous_value);
            passed = false;
        }

        previous_value = value;
    }

    return passed;
}

int main(void)
{
    if (test()) {
        printf("All tests passed.\n");
        return 0;
    }
    return 1;
}
