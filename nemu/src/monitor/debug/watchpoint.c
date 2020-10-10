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
// WP* new_wp(char* expressions){
// 	if(free_){
// 		WP* new_watchpoint = free_;
// 		new_watchpoint->next = head;
// 		head = new_watchpoint;
// 		free_ = free_->next;
// 		bool succ = true;
// 		uint32_t val = expr(expressions, &succ);
// 		if(succ){
// 			new_watchpoint->key_val = val;
// 			strcpy(new_watchpoint->expression, expressions);
// 			return new_watchpoint;
// 		}
// 		else{
// 			Assert(0, "get value form expr failed!\n");
// 		}
// 	}
// 	else{
// 		Assert(0, "no more space for watchpoints!\n");
// 	}
// 	return NULL;
// }
WP* new_wp(char* args) {
	bool success = true;
	int a = expr(args, &success);
	if(!success) {
		printf("get value form expr failed!!\n");
		return NULL;
	}
	if(!free_ ) {
		printf("no more space for watchpoints\n");
		return NULL;	
	}
	WP* work = free_;
	free_ = free_->next;
	work->next = head;
	head = work;
	strcpy(work->expression, args);
	work->key_val = a;
	return work;
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
			printf("Successfully delete watchpoint %d\n", wp->NO);
		}
		else{
			printf("watchpoint %d do not exist", wp->NO);
		}
	}
	Assert(wp, "invalid watchpoint input!\n");
	Assert(head, "no watchpoint here!\n");
}

void delete_wp(int index){
	if(index >= 0 && index < NR_WP){
		free_wp(&wp_pool[index]);
	}
	else{
		Assert(0, "invalid input!\n");
	}
}

void print_wp(){
	WP* ptr = head;
	while(ptr){
		printf("NO:%d, Expr:%s, Value(hex):%#x, Value(dec):%d\n",ptr->NO, ptr->expression, ptr->key_val, ptr->key_val);
		ptr = ptr->next;
	}
}

bool check_wp() {
	bool flag_ch = false;
	if(!head) return flag_ch;
	
	WP* curr_ptr = head;
	while(curr_ptr) {
		bool success = true;
		int val = expr(curr_ptr->expression, &success);
		if(val != curr_ptr->key_val) {
			printf("Expr: %s\n", curr_ptr->expression);
			printf("previous(hex): %#x, previous(dec): %d\n", curr_ptr->key_val, curr_ptr->key_val);
			printf("now(hex): %#x, now(dec): %d\n", val, val);
			curr_ptr->key_val = val;
			flag_ch = true;
		}
		curr_ptr = curr_ptr->next;
	}
	return flag_ch;
}
