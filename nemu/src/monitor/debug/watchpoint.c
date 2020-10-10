#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
	int i;
	for(i = 0; i < NR_WP; i ++) {
		wp_pool[i].NO = i;
		wp_pool[i].next = &wp_pool[i + 1];
	}
	wp_pool[NR_WP - 1].next = NULL;

	head = NULL;
	free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
WP* new_wp(char* expressions){
	if(free_){
		WP* new_watchpoint = free_;
		new_watchpoint->next = head;
		head = new_watchpoint;
		free_ = free_->next;
		bool succ = true;
		uint32_t val = expr(expressions, &succ);
		if(succ){
			new_watchpoint->key_val = val;
			strcpy(new_watchpoint->expression, expressions);
			return new_watchpoint;
		}
		else{
			Assert(0, "get value form expr failed!\n");
		}
	}
	else{
		Assert(0, "no more space for watchpoints!\n");
	}
	return NULL;
}

void free_wp(WP *wp){
	if(wp && head){
		bool found_flag = false;
		WP* current_ptr = head;
		while(current_ptr && current_ptr->NO != wp->NO){
			current_ptr = current_ptr->next;
		}
		if(current_ptr){
			found_flag = true;
			wp->next = free_;
			free_ = wp;
			printf("Successfully delete watchpoint %d", wp->NO);
		}
		else{
			printf("watchpoint %d do not exist", wp->NO);
		}
	}
	Assert(wp, "invalid watchpoint input!\n");
	Assert(head, "no watchpoint here!\n");
}
