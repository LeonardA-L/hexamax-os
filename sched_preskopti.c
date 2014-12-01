#include "sched.h"


struct pcb_s* current_process;
struct pcb_s* first_process;
struct pcb_s* tail;
/*
void init_ctx(struct ctx_s* ctx, func_t f, unsigned int stack_size){
	void* newStack = phyAlloc_alloc(stack_size);
	ctx->routine = f;
	ctx->sp = newStack+(stack_size-32);
	ctx->context = f;
}
*/

void start_current_process(){
	__asm("bx %0" : : "r"(current_process->lr));
}

void __attribute__ ((naked)) ctx_switch()
{
	// Stack
	__asm("push {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12}");
	
	__asm("mov %0, sp" : "=r"(current_process->sp));
	__asm("mov %0, lr" : "=r"(current_process->lr));
	
	elect();
	
	__asm("mov sp, %0" : : "r"(current_process->sp));
	// Destack
	__asm("pop {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12}");
	
	start_current_process();
	
}

void init_pcb(struct pcb_s* pcb, func_t f, struct arg_s* arg, void* sp){
	pcb->lr = f;
	pcb->function = f;
	pcb->sp = sp;
	pcb->arg = arg;
	pcb->state = NEW;
}

void create_process(func_t f, void* args, unsigned int stack_size){
	void* newStack = phyAlloc_alloc(stack_size)+(stack_size-32);
	
	//struct pcb_s newPcb;
	int sizePcb = sizeof(struct pcb_s);
	struct pcb_s* newPcb = phyAlloc_alloc(sizePcb);
	
	if(first_process == NULL){
		first_process = newPcb;
	}
	else{
		/*
		struct pcb_s* p;
		for(p = first_process; p->next != NULL; p = p->next);
		p->next = newPcb;
		*/
		tail->next = newPcb;
	}
	
	tail = newPcb;
	init_pcb(newPcb, f, args, newStack);
}



void elect(){
	current_process = current_process->next;
}

void start_sched(){
		tail->next = first_process;
		current_process = tail;
}
