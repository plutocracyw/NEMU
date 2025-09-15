#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>
#include <string.h>

#include "cpu/reg.h"

uint32_t vaddr_read(uint32_t addr, int len);

enum {
  NOTYPE = 256, EQ, NUM, HEX, REG, NEQ, AND, OR, NEG, DEREF,NOT,
  PLUS = '+', MINUS = '-', MUL = '*', DIV = '/', LPAREN = '(', RPAREN = ')'
};

static struct rule {
	char *regex;
	int token_type;
} 

rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */

	{" +",   NOTYPE},       // 空格
	{"\\+",   PLUS},         // +(转义字符)
	{"-",     MINUS},        // -
	{"\\*",   MUL},          // *
	{"/",     DIV},          // /
	{"\\(",   LPAREN},       // (
	{"\\)",   RPAREN},       // )
	{"==",    EQ},           // ==
	{"!=",    NEQ},          // !=
	{"&&",    AND},          // &&
	{"\\|\\|", OR},          // ||
	{"0[xX][0-9a-fA-F]+", HEX}, // 十六进制数
	{"[0-9]+", NUM},         // 十进制数
	{"\\$[a-zA-Z]+", REG},   // 寄存器
	{"!", NOT},              // 逻辑非


};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */

 //定义类型token
typedef struct token {
	int type;
	char str[32];
} Token;

Token tokens[1024];
int nr_token;

static bool make_token(char *e);
static void mark_unary_operators(void);
static int precedence(int type);
static bool is_binary_op_token(int type);
static int check_parentheses(int p,int q);
static int dominant_op(int p,int q);
static uint32_t eval(int p,int q,bool *success);
uint32_t expr(char *e,bool *success);
uint32_t reg_str2val(const char *s, bool *success);

