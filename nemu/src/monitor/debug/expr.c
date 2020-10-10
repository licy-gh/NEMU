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
	REGISTER_NUM,				// register
	MINUS, DERFE				// negative number, pointer dereference
	
	/* TODO: Add more token types */

};

static struct rule {
	char *regex;
	int token_type;
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */

	{"0[xX][0-9a-fA-F]+", HEXNUM},
	{"[0-9]+", DECNUM},
	{" +",	NOTYPE},
	{"\\+", PLUS},
	{"-", SUB},
	{"\\*", MULTI},
	{"/", DIV},
	{"\\(", LBRAKT},
	{"\\)", RBRAKT},
	{"&&", DAND},
	{"\\|\\|", DOR},
	{"!=", NEQ},
	{"==", EQ},
	{"!", DNOT},
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
					case REGISTER_NUM:
						tokens[nr_token].type = rules[i].token_type; 
						priority[nr_token] = 0;
						strncpy(tokens[nr_token].str,substr_start + 1,substr_len-1);
						tokens[nr_token].str[substr_len-1]='\0';
						nr_token ++;
						break;
					default:
						tokens[nr_token].type = rules[i].token_type;
						// DECNUM, HEXNUM:	0
						// ==, !=:			1
						// &&, ||:			2
						// +, -  :			3
						// *, /  :			4
						// !, DEREF, MINUS:	5
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
								strncpy(tokens[nr_token].str,substr_start,substr_len);
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
			if(tokens[i].type == RBRAKT)
			    r_cnt++;
			if(r_cnt > l_cnt) return false;
		}
		if(l_cnt == r_cnt) return true;
	}
	return false;
}

int dominant_operator(int left, int right){
	int i, min_priority = 114514, pos = left;
	for(i = left; i <= right; i++){
		if(tokens[i].type == DECNUM || tokens[i].type == HEXNUM || tokens[i].type == REGISTER_NUM){
			continue;
		}
		while(tokens[i].type == LBRAKT){
			int j, cnt = 1;
			for(j = i + 1; j <= right && cnt; j++){
				if(tokens[j].type == LBRAKT) cnt++;
				if(tokens[j].type == RBRAKT) cnt--;
			}
			i = j;
		}
		if(i > right) break;
		// int cnt = 0, j;
		// bool flag = true;
		// for (j = i - 1; j >= left ;j --){ 
		// 	if (tokens[j].type == LBRAKT && !cnt){
		// 		flag = false;
		// 		break;
		// 	}
		// 	if (tokens[j].type == LBRAKT)cnt --;
		// 	if (tokens[j].type == RBRAKT)cnt ++; 
		// }
		// if (!flag)continue;

		if(priority[i] <= min_priority){
			min_priority = priority[i];
			pos = i;
		}
	}
	printf("dominate op position: %d\n", pos);
	return pos;
}

uint32_t eval(int left, int right){
	if(left > right){
	    Assert(0,"left greater than right\n");
	}
	else if(left == right){
		uint32_t num = 0;
		if(tokens[left].type == DECNUM){
			sscanf(tokens[left].str,"%d",&num);
		}
		else if(tokens[left].type == HEXNUM){
			sscanf(tokens[left].str,"%x",&num);
		}
		else if (tokens[left].type == REGISTER_NUM){
			if (strlen(tokens[left].str) == 3) {
				int i;
				for (i = R_EAX; i <= R_EDI; i ++)
					if (strcmp(tokens[left].str,regsl[i]) == 0) break;
				if (i > R_EDI)
					if (strcmp(tokens[left].str,"eip") == 0)
						num = cpu.eip;
					else Assert (0,"no this register!\n");
				else num = reg_l(i);
			}
			else if (strlen(tokens[left].str) == 2) {
				if (tokens[left].str[1] == 'x' || tokens[left].str[1] == 'p' || tokens[left].str[1] == 'i') {
					int i;
					for (i = R_AX; i <= R_DI; i ++)
						if (strcmp (tokens[left].str,regsw[i]) == 0)break;
					num = reg_w(i);
				}
				else if (tokens[left].str[1] == 'l' || tokens[left].str[1] == 'h') {
					int i;
					for (i = R_AL; i <= R_BH; i ++)
						if (strcmp (tokens[left].str,regsb[i]) == 0)break;
					num = reg_b(i);
				}
				else Assert(0, "abababababa");
			}
		}
		return num;
	}
	else if(check_parentheses(left, right)== true){
		return eval(left + 1, right - 1);
	}
	else{
		int op = dominant_operator(left, right);
		if (left == op || tokens[op].type == DERFE || tokens[op].type == MINUS || tokens[op].type == DNOT){
			uint32_t val = eval (left + 1,right);
			printf("single calculate\n");
			printf("val = %d\n", val);
			switch (tokens[left].type){
				case DERFE:return swaddr_read (val,4);
				case MINUS:return -val;
				case DNOT:return !val;
				default :Assert(0, "come default\n");
			} 
		}
		printf("double calculate\n");
		uint32_t val1 = eval(left, op - 1);
		uint32_t val2 = eval(op + 1, right);
		printf("val1 = %d, val2 = %d\n", val1, val2);
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
	int i;
	for(i = 0; i < nr_token; i++){
		if(tokens[i].type == SUB && (i == 0 || (tokens[i - 1].type != DECNUM && tokens[i - 1].type != HEXNUM && tokens[i - 1].type != REGISTER_NUM && tokens[i].type != RBRAKT))){
			tokens[i].type = MINUS;
			priority[i] = 5;
		}
		if(tokens[i].type == MULTI && (i == 0 || (tokens[i - 1].type != DECNUM && tokens[i - 1].type != HEXNUM && tokens[i - 1].type != REGISTER_NUM && tokens[i].type != RBRAKT))){
			tokens[i].type = DERFE;
			priority[i] = 5;
		}
	}
	/* TODO: Insert codes to evaluate the expression. */
	*success = true;
	return eval(0, nr_token - 1);
}