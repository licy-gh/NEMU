#include "nemu.h"
#include <stdlib.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
	NOTYPE = 256,
	EQ,	NEQ,					// ==, !=
	DECNUM,	HEXNUM,				// decimal number, hex number
	PLUS, SUB, MULTI, DIV,		// +, -, *, /
	LBRAKT, RBRAKT,				// (, )
	DAND, DOR, DNOT,			// &&, ||, !
	REGISTER_NUM				// register
	
	/* TODO: Add more token types */

};

static struct rule {
	char *regex;
	int token_type;
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */

	{" +",	NOTYPE},
	{"\\+", PLUS},
	{"-", SUB},
	{"\\*", MULTI},
	{"/", DIV},
	{"\\(", LBRAKT},
	{"\\)", RBRAKT},
	{"&&", DAND},
	{"\\|\\|", DOR},
	{"!", DNOT},
	{"[0-9]+", DECNUM},
	{"0[xX][0-9a-fA-F]+", HEXNUM},
	{"!=", NEQ},
	{"==", EQ},
	{"\\$[a-zA-Z]+", REGISTER_NUM}
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
int priority[32];
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

				switch(rules[i].token_type) {
					case NOTYPE:
						break;
					default:
						tokens[nr_token].type = rules[i].token_type;
						// DECNUM, HEXNUM:	0
						// ==, !=:			1
						// &&, ||:			2
						// +, -  :			3
						// *, /  :			4
						// !     :			5
						// (, )  :			6
						switch (tokens[nr_token].type){
							case EQ:
							case NEQ:
						    	priority[nr_token] = 1;
								break;
							case DAND:
							case DOR:
							    priority[nr_token] = 2;
								break;
							case PLUS:
							case SUB:
							    priority[nr_token] = 3;
								break;
							case MULTI:
							case DIV:
								priority[nr_token] = 4;
								break;
							case DNOT:
							    priority[nr_token] = 5;
								break;
							case LBRAKT:
							case RBRAKT:
							    priority[nr_token] = 6;
							default:
								priority[nr_token] = 0;
								break;
						}
						if(rules[i].token_type == DECNUM || rules[i].token_type == HEXNUM){
							if(substr_len >= 32)
							    Assert(0, "overflow!");
							else{
								strncpy (tokens[nr_token].str,substr_start,substr_len);
								tokens[nr_token].str[substr_len]='\0';
							}
						}
						nr_token++;
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

bool check_parentheses(int left, int right){
	if(tokens[left].type == LBRAKT && tokens[right].type == RBRAKT){
		int i, l_cnt = 0, r_cnt = 0;
		for(i = left + 1; i <= right - 1; i++){
			if(tokens[i].type == LBRAKT)
			    l_cnt++;
			if(tokens[i].type == RBRAKT && l_cnt)
			    r_cnt++;
		}
		return l_cnt == r_cnt;
	}
	return false;
}

int dominant_operator(int left, int right){
    int i;
	int min_priority = 114514, pos = left;
	for(i = left; i <= right; i++){
		if(tokens[i].type == DECNUM || tokens[i].type == HEXNUM){
			continue;
		}
		else{
			if(tokens[i].type == LBRAKT){
				int lb_cnt = 1, j;
				for(j = i + 1; j <= right && lb_cnt; j++){
					if(tokens[j].type == LBRAKT) lb_cnt++;
					if(tokens[j].type == RBRAKT && lb_cnt) lb_cnt--;
				}
				i = j - 1;
			}
			if(priority[i] <= min_priority){
				min_priority = priority[i];
				pos = i;
			}
		}
	}
	return pos;
}

uint32_t eval(int left, int right){
	if(left > right){
	    Assert(0,"left greater than right\n");
	}
	else if(left == right){
		int num;
		if(tokens[left].type == DECNUM){
			sscanf(tokens[left].str,"%d",&num);
		}
		if(tokens[left].type == HEXNUM){
			sscanf(tokens[left].str,"%x",&num);
		}
		return num;
	}
	else if(check_parentheses(left, right)){
		return eval(left + 1, right - 1);
	}
	else{
		int op = dominant_operator(left, right);
		uint32_t val1 = eval(left, op - 1);
		uint32_t val2 = eval(op + 1, right);
		switch (tokens[op].type){
			case PLUS:	return val1 + val2;
			case SUB:	return val1 - val2;
			case MULTI:	return val1 * val2;
			case DIV:	return val1 / val2;
			case EQ:	return val1 == val2;
			case NEQ:	return val1 != val2;
			case DAND:	return val1 && val2;
			case DOR:	return val1 || val2;
		}
	}
	return -114514;
}

uint32_t expr(char *e, bool *success) {
	if(!make_token(e)) {
		*success = false;
		return 0;
	}

	/* TODO: Insert codes to evaluate the expression. */
	*success = true;
	return eval(0, nr_token - 1);
}