//初始化正则表达式数组，为后续的词法分析做准备。
void init_regex() {
	char error_msg[128];
	int ret;
	int i;
	for(i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if(ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			Assert(ret == 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

/* 标记一元运算符（NEG, DEREF） */
static void mark_unary_operators(void) {
    int i;
    for (i = 0; i < nr_token; i++) {
        if (tokens[i].type == MINUS) {
            if (i == 0) {
                tokens[i].type = NEG;
            } else {
                int t = tokens[i - 1].type;
                if (t != NUM && t != HEX && t != REG && t != RPAREN) {
                    tokens[i].type = NEG;
                }
            }
        } else if (tokens[i].type == MUL) {
            if (i == 0) {
                tokens[i].type = DEREF;
            } else {
                int t = tokens[i - 1].type;
                if (t != NUM && t != HEX && t != REG && t != RPAREN) {
                    tokens[i].type = DEREF;
                }
            }
        }
    }
}



//词义分析
static bool make_token(char *e) {
    int position = 0;         /* 当前扫描位置 */
    int i;
    regmatch_t pmatch;     
	
    nr_token = 0;             /* 已生成 token 的数量 */

    while (e[position] != '\0') {
        int matched = 0;
        /* Try all rules one by one. */
        for (i = 0; i < NR_REGEX; i++) {
            if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 
                && pmatch.rm_so == 0) {

                int substr_len = pmatch.rm_eo;  /* 匹配到的子串长度 */
                char *substr_start = e + position;
                int token_type = rules[i].token_type;

                Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
                    i, rules[i].regex, position, substr_len, substr_len, substr_start);

                position += substr_len;

                /* 跳过空格 */
                if (token_type == NOTYPE) {
                    matched = 1;
                    break;
                }

                if (nr_token >= (int)(sizeof(tokens) / sizeof(tokens[0]))) {
                    printf("Error: too many tokens (nr_token >= %lu)\n",
                           (unsigned long)(sizeof(tokens) / sizeof(tokens[0])));
                    return false;
                }

                if (token_type == NUM || token_type == HEX || token_type == REG) {
                    size_t copy_len = (size_t)substr_len;
                    if (copy_len >= sizeof(tokens[nr_token].str)) {
                        copy_len = sizeof(tokens[nr_token].str) - 1;
                    }
                    strncpy(tokens[nr_token].str, substr_start, copy_len);
                    tokens[nr_token].str[copy_len] = '\0';
                } else {
                    tokens[nr_token].str[0] = '\0'; /* 非字符串类 token 清空 str */
                }

                tokens[nr_token].type = token_type;
                nr_token++;
                matched = 1;
                break;
            }
        }

        /* 匹配失败 */
        if (!matched) {  
            printf("no match at position %d\n%s\n%*.s^\n",
                   position, e, position, "");
            return false;
        }
    }

	
    return true; 
}


static bool is_binary_op_token(int type){
	switch(type){
		case PLUS:
		case MINUS:
		case MUL:
		case DIV:
		case EQ:
		case NEQ:
		case AND:
		case OR:
			return true;
		default:
			return false;
	}
}



//确定运算顺序
static int precedence(int type)	{
		switch(type){
			case OR : return 1;
			case AND : return 2;
			case EQ : return 3;
			case NEQ : return 3;
			case PLUS : return 4;
			case MINUS : return 4;
			case MUL: return 5;
			case DIV: return 5;
    		case NEG: return 6;
			case DEREF: return 6;
			case NOT: return 6;
    		
			default: return 7;
		}
}


/* 检查 p 和 q 的 token 是否是包裹整个子表达式的一对括号 */
static int check_parentheses(int p, int q) {
    if (tokens[p].type != LPAREN || tokens[q].type != RPAREN) {
        return 0;
    }

    int balance = 0;
    int i;
    for (i = p + 1; i < q; i++) {
        if (tokens[i].type == LPAREN) {
            balance++;
        } else if (tokens[i].type == RPAREN) {
            if (balance == 0) {
                return 0;  /* 提前闭合了，不是整体包裹 */
            }
            balance--;
        }
    }
    return (balance == 0);
}

/* 找出 p..q 范围内的主导运算符 */
static int dominant_op(int p, int q) {
    int op = -1;
    int min_pri = 100;
    int balance = 0;
    int i;

    for (i = p; i <= q; i++) {
        int type = tokens[i].type;

        if (type == LPAREN) {
            balance++;
            continue;
        }
        if (type == RPAREN) {
            balance--;
            continue;
        }
        if (balance > 0) {
            continue;
        }

        if (is_binary_op_token(type)) {
            int pri = precedence(type);
            if (pri <= min_pri) {
                min_pri = pri;
                op = i;
            }
        }
    }

    return op;
}


uint32_t reg_str2val(const char *s, bool *success) {
    *success = true;

    if (strcasecmp(s, "$eax") == 0) return cpu.eax;
    if (strcasecmp(s, "$ebx") == 0) return cpu.ebx;
    if (strcasecmp(s, "$ecx") == 0) return cpu.ecx;
    if (strcasecmp(s, "$edx") == 0) return cpu.edx;
    if (strcasecmp(s, "$esp") == 0) return cpu.esp;
    if (strcasecmp(s, "$ebp") == 0) return cpu.ebp;
    if (strcasecmp(s, "$esi") == 0) return cpu.esi;
    if (strcasecmp(s, "$edi") == 0) return cpu.edi;
    if (strcasecmp(s, "$eip") == 0) return cpu.eip;

    *success = false;
    return 0;
}

uint32_t vaddr_read(uint32_t addr, int len) {
    printf("vaddr_read(0x%x, %d)\n", addr, len);
    return addr;  // 暂时直接返回地址本身
}



static uint32_t eval(int p,int q,bool *success){
	uint32_t val1,val2,addr,tmp;
	int op;
	/* TODO: Insert codes to evaluate the expression. */
	if(p>q)
	{
		*success=false;
		return 0;
	}


	//区间只有一个token
	if(p==q){
		*success=true;
		switch(tokens[p].type){
			case NUM:
				return (uint32_t)strtoul(tokens[p].str,NULL,10);
			case HEX:
			{
				return (uint32_t)strtoul(tokens[p].str,NULL,16);
			}
			case REG:
			{
				return reg_str2val(tokens[p].str,success);
			}
			default:
				*success=false;
				return 0;
			
		}
	}

	if(check_parentheses(p,q)) {
		return eval(p+1,q-1,success);
	}

	
	op=dominant_op(p,q);

	if(op!=-1){

		val1=eval(p,op-1,success);
		if(!*success) return 0;
		val2=eval(op+1,q,success);
		if(!*success) return 0;

		switch(tokens[op].type){
			case PLUS: return val1+val2;
			case MINUS: return val1-val2;
			case MUL: return val1*val2;
			case DIV: Assert(val2 != 0, "Divide by zero"); return val1/val2;
			case EQ: return val1==val2;
			case NEQ: return val1!=val2;
			case AND: return val1&&val2;
			case OR:  return val1||val2;
			default: Assert(0, "Unknown operator %d at position %d", tokens[op].type,op);
		}
	}
	else{
		if(tokens[p].type==NEG){
			tmp=eval(p+1,q,success);
			if(!*success) return 0;
			return (uint32_t)(-(int32_t)tmp);
		}
		else if(tokens[p].type==DEREF){
			addr=eval(p+1,q,success);
			if(!*success) return 0;
			return vaddr_read(addr,4);
		}
		else if (tokens[p].type == NOT) {
			tmp = eval(p+1,q,success);
			if(!*success) return 0;
			return (tmp == 0 ? 1 : 0);
		}
		else{
			*success=false;
			printf("Error: no dominant operator found in p=%d q=%d\n", p, q);
			return 0;
		}
	}

		*success=false;
		return 0;
}

//将字符串转换为可解析的 token 数组
uint32_t expr(char *e,bool *success){
    if(!make_token(e))
    {
        *success=false;
        return 0;
    }

    mark_unary_operators();
	return eval(0,nr_token-1,success);
}