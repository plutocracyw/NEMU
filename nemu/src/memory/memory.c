#include "common.h"
#include "memory/cache.h"
#include <string.h>
#include "cpu/reg.h"

extern CPU_state cpu;

#pragma GCC diagnostic ignored "-Wstrict-aliasing"


#ifndef CACHE_B
#define CACHE_B 64
#endif

#ifndef BURST_LEN
#define BURST_LEN 8
#endif

uint32_t dram_read(hwaddr_t, size_t);
void dram_write(hwaddr_t, size_t, uint32_t);
int read_cache_L1(hwaddr_t addr);
void write_cache_L1(hwaddr_t addr, size_t len, uint32_t data);

static inline lnaddr_t seg_translate(swaddr_t addr, size_t len, uint8_t sreg) {
	SegReg *seg = NULL;  

	switch (sreg) {
		case R_CS: seg = &cpu.CS; break;  
		case R_DS: seg = &cpu.DS; break;
		case R_ES: seg = &cpu.ES; break;
		case R_SS: seg = &cpu.SS; break;
		default: assert(0);
	}

#ifdef IA32_SEG
	if (cpu.CR0.PE) { // 
		assert(seg->limit >= addr + len - 1); // 简单段界限检查
		return seg->base + addr;
	} else { // 实模式
		return (seg->selector << 4) + addr;
	}
#else
	return addr;
#endif
}

uint32_t hwaddr_read(hwaddr_t addr, size_t len) {
	int index1 = read_cache_L1(addr);
	uint32_t block_bias = addr & (CACHE_B - 1);
	uint8_t ret[BURST_LEN << 1];

	if (block_bias + len > CACHE_B) {
		int index2 = read_cache_L1(addr + CACHE_B - block_bias);
		memcpy(ret, cache_L1[index1].data + block_bias, CACHE_B - block_bias);
		memcpy(ret + CACHE_B - block_bias, cache_L1[index2].data, len - (CACHE_B - block_bias));
	} else {
		memcpy(ret, cache_L1[index1].data + block_bias, len);
	}

	uint32_t result = unalign_rw(ret, 4) & (~0u >> ((4 - len) << 3));
	return result;
}

void hwaddr_write(hwaddr_t addr, size_t len, uint32_t data) {
	write_cache_L1(addr, len, data);
}

uint32_t lnaddr_read(lnaddr_t addr, size_t len) {
	return hwaddr_read(addr, len);
}

void lnaddr_write(lnaddr_t addr, size_t len, uint32_t data) {
	hwaddr_write(addr, len, data);
}

uint32_t swaddr_read(swaddr_t addr, size_t len, uint8_t sreg) {
#ifdef DEBUG
	assert(len == 1 || len == 2 || len == 4);
#endif
	lnaddr_t laddr = seg_translate(addr, len, sreg);
	return lnaddr_read(laddr, len);
}

void swaddr_write(swaddr_t addr, size_t len, uint8_t sreg, uint32_t data) {
#ifdef DEBUG
	assert(len == 1 || len == 2 || len == 4);
#endif
	lnaddr_t laddr = seg_translate(addr, len, sreg);
	lnaddr_write(laddr, len, data);
}

static inline uint32_t instr_fetch(swaddr_t addr, size_t len) {
	return swaddr_read(addr, len, R_CS);
}
