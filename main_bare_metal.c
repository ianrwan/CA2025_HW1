#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* ============= 裸機輸出宏（從老師範例複製） ============= */
#define printstr(ptr, length)                   \
    do {                                        \
        asm volatile(                           \
            "add a7, x0, 0x40;"                 \
            "add a0, x0, 0x1;" /* stdout */     \
            "add a1, x0, %0;"                   \
            "mv a2, %1;" /* length character */ \
            "ecall;"                            \
            :                                   \
            : "r"(ptr), "r"(length)             \
            : "a0", "a1", "a2", "a7");          \
    } while (0)

#define TEST_OUTPUT(msg, length) printstr(msg, length)

#define TEST_LOGGER(msg)                     \
    {                                        \
        char _msg[] = msg;                   \
        TEST_OUTPUT(_msg, sizeof(_msg) - 1); \
    }

/* ============= 整數輸出函數（從老師範例複製） ============= */
static void print_hex(unsigned long val)
{
    char buf[20];
    char *p = buf + sizeof(buf) - 1;
    *p = '\n';
    p--;

    if (val == 0) {
        *p = '0';
        p--;
    } else {
        while (val > 0) {
            int digit = val & 0xf;
            *p = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
            p--;
            val >>= 4;
        }
    }

    p++;
    printstr(p, (buf + sizeof(buf) - p));
}

static void print_dec(unsigned long val)
{
    char buf[20];
    char *p = buf + sizeof(buf) - 1;
    *p = '\n';
    p--;

    if (val == 0) {
        *p = '0';
        p--;
    } else {
        while (val > 0) {
            /* 使用軟體除法（因為只有 RV32I） */
            unsigned long digit = val % 10;
            *p = '0' + digit;
            p--;
            val = val / 10;
        }
    }

    p++;
    printstr(p, (buf + sizeof(buf) - p));
}

/* ============= 你的原始程式碼（保持不變） ============= */
typedef uint8_t uf8;

static const uint32_t uf8_lower_bound[16] = 
{
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

/* ============= 修改後的測試函數（移除 printf） ============= */
/* Test encode/decode round-trip */
static bool test(void)
{
    int32_t previous_value = -1;
    bool passed = true;
    char error_msg[100];
    char hex_buf[10];

    for (int i = 0; i < 256; i++) {
        uint8_t fl = i;
        int32_t value = uf8_decode(fl);
        uint8_t fl2 = uf8_encode_optimized(value);

        if (fl != fl2) {
            TEST_LOGGER("Error: ");
            print_hex(fl);
            TEST_LOGGER(" produces value ");
            print_dec(value);
            TEST_LOGGER(" but encodes back to ");
            print_hex(fl2);
            TEST_LOGGER("\n");
            passed = false;
        }

        if (value <= previous_value) {
            TEST_LOGGER("Error: ");
            print_hex(fl);
            TEST_LOGGER(" value ");
            print_dec(value);
            TEST_LOGGER(" <= previous_value ");
            print_dec(previous_value);
            TEST_LOGGER("\n");
            passed = false;
        }

        previous_value = value;
    }

    return passed;
}

/* ============= 效能計數器函數宣告 ============= */
extern uint64_t get_cycles(void);
extern uint64_t get_instret(void);

/* ============= 主函數（裸機版本） ============= */
int main(void)
{
    uint64_t start_cycles, end_cycles, cycles_elapsed;
    uint64_t start_instret, end_instret, instret_elapsed;
    
    TEST_LOGGER("=== UF8 Encode/Decode Tests ===\n");
    
    start_cycles = get_cycles();
    start_instret = get_instret();
    
    if (test()) {
        TEST_LOGGER("All tests passed.\n");
    } else {
        TEST_LOGGER("Some tests failed.\n");
    }
    
    end_cycles = get_cycles();
    end_instret = get_instret();
    cycles_elapsed = end_cycles - start_cycles;
    instret_elapsed = end_instret - start_instret;
    
    TEST_LOGGER("Cycles: ");
    print_dec((unsigned long) cycles_elapsed);
    TEST_LOGGER("Instructions: ");
    print_dec((unsigned long) instret_elapsed);
    TEST_LOGGER("\n");
    
    return 0;
}