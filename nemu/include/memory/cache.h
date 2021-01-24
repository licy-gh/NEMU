#ifndef __CACHE_H__
#define __CACHE_H__

#include "common.h"

#define Cache_Size_L1 64 * 1024
#define Cache_Block_Size_L1 64
#define Cache_Line_Bit_L1 3
#define Cache_Set_Bit_L1 7
#define Cache_Block_Bit_L1 6
#define Cache_Set_Size_L1 (1 << Cache_Set_Bit_L1)
#define Cache_Line_Size_L1 (1 << Cache_Line_Bit_L1)

#define CNTFLAG // 控制是否开启计时器

typedef struct
{
    uint8_t data[Cache_Block_Size_L1]; // 64B
    uint32_t tag;
    bool valid;
} Cache_L1;

Cache_L1 cache1[Cache_Size_L1 / Cache_Block_Size_L1];

#define Cache_Size_L2 4 * 1024 * 1024
#define Cache_Block_Size_L2 64
#define Cache_Line_Bit_L2 4
#define Cache_Set_Bit_L2 12
#define Cache_Block_Bit_L2 6
#define Cache_Set_Size_L2 (1 << Cache_Set_Bit_L2)
#define Cache_Line_Size_L2 (1 << Cache_Line_Bit_L2)

typedef struct
{
    uint8_t data[Cache_Block_Size_L2]; // 64B
    uint32_t tag;
    bool valid;
    bool dirty;
} Cache_L2;

Cache_L2 cache2[Cache_Size_L2 / Cache_Block_Size_L2];

#ifdef CNTFLAG
int timeCost;
#endif

void init_cache();
int read_cache1(hwaddr_t);
void write_cache1(hwaddr_t, size_t, uint32_t);
void write_cache2(hwaddr_t, size_t, uint32_t);
int read_cache2(hwaddr_t);
#endif
