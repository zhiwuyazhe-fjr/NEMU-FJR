#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>

enum {
	NOTYPE = 256,
	EQ,
	TK_DEC

};

static struct rule {
	char *regex;
	int token_type;
} rules[] = {

	{" +",	NOTYPE},				// spaces
	{"[0-9]+",	TK_DEC},			// decimal number
	{"\\+",	'+'},					// plus
	{"-",	'-'},					// minus
	{"\\*",	'*'},					// multiply
	{"/",	'/'},					// divide
	{"\\(",	'('},					// left paren
	{"\\)",	')'},					// right paren
	{"==",	EQ}						// equal
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
				switch(rules[i].token_type) {
					case NOTYPE:
						/* spaces are skipped */
						break;

					case TK_DEC:
						if(substr_len >= 32) { assert(0); }
						memcpy(tokens[nr_token].str, substr_start, substr_len);
						tokens[nr_token].str[substr_len] = '\0';
						tokens[nr_token].type = TK_DEC;
						nr_token ++;
						break;

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

/* precedence of operators, larger means higher */
static int prec(int type) {
	switch(type) {
		case '*': case '/': return 3;
		case '+': case '-': return 2;
		case EQ: return 1;
		default: return -1;
	}
}

static uint32_t eval(int p, int q, bool *success) {
	if(*success == false) { return 0; }

	if(p > q) {
		/* bad expression */
		*success = false;
		return 0;
	}
	else if(p == q) {
		/* single token, must be a number */
		if(tokens[p].type != TK_DEC) {
			*success = false;
			return 0;
		}
		return strtoul(tokens[p].str, NULL, 10);
	}

	if(check_parentheses(p, q) == true) {
		return eval(p + 1, q - 1, success);
	}

	/* find the dominant operator: the one with the lowest precedence
	 * and out of parentheses, take the rightmost on a tie */
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
