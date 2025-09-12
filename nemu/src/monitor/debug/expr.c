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

//初始化正则表达式数组，为后续的词法分析做准备。
void init_regex() {
	int i;
	char error_msg[128];
	int ret;

	for(i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if(ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			Assert(ret == 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

//定义类型token
typedef struct token {
	int type;
	char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool is_binary_op_token(int type);

static void mark_unary_operators() {
	for (int i = 0; i < nr_token; i++) {
		if (tokens[i].type == MINUS) {
			if (i == 0 || tokens[i - 1].type == LPAREN || is_binary_op_token(tokens[i - 1].type)) {
				tokens[i].type = NEG; 
			}
		}
		else if (tokens[i].type == MUL) {
			if (i == 0 || tokens[i - 1].type == LPAREN || is_binary_op_token(tokens[i - 1].type)) {
				tokens[i].type = DEREF; 
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
		bool matched = false;
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0){
				//匹配成功处理
				matched = true;

				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);

				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array `tokens'. For certain types
				 * of tokens, some extra actions should be performed.
				 */

				//token生成逻辑

				int token_type=rules[i].token_type;

				if(token_type == MINUS) {
                    if(nr_token == 0 ||                             // 表达式开头
                       tokens[nr_token - 1].type == LPAREN ||      // 左括号后
                       tokens[nr_token - 1].type == PLUS ||
                       tokens[nr_token - 1].type == MINUS ||
                       tokens[nr_token - 1].type == MUL ||
                       tokens[nr_token - 1].type == DIV ||
                       tokens[nr_token - 1].type == EQ ||
                       tokens[nr_token - 1].type == NEQ ||
                       tokens[nr_token - 1].type == AND ||
                       tokens[nr_token - 1].type == OR) {
                        token_type = NEG;  // 标记为一元负号
                    }
                }
				switch(token_type){
					case NOTYPE:break;
					case NUM:
					case HEX:
					case REG:
						strncpy(tokens[nr_token].str, substr_start, substr_len);
            			tokens[nr_token].str[substr_len] = '\0';
            			tokens[nr_token].type = rules[i].token_type;
            			nr_token++;
						break;

					default: 
						tokens[nr_token].type=token_type;
						tokens[nr_token].str[0]='\0';
						nr_token++;
						break;
				}

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


//括号匹配
static bool check_parentheses(int p,int q){
		if(tokens[p].type!=LPAREN || tokens[q].type!=RPAREN )
			return false;
		int balance=0;        //用来跟踪括号配对,遇到 (，balance++,遇到 )，balance--,最终 balance==0 表示括号完全配对。
		for(int i=p;i<=q;i++){
			if(tokens[i].type==LPAREN)
				balance++;
			else if(tokens[i].type==RPAREN)
				balance--;
			if(balance==0 && i<q)
				return false;
		}

		return balance==0;
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

		for (int i = q; i >= p; i++) {
			if (tokens[i].type == RPAREN) { 
				balance++; 
				continue;
			}
			if (tokens[i].type == LPAREN) { 
				balance--; 
				continue; 
			}
			if (balance > 0) 
				continue;

			int pri=precedence(tokens[i].type);

			if(tokens[i].type == NEG || tokens[i].type == DEREF) {
            	pri = 6;
        	}
			//更换主导运算符
			if(pri<=min_pri){
				min_pri=pri;
				op=i;
			}
		}
		return op;
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
	/* TODO: Insert codes to evaluate the expression. */
	if(p>q)
	{
		*success=false;
		return 0;
	}


	//区间只有一个token
	else if(p==q){
		*success=true;
		switch(tokens[p].type){
			case NUM:
				return strtoul(tokens[p].str,NULL,10);
			case HEX:
			{
				return strtoul(tokens[p].str,NULL,16);
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


	//如果区间被一对括号完整包裹,去掉首尾括号递归求值
	else if (check_parentheses(p, q)) {
		return eval(p + 1, q - 1,success);
	}

	//含运算符的区间
	else {
		int op=dominant_op(p,q);         //找到主导运算符 op

		if (op == -1) {  
			*success = false;
			printf("Error: No dominant operator found in expression segment p=%d q=%d\n", p, q);
			return 0;
    	}
		if (op < p || op > q) { 
			*success = false;
			printf("Error: dominant operator out of range: op=%d p=%d q=%d\n", op, p, q);
			return 0;
		}

		int type = tokens[op].type;

		if(type==NEG){
			if (op + 1 > q) {   // 检查右操作数是否存在
            *success = false;
            printf("Error: unary NEG without operand at op=%d\n", op);
            return 0;
        }
			uint32_t val=eval(op+1,q,success);
			return (uint32_t)(-((int32_t)val));
		}
		if(type==DEREF){
			if (op + 1 > q) {   //检查右操作数是否存在
            *success = false;
            printf("Error: DEREF without operand at op=%d\n", op);
            return 0;
        }
			uint32_t addr=eval(op+1,q,success);
			return vaddr_read(addr,4);
		}
		if (op - 1 < p || op + 1 > q) {  //检查二元运算符是否有左右操作数
        *success = false;
        printf("Error: binary operator at edge op=%d p=%d q=%d\n", op, p, q);
        return 0;
    }


		//二元运算
		uint32_t val1 = eval(p, op - 1,success);
		uint32_t val2 = eval(op + 1, q,success);


		//根据运算符计算
		switch(type){
			case PLUS: return val1 + val2;
			case MINUS: return val1 - val2;
			case MUL: return val1 * val2;
			case DIV: Assert(val2 != 0, "Divide by zero"); return val1 / val2;
			case EQ: return val1 == val2;
			case NEQ: return val1 != val2;
			case AND: return val1 && val2;
			case OR:  return val1 || val2;
			default: Assert(0, "Unknown operator %d", type);
		}
	}
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

    *success=true;
    uint32_t val = eval(0, nr_token-1, success);

    // 调试输出 signed/unsigned 两种形式
    printf("expr result = %d (signed), %u (unsigned)\n", (int32_t)val, val);

    return val;
}



