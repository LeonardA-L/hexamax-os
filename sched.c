#include "sched.h"
#include "hw.h"
#include "syscall.h"

// TODO - faire une fonction pour terminer le process manuellement

struct pcb_s* current_process;
int highest_priority; //retains the highest priority of the currently running processes
unsigned long clockTicks;

void init_sched()
{
	highest_priority = LOW_PRIORITY;
	clockTicks = 0;
	current_process = NULL;
}

void start_sched(){
	// Create a blank init process
	int sizePcb = sizeof(struct pcb_s);
	struct pcb_s* init_process = phyAlloc_alloc(sizePcb);
	init_process->priority = LOW_PRIORITY;
	init_process->next = current_process->next;
	current_process->next = init_process; // Set it as first process
	current_process = init_process;
	
	ENABLE_IRQ();
	set_tick_and_enable_timer();
}

void init_pcb(struct pcb_s* pcb, func_t f, struct arg_s* arg, void* sp, int prio)
{
	// init pcb's variables
	pcb->lr = f;
	pcb->function = f;
	pcb->sp = sp;
	pcb->arg = arg;
	pcb->state = NEW;
	pcb->waitCounter = 0;
	if(prio < HIGH_PRIORITY || prio > LOW_PRIORITY) {
		pcb->priority = LOW_PRIORITY;
	}
	else {
		pcb->priority = prio;
	}
}

void terminate_process(struct pcb_s* pcb){
	// Unalloc
	phyAlloc_free(pcb->sp -(STACK_SIZE-REGISTERS_SIZE-1), STACK_SIZE);		// Unalloc stack by reshifting its pointer to the original value
	phyAlloc_free(pcb, sizeof(struct pcb_s));								// Unalloc pcb's memory
	
	// Reloop
	struct pcb_s* p;
	for(p = pcb; p->next != pcb; p = p->next);
	p->next = pcb->next;
}

void exit_process(enum SYSCALL index, unsigned int errorCode) {
	//terminate_process(current_process); // fait dans le elect()
	ctx_switch_from_syscall(index, errorCode);
}

void start_current_process()
{
	__asm("bx %0" : : "r"(current_process->lr));		// Goto current process' lr
}

void create_process_dynamically (func_t f, void* args, unsigned int stack_size, int priority)
{
	// Alloc a new stack space, shift the pointer to the end minus the registers we will pop, minus one because it's the last address
	void* newStack = phyAlloc_alloc(stack_size)+(stack_size-REGISTERS_SIZE-1);
	
	//struct pcb_s newPcb;
	int sizePcb = sizeof(struct pcb_s);
	void* newPcb = phyAlloc_alloc(sizePcb);
	
	if(priority < highest_priority)
	{
		highest_priority = priority;
	}
	
	//TODO :parse the PCB list for individual processes priority and insert the newly created process in the list
	struct pcb_s* p;		
	for(p = current_process; p->next != current_process; p = p->next);
	p->next = newPcb;

	init_pcb(newPcb, f, args, newStack, priority);
	((struct pcb_s*)newPcb)->next = current_process;
	((struct pcb_s*)newPcb)->state = READY;
	((struct pcb_s*)newPcb)->start_stack = newStack;
}

void copy_stack(struct pcb_s* pcbFrom, struct pcb_s* pcbTo)
{
	void* adr = NULL;
	for(adr=pcbFrom->start_stack; adr!=pcbFrom->sp; adr-=2)
	{
		*((char*)pcbTo->start_stack + (adr-pcbFrom->start_stack) ) = *(char*)adr;
	}
}

void updateHighestPriority () {
	int highestPrio = LOW_PRIORITY;
	
	int cptBreak = MAX_PROCESS; // because circle list
	struct pcb_s* p;
	// parse chain list
	for (p = current_process->next ; cptBreak ; p = p->next, --cptBreak)
	{
		// update state from WAITING to READY if needed
		if (p->state == WAITING && p->waitCounter < clockTicks) {
			p->state = READY;
		}
		// uppdate highest_priority if needed
		if (p->state == READY && highestPrio > p->priority) {
			highestPrio = p->priority;
		}
		// break condition
		if (p->next == current_process) {
			cptBreak = 2;
		}
	}
	highest_priority=highestPrio;
}

