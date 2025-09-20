#include "cpu/exec/helper.h"
#include "add.h"

// 生成 16 位版本
#define DATA_BYTE 2
#include "add-template.h"
#undef DATA_BYTE

// 生成 32 位版本
#define DATA_BYTE 4
#include "add-template.h"
#undef DATA_BYTE

// 额外可以生成通用 _v 版本
make_helper_v(add_i2a)
make_helper_v(add_i2rm)
make_helper_v(add_si2rm)
make_helper_v(add_r2rm)
make_helper_v(add_rm2r)
