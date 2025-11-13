#include <stdint.h>

volatile uint32_t *ptr = (uint32_t *)0x100; // 偏移地址 0x100
int main() {
    *ptr = 0x12345678;   // 向 DS:0x100 写入值
    uint32_t val = *ptr; // 从 DS:0x100 读值
    return 0;
}