void elect() {
	updateHighestPriority();
	elect_with_fixed_priority();
}

void elect_with_fixed_priority(){
	while(1) {
		if (current_process->next->state == TERMINATED) {
			terminate_process(current_process->next);
		}
		current_process = current_process->next;

		if (current_process->priority == highest_priority) {
			if (current_process->state == READY) {
				break;
			}
		}
	}
}

void waitAndSwitch(enum SYSCALL index, unsigned int nbQuantum) {
	current_process->waitCounter = clockTicks+nbQuantum;
	ctx_switch_from_syscall(index, nbQuantum);
}

void __attribute__ ((naked)) ctx_switch_from_irq()
{
	DISABLE_IRQ();
	
	__asm("sub lr, lr, #4");
	__asm("srsdb sp!, #0x13");
	__asm("mov %0, lr" : "=r"(current_process->lr));
	__asm("cps #0x13");
	
	// Stack
	__asm("push {r0-r12}");
	// Store sp and lr
	__asm("mov %0, sp" : "=r"(current_process->sp));
	
	current_process->state = READY;
	
	clockTicks++;
	
	elect();
	
	// Restore sp
	__asm("mov sp, %0" : : "r"(current_process->sp));
	// Destack
	__asm("pop {r0-r12}");
		
	set_tick_and_enable_timer();
	ENABLE_IRQ();
	
	if(current_process->state == READY){  // probably no need but safer
		current_process->state = RUNNING;
		__asm("bx %0" : : "r"(current_process->lr));		// Goto current process' lr
	}
	else{
		__asm("rfeia sp!");
	}
}

void __attribute__ ((naked)) ctx_switch_from_syscall(enum SYSCALL index, unsigned int param) {
	DISABLE_IRQ();
	
	// Stack
	__asm("push {r0-r12}");
	// Store sp and lr
	__asm("mov %0, sp" : "=r"(current_process->sp));
	__asm("mov %0, lr" : "=r"(current_process->lr));
	
	// IF SYSCALL ASK TO WAIT !!!
	if ( index == WAIT ) {
		current_process->state = WAITING;
	}
	else if ( index == EXIT ) {
		current_process->state = TERMINATED;
	}
	
	elect();
	
	// Restore sp
	__asm("mov sp, %0" : : "r"(current_process->sp));
	// Destack
	__asm("pop {r0-r12}");
	
	
	set_tick_and_enable_timer();
	ENABLE_IRQ();
	
	if(current_process->state == READY){ // probably no need but safer
		current_process->state = RUNNING;
		__asm("bx %0" : : "r"(current_process->lr));		// Goto current process' lr
	}
	else{
		start_current_process();
	}
}

void create_process(func_t f, void* args, unsigned int stack_size, int priority)
{
	// Alloc a new stack space, shift the pointer to the end minus the registers we will pop, minus one because it's the last address
	void* newStack = phyAlloc_alloc(stack_size)+(stack_size-REGISTERS_SIZE-1);
	
	//struct pcb_s newPcb;
	int sizePcb = sizeof(struct pcb_s);
	void* newPcb = phyAlloc_alloc(sizePcb);
	
	if(priority < highest_priority) {
		highest_priority = priority;
	}
	
	init_pcb(newPcb, f, args, newStack, priority);
	
	if(current_process == NULL){
		current_process = newPcb;
		current_process->next = current_process;
	}
	else {
		((struct pcb_s*)newPcb)->next = current_process->next;
		current_process->next = newPcb;
	}
	((struct pcb_s*)newPcb)->state = READY;
	((struct pcb_s*)newPcb)->start_stack = newStack;
}
