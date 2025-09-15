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
  NOTYPE = 256, EQ, NUM, HEX, REG, NEQ, AND, OR, NEG, DEREF,  
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
static void mark_unary_operators();
static int precedence(int type);
static bool is_binary_op_token(int type);
static bool check_parentheses(int p,int q);
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
	int position = 0;         //索引位置
	int i;
	regmatch_t pmatch;     
	
	nr_token = 0;             //已生成 token 的数量

	while(e[position] != '\0') {
		int matched = 0;
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0){
				int substr_len=pmatch.rm_eo;

				char *substr_start = e + position;
				int token_type = rules[i].token_type;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);

				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array `tokens'. For certain types
				 * of tokens, some extra actions should be performed.
				 */

				//token生成逻辑

				if (token_type != NOTYPE) {
					if (nr_token >= (int)(sizeof(tokens) / sizeof(tokens[0]))) {
						printf("Error: too many tokens (nr_token >= %zu)\n", sizeof(tokens) / sizeof(tokens[0]));
						return false;
					}
				}
				
				if(token_type == NUM || token_type == HEX || token_type == REG) {
					size_t copy_len;
					copy_len=(size_t)substr_start;
					if(copy_len >= sizeof(tokens[nr_token].str)) {
						copy_len = sizeof(tokens[nr_token].str) - 1;
					}
					strncpy(tokens[nr_token].str, substr_start, copy_len);
					tokens[nr_token].str[copy_len] = '\0'; // 确保字符串以 null 结尾
					tokens[nr_token].type = token_type;
					nr_token++;
				}
				else if(token_type != NOTYPE) {
					tokens[nr_token].type = token_type;
					tokens[nr_token].str[0] = '\0'; // 清空字符串
					nr_token++;
				}
				matched=1;
				break;
			}
		}

		//匹配失败
		if (!matched) {  
            printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
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
    		
			default: return 7;
		}
}


static int dominant_op(int p,int q){
		int op=-1;               //当前找到的主导运算符下标，初始化 -1 表示未找到
		int min_pri=100;          //当前最小优先级（初始比任何实际运算符优先级都大）
		int balance=0;           //括号平衡计数
		int i;

		for (i = p; i <= q; i++) {
			int t=tokens[i].type;
			 if (t == LPAREN) { 
				balance++; 
				continue; 
			}
			if (t == RPAREN) { 
				balance--; 
				continue; 
			}
			if(balance!=0) continue; //跳过括号内的运算符

			if(is_binary_op_token(t)){
				int pri=precedence(t);
				if(pri<=min_pri){
					min_pri=pri;
					op=i;
				}
			}
		}
		if(balance!=0){
			return -1;
		}
		return op;
}

// 判断 tokens[p..q] 是否被一对括号完整包裹
static bool check_parentheses(int p, int q) {
    int i;
    int balance = 0;

    if (p > q) 
		return false;
    if (tokens[p].type != LPAREN || tokens[q].type != RPAREN) 
		return false;

    for (i = p; i <= q; i++) {
        if (tokens[i].type == LPAREN) {
            balance++;
        }
        else if (tokens[i].type == RPAREN) {
            balance--;
            if (balance < 0) {
                return false; // 括号不匹配
            }
            if (balance == 0 && i < q) {
                return false; // 最外层括号提前闭合，不是完整包裹
            }
        }
    }

    return balance == 0;
}

uint32_t reg_str2val(const char *s, bool *success) {
    *success = true;

    if (strcmp(s, "$eax") == 0) return cpu.eax;
    if (strcmp(s, "$ebx") == 0) return cpu.ebx;
    if (strcmp(s, "$ecx") == 0) return cpu.ecx;
    if (strcmp(s, "$edx") == 0) return cpu.edx;
    if (strcmp(s, "$esp") == 0) return cpu.esp;
    if (strcmp(s, "$ebp") == 0) return cpu.ebp;
    if (strcmp(s, "$esi") == 0) return cpu.esi;
    if (strcmp(s, "$edi") == 0) return cpu.edi;
    if (strcmp(s, "$eip") == 0) return cpu.eip;

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