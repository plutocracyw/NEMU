#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include "common.h"
#include "memory/cache.h"
#include "memory/memory.h"

int read_cache_L1(hwaddr_t addr);
void write_cache_L1(hwaddr_t addr, size_t len, uint32_t data);
int read_cache_L2(hwaddr_t addr);
void write_cache_L2(hwaddr_t addr, size_t len, uint32_t data);

#ifndef BURST_LEN
#define BURST_LEN 8
#endif

//初始化
void init_cache(){
    int i;
    for(i=0;i<CACHE_L1_S*CACHE_L1_E;i++){
        cache_L1[i].valid_bit=false;
    }
    for(i=0;i<CACHE_L2_S*CACHE_L2_E;i++){
        cache_L2[i].dirty_val=false;
        cache_L2[i].valid_bit=false;
    }
    //清空有效位
    return;
}

void ddr3_read_me(hwaddr_t addr, void* data);
 
int read_cache_L1(hwaddr_t addr) {
    uint32_t set = ((addr >> CACHE_b) & (CACHE_L1_S - 1)); //组索引参数
    uint32_t tag = (addr >> (CACHE_b + CACHE_L1_s)); //标签参数

    int block_i;
    int set_begin = set * CACHE_L1_E;
    int set_end = (set + 1) * CACHE_L1_E;

    for (block_i = set_begin; block_i < set_end; block_i++)
        if (cache_L1[block_i].valid_bit && cache_L1[block_i].tag == tag) // Hit!
        return block_i;

    srand(time(0));
    int block_i_L2 = read_cache_L2(addr);//在二级高速缓存中查找

    for (block_i = set_begin; block_i < set_end; block_i++)
        if (!cache_L1[block_i].valid_bit)
            break;

    if (block_i == set_end)
        block_i = set_begin + rand() % CACHE_L1_E;

    memcpy(cache_L1[block_i].data, cache_L2[block_i_L2].data, CACHE_B);
    
    cache_L1[block_i].valid_bit = true;
    cache_L1[block_i].tag = tag;
    return block_i;
}

void ddr3_write_me(hwaddr_t addr, void* data, uint8_t* mask);
 
int read_cache_L2(hwaddr_t addr) {
    uint32_t set = ((addr >> CACHE_b) & (CACHE_L2_S - 1));
    uint32_t tag = (addr >> (CACHE_b + CACHE_L2_s));

    uint32_t block_start = ((addr >> CACHE_b) << CACHE_b);
    int block_i;
    int set_begin = set * CACHE_L2_E;
    int set_end = (set + 1) * CACHE_L2_E;
    for (block_i = set_begin; block_i < set_end; block_i++)
        if (cache_L2[block_i].valid_bit && cache_L2[block_i].tag == tag) 
            return block_i; 

    srand(time(0));
    for (block_i = set_begin; block_i < set_end; block_i++)
        if (!cache_L2[block_i].valid_bit)
            break;

    if (block_i == set_end)
        block_i = set_begin + rand() % CACHE_L2_E;

    int i;
    if (cache_L2[block_i].valid_bit && cache_L2[block_i].dirty_val) {
        uint8_t tmp[BURST_LEN << 1];
        memset(tmp, 1, sizeof(tmp));
        uint32_t block_start_x = (cache_L2[block_i].tag << (CACHE_L2_s + CACHE_b)) | (set << CACHE_b);

        for (i = 0; i < CACHE_B / BURST_LEN; i++) {
            ddr3_write_me(block_start_x + BURST_LEN * i, cache_L2[block_i].data + BURST_LEN * i, tmp);
        }
    }

    for (i = 0; i < CACHE_B / BURST_LEN; i++) {
        ddr3_read_me(block_start + BURST_LEN * i, cache_L2[block_i].data + BURST_LEN * i);
    }

    cache_L2[block_i].valid_bit = true;
    cache_L2[block_i].dirty_val = false;
    cache_L2[block_i].tag = tag;

    return block_i;
}

void dram_write(hwaddr_t addr, size_t len, uint32_t data);
 
void write_cache_L1(hwaddr_t addr, size_t len, uint32_t data) {
    uint32_t set = ((addr >> CACHE_b) & (CACHE_L1_S - 1));
    uint32_t tag = (addr >> (CACHE_b + CACHE_L1_s));
    uint32_t block_bias = addr & (CACHE_B - 1);

    int block_i;
    int set_begin = set * CACHE_L1_E;
    int set_end = (set + 1) * CACHE_L1_E;

    for (block_i = set_begin; block_i < set_end; block_i++) {
        if (cache_L1[block_i].valid_bit && cache_L1[block_i].tag == tag) {
        if (block_bias + len > CACHE_B) {
            dram_write(addr, CACHE_B - block_bias, data);
            memcpy(cache_L1[block_i].data + block_bias, &data, CACHE_B - block_bias);

            write_cache_L2(addr, CACHE_B - block_bias, data);
            write_cache_L1(addr + CACHE_B - block_bias, len - (CACHE_B - block_bias), data >> (CACHE_B - block_bias));
        } 
        else {
            dram_write(addr, len, data);
            memcpy(cache_L1[block_i].data + block_bias, &data, len);
            write_cache_L2(addr, len, data);
        }
        return;
        }
    }

    write_cache_L2(addr, len, data);
    return;
}
 
void write_cache_L2(hwaddr_t addr, size_t len, uint32_t data) {
    uint32_t set = ((addr >> CACHE_b) & (CACHE_L2_S - 1));
    uint32_t tag = (addr >> (CACHE_b + CACHE_L2_s));
    uint32_t block_bias = addr & (CACHE_B - 1);

    int block_i;
    int set_begin = set * CACHE_L2_E;
    int set_end = (set + 1) * CACHE_L2_E;

    for (block_i = set_begin; block_i < set_end; block_i++) {
        if (cache_L2[block_i].valid_bit && cache_L2[block_i].tag == tag) {
            cache_L2[block_i].dirty_val = true;

        if (block_bias + len > CACHE_B) {
            memcpy(cache_L2[block_i].data + block_bias, &data, CACHE_B - block_bias);
            write_cache_L2(addr + CACHE_B - block_bias, len - (CACHE_B - block_bias), data >> (CACHE_B - block_bias));
        } 
        else {
            memcpy(cache_L2[block_i].data + block_bias, &data, len);
        }
        return;
        }
    }

    block_i = read_cache_L2(addr);
    cache_L2[block_i].dirty_val = true;
    memcpy(cache_L2[block_i].data + block_bias, &data, len);
    return;
}