#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>

enum {
	NOTYPE = 256,
	EQ, NEQ, AND, OR, NOT,
	TK_DEC, TK_HEX, TK_REG

};

static struct rule {
	char *regex;
	int token_type;
} rules[] = {

	/* 长的token要放在短的前面, 比如==要放在=之类的前面,
	 * 0x开头的十六进制要放在十进制前面, 不然0xc0100000会被拆开 */
	{" +",	NOTYPE},				// spaces
	{"0[xX][0-9a-fA-F]+",	TK_HEX},	// hexadecimal number
	{"[0-9]+",	TK_DEC},			// decimal number
	{"\\$[a-zA-Z]+",	TK_REG},	// register
	{"==",	EQ},					// equal
	{"!=",	NEQ},					// not equal
	{"&&",	AND},					// logical and
	{"\\|\\|",	OR},				// logical or
	{"!",	NOT},					// logical not
	{"\\*",	'*'},					// multiply
	{"\\+",	'+'},					// plus
	{"-",	'-'},					// minus
	{"/",	'/'},					// divide
	{"\\(",	'('},					// left paren
	{"\\)",	')'}					// right paren
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

				/* a new token is recognized, record it in tokens[] */
				if(nr_token >= 32) { assert(0); }

				switch(rules[i].token_type) {
					case NOTYPE:
						/* spaces are skipped */
						break;

					case TK_DEC:
					case TK_HEX:
						if(substr_len >= 32) { assert(0); }
						memcpy(tokens[nr_token].str, substr_start, substr_len);
						tokens[nr_token].str[substr_len] = '\0';
						tokens[nr_token].type = rules[i].token_type;
						nr_token ++;
						break;

					case TK_REG: {
						int j;
						if(substr_len >= 32) { assert(0); }
						for(j = 0; j < substr_len; j ++) {
							char c = substr_start[j];
							/* 转成小写, 这样$EAX和$eax都能认 */
							tokens[nr_token].str[j] = (c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c;
						}
						tokens[nr_token].str[substr_len] = '\0';
						tokens[nr_token].type = TK_REG;
						nr_token ++;
						break;
					}

					default:
						tokens[nr_token].type = rules[i].token_type;
						nr_token ++;
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

/* check whether tokens[p..q] is surrounded by a matched pair of
 * parentheses, e.g. "( 2 - 1 )" is true, "4 + 3 * ( 2 - 1 )" is false */
static bool check_parentheses(int p, int q) {
	if(tokens[p].type != '(' || tokens[q].type != ')') {
		return false;
	}

	int par = 0;
	int i;
	for(i = p; i <= q; i ++) {
		if(tokens[i].type == '(') { par ++; }
		else if(tokens[i].type == ')') {
			par --;
			if(par < 0) { return false; }
			if(par == 0) { return (i == q); }
		}
	}
	/* unbalanced */
	return false;
}

/* precedence of binary operators, larger means higher,
 * return -1 if type is not a binary operator */
static int prec(int type) {
	switch(type) {
		case '*': case '/': return 4;
		case '+': case '-': return 3;
		case EQ: case NEQ: return 2;
		case AND: return 1;
		case OR: return 0;
		default: return -1;
	}
}

/* name is like "$eax" */
static uint32_t reg_value(const char *name, bool *success) {
	int i;
	for(i = R_EAX; i <= R_EDI; i ++) {
		if(strcmp(name + 1, regsl[i]) == 0) { return reg_l(i); }
	}
	for(i = R_AX; i <= R_DI; i ++) {
		if(strcmp(name + 1, regsw[i]) == 0) { return reg_w(i); }
	}
	for(i = R_AL; i <= R_BH; i ++) {
		if(strcmp(name + 1, regsb[i]) == 0) { return reg_b(i); }
	}
	if(strcmp(name + 1, "eip") == 0) { return cpu.eip; }

	*success = false;
	return 0;
}

static uint32_t eval(int p, int q, bool *success) {
	if(*success == false) { return 0; }

	if(p > q) {
		/* bad expression */
		*success = false;
		return 0;
	}
	else if(p == q) {
		/* single token, must be a number or a register */
		switch(tokens[p].type) {
			case TK_DEC: return strtoul(tokens[p].str, NULL, 10);
			case TK_HEX: return strtoul(tokens[p].str, NULL, 16);
			case TK_REG: return reg_value(tokens[p].str, success);
			default:
				*success = false;
				return 0;
		}
	}

	if(check_parentheses(p, q) == true) {
		return eval(p + 1, q - 1, success);
	}

	/* find the dominant operator: the binary operator with the lowest
	 * precedence out of parentheses, take the rightmost on a tie */
	int op = -1;
	int par = 0;
	int i;
	for(i = p; i <= q; i ++) {
		if(tokens[i].type == '(') { par ++; continue; }
		else if(tokens[i].type == ')') { par --; continue; }
		if(par != 0) { continue; }

		if(prec(tokens[i].type) < 0) { continue; }
		if(op == -1 || prec(tokens[i].type) <= prec(tokens[op].type)) {
			op = i;
		}
	}

	if(op == -1) {
		/* 没有二元运算符, 可能是单目的! */
		if(tokens[p].type == NOT) {
			uint32_t val = eval(p + 1, q, success);
			if(*success == false) { return 0; }
			return !val;
		}
		*success = false;
		return 0;
	}

	uint32_t val1 = eval(p, op - 1, success);
	uint32_t val2 = eval(op + 1, q, success);
	if(*success == false) { return 0; }

	switch(tokens[op].type) {
		case '+': return val1 + val2;
		case '-': return val1 - val2;
		case '*': return val1 * val2;
		case '/':
			if(val2 == 0) {
				*success = false;
				return 0;
			}
			return val1 / val2;
		case EQ: return val1 == val2;
		case NEQ: return val1 != val2;
		case AND: return val1 && val2;
		case OR: return val1 || val2;
		default:
			assert(0);
			return 0;
	}
}

uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}

	*success = true;
	return eval(0, nr_token - 1, success);
}
