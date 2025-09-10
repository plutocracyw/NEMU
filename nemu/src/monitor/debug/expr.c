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
  NOTYPE = 256, EQ, NUM, HEX, REG, NEQ, AND, OR,
  NEG, DEREF,   // 单目运算
  // 二元运算
  PLUS = '+', MINUS = '-', MUL = '*', DIV = '/',
  LPAREN = '(', RPAREN = ')'
};

static struct rule 
{
	char *regex;
	int token_type;
} 

rules[] = 
{

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */

	 {" +",   NOTYPE},       // 空格
	{"\\+",   PLUS},         // +
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

typedef struct token {
	int type;
	char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
	int position = 0;
	int i;
	regmatch_t pmatch;
	
	nr_token = 0;

	while(e[position] != '\0') {
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", i, rules[i].regex, position, substr_len, substr_len, substr_start);

				position += substr_len;

				/* TODO: Now a new token is recognized with rules[i]. Add codes
				 * to record the token in the array `tokens'. For certain types
				 * of tokens, some extra actions should be performed.
				 */

				switch(rules[i].token_type) 
				{
					case NOTYPE:break;
					case NUM:
					case REG:
						strncpy(tokens[nr_token].str, substr_start, substr_len);
            			tokens[nr_token].str[substr_len] = '\0';
            			tokens[nr_token].type = rules[i].token_type;
            			nr_token++;
						break;

					default: 
						tokens[nr_token].type=rules[i].token_type;
						tokens[nr_token].str[0]='\0';
						nr_token++;
						break;
				}

				break;
			}
		}

		if(i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}

	return true; 
}


static bool check_parentheses(int p,int q)
{
		if(tokens[p].type!=LPAREN || tokens[q].type!=RPAREN )
			return false;
		int balance=0;
		for(int i=p;i<=q;i++)
		{
			if(tokens[i].type==LPAREN)
				balance++;
			else if(tokens[i].type==RPAREN)
				balance--;
			if(balance==0 && i<q)
				return false;
		}

		return balance==0;
}  //括号匹配


static int precedence(int type)
{
		switch(type)
		{
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


static int dominant_op(int p,int q)
{
		int op=-1;
		int min_pri=10;
		int balance=0;

		for (int i = p; i <= q; i++) 
		{
			if (tokens[i].type == LPAREN) 
			{ 
				balance++; 
				continue;
			}
			if (tokens[i].type == RPAREN) 
			{ 
				balance--; 
				continue; 
			}
			if (balance > 0) 
				continue;

			int pri=precedence(tokens[i].type);
			if(pri<=min_pri)
			{
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
    printf("Warning: vaddr_read(%u, %d) called, return dummy 0\n", addr, len);
    return 0;
}


static uint32_t eval(int p,int q,bool *success)
{
	if(p>q)
	{
		*success=false;
		return 0;
	}

	else if(p==q)
	{
		if(tokens[p].type==NUM)
			return strtoul(tokens[p].str,NULL,10);
		else if(tokens[p].type==HEX)
			return strtoul(tokens[p].str,NULL,16);
		else if (tokens[p].type == REG) 
			return reg_str2val(tokens[p].str, success);
		else
		{
			*success=false;
			return 0;
		}
			
	}

	else if (check_parentheses(p, q)) 
	{
		return eval(p + 1, q - 1,success);
	}

	else 
	{
		int op=dominant_op(p,q);
		int type=tokens[op].type;

		if(type==NEG)
		{
			uint32_t val=eval(op+1,q,success);
			return -val;
		}

		else if(type==DEREF)
		{
			uint32_t addr=eval(op+1,q,success);
			return vaddr_read(addr,4);
		}

		uint32_t val1 = eval(p, op - 1,success);
		uint32_t val2 = eval(op + 1, q,success);

		switch(type)
		{
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
		//运算
	}
	return 0;
}

	/* TODO: Insert codes to evaluate the expression. */

	//main expr

uint32_t expr(char *e,bool *success)
{
	if(!make_token(e))
	{
		*success=false;
		return 0;
	}


	//解决负号和解引用
	for(int i=0;i<nr_token;i++)
	{
		if(tokens[i].type==MUL)
		{
			if(i==0 || (tokens[i-1].type != NUM &&tokens[i-1].type != HEX&& tokens[i-1].type != REG && tokens[i-1].type != RPAREN))
			{
				tokens[i].type=DEREF;
			}
		}

		else if(tokens[i].type ==MINUS)
		{
			if (i == 0 || (tokens[i-1].type != NUM && tokens[i-1].type != HEX
			&& tokens[i-1].type != REG && tokens[i-1].type != RPAREN)) 
			{
				tokens[i].type = NEG;
			}
		}
	}

	*success=true;
	return eval(0,nr_token-1,success);

}

