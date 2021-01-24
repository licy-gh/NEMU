#include "common.h"
#include "memory/cache.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "burst.h"

void ddr3_read_replace(hwaddr_t addr, void *data);
void dram_write(hwaddr_t addr, size_t len, uint32_t data);
void ddr3_write_replace(hwaddr_t addr, void *data, uint8_t *mask);

void init_cache()
{
    int i;
    for (i = 0; i < Cache_Size_L1 / Cache_Block_Size_L1; i++)
    {
        cache1[i].valid = 0;
    }
    for (i = 0; i < Cache_Size_L2 / Cache_Block_Size_L2; i++)
    {
        cache2[i].valid = 0;
        cache2[i].dirty = 0;
    }
#ifdef CNTFLAG
    timeCost = 0;
#endif
}
int read_cache1(hwaddr_t addr)
{
    uint32_t group_idx = (addr >> Cache_Block_Bit_L1) & (Cache_Set_Size_L1 - 1);
    uint32_t tag = (addr >> (Cache_Set_Bit_L1 + Cache_Block_Bit_L1));
    // uint32_t block_start = (addr >> Cache_Block_Bit_L1) << Cache_Block_Bit_L1;

    int i, group = group_idx * Cache_Line_Size_L1;
    for (i = group + 0; i < group + Cache_Line_Size_L1; i++)
    {
        if (cache1[i].valid == 1 && cache1[i].tag == tag)
        { // READ HIT

#ifdef CNTFLAG
            timeCost += 2; //HIT in Cache1
#endif
            return i;
        }
    }

    // Find in Cache2
    int pl = read_cache2(addr);
    srand(time(0));
    i = group + rand() % Cache_Line_Size_L1;
    memcpy(cache1[i].data, cache2[pl].data, Cache_Block_Size_L1);
    cache1[i].valid = 1;
    cache1[i].tag = tag;
    return i;
}

int read_cache2(hwaddr_t addr)
{
    uint32_t group_idx = (addr >> Cache_Block_Bit_L2) & (Cache_Set_Size_L2 - 1);
    uint32_t tag = (addr >> (Cache_Set_Bit_L2 + Cache_Block_Bit_L2));
    uint32_t block_start = (addr >> Cache_Block_Bit_L2) << Cache_Block_Bit_L2;

    int i, group = group_idx * Cache_Line_Size_L2;
    for (i = group + 0; i < group + Cache_Line_Size_L2; i++)
    {
        if (cache2[i].valid == 1 && cache2[i].tag == tag)
        { // READ HIT

#ifdef CNTFLAG
            timeCost += 20; //HIT in Cache2
#endif
            return i;
        }
    }
    // Random (PA3 optional task1)
#ifdef CNTFLAG
    timeCost += 200; //NOT HIT in Cache2
#endif
    srand(time(0));
    i = group + rand() % Cache_Line_Size_L2;
    /*write back*/
    if (cache2[i].valid == 1 && cache2[i].dirty == 1)
    {
        uint8_t ret[BURST_LEN << 1];
        uint32_t block_st = (cache2[i].tag << (Cache_Set_Bit_L2 + Cache_Block_Bit_L2)) | (group_idx << Cache_Block_Bit_L2);
        int w;
        memset(ret, 1, sizeof ret);
        for (w = 0; w < Cache_Block_Size_L2 / BURST_LEN; w++)
        {
            ddr3_write_replace(block_st + BURST_LEN * w, cache2[i].data + BURST_LEN * w, ret);
        }
    }
    /*new content*/
    int j;
    for (j = 0; j < Cache_Block_Size_L2 / BURST_LEN; j++)
    {
        ddr3_read_replace(block_start + BURST_LEN * j, cache2[i].data + BURST_LEN * j);
    }
    cache2[i].dirty = 0;
    cache2[i].valid = 1;
    cache2[i].tag = tag;
    return i;
}

void write_cache1(hwaddr_t addr, size_t len, uint32_t data)
{
    uint32_t group_idx = (addr >> Cache_Block_Bit_L1) & (Cache_Set_Size_L1 - 1);
    uint32_t tag = (addr >> (Cache_Set_Bit_L1 + Cache_Block_Bit_L1));
    uint32_t offset = addr & (Cache_Block_Size_L1 - 1);

    int i, group = group_idx * Cache_Line_Size_L1;
    for (i = group + 0; i < group + Cache_Line_Size_L1; i++)
    {
        if (cache1[i].valid == 1 && cache1[i].tag == tag)
        { // WRITE HIT
            /*write through*/
            if (offset + len > Cache_Block_Size_L1)
            {
                dram_write(addr, Cache_Block_Size_L1 - offset, data);
                memcpy(cache1[i].data + offset, &data, Cache_Block_Size_L1 - offset);
                /*Update Cache2*/
                write_cache2(addr, Cache_Block_Size_L1 - offset, data);

                write_cache1(addr + Cache_Block_Size_L1 - offset, len - (Cache_Block_Size_L1 - offset), data >> (8 * (Cache_Block_Size_L1 - offset)));
            }
            else
            {
                dram_write(addr, len, data);
                memcpy(cache1[i].data + offset, &data, len);
                /*Update Cache2*/
                write_cache2(addr, len, data);
            }
            return;
        }
    }
    write_cache2(addr, len, data);
}

void write_cache2(hwaddr_t addr, size_t len, uint32_t data)
{
    uint32_t group_idx = (addr >> Cache_Block_Bit_L2) & (Cache_Set_Size_L2 - 1);
    uint32_t tag = (addr >> (Cache_Set_Bit_L2 + Cache_Block_Bit_L2));
    uint32_t offset = addr & (Cache_Block_Size_L2 - 1);

    int i, group = group_idx * Cache_Line_Size_L2;
    for (i = group + 0; i < group + Cache_Line_Size_L2; i++)
    {
        if (cache2[i].valid == 1 && cache2[i].tag == tag)
        { // WRITE HIT
            cache2[i].dirty = 1;
            if (offset + len > Cache_Block_Size_L2)
            {
                memcpy(cache2[i].data + offset, &data, Cache_Block_Size_L2 - offset);
                write_cache2(addr + Cache_Block_Size_L2 - offset, len - (Cache_Block_Size_L2 - offset), data >> (8 * (Cache_Block_Size_L2 - offset)));
            }
            else
            {
                memcpy(cache2[i].data + offset, &data, len);
            }
            return;
        }
    }
    /*write allocate*/
    i = read_cache2(addr);
    cache2[i].dirty = 1;
    memcpy(cache2[i].data + offset, &data, len);
}