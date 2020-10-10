#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
	int NO;
	struct watchpoint *next;

	/* TODO: Add more members if necessary */
	int key_val;
	char expression[32];

} WP;

void init_wp_pool();

WP* new_wp(char* expressions);

void free_wp(WP *wp);

void delete_wp(int index);

void print_wp();

bool check_wp();

#endif
