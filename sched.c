#include "sched.h"
#include "hw.h"


struct pcb_s* current_process;
struct pcb_s* first_process;
struct pcb_s* init_process;
int highest_priority; //retains the highest priority of the currently running processes

void init_sched()
{
	highest_priority = LOW_PRIORITY;
}

void save_elect_restore()
{
	if(current_process->state != TERMINATED){
		// Stack
		__asm("push {r0-r12}");
		// Store sp and lr
		__asm("mov %0, sp" : "=r"(current_process->sp));
		__asm("mov %0, lr" : "=r"(current_process->lr));
	
		current_process->state = WAITING;
	}
	
	elect();
	
	current_process->state = RUNNING;
	
	// Restore sp
	__asm("mov sp, %0" : : "r"(current_process->sp));
	// Destack
	__asm("pop {r0-r12}");
}

void __attribute__ ((naked)) ctx_switch()
{
	save_elect_restore();
	start_current_process();
	
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

void sched_exit()
{
	current_process -> state = TERMINATED;
	//ctx_switch();
	elect();
}

void terminate_process(struct pcb_s* pcb)
{
	// Unalloc
	phyAlloc_free(pcb->sp -(STACK_SIZE-REGISTERS_SIZE-1), STACK_SIZE);		// Unalloc stack by reshifting its pointer to the original value
	phyAlloc_free(pcb, sizeof(struct pcb_s));								// Unalloc pcb's memory
	// Reloop
	int highestPrio = LOW_PRIORITY;
	struct pcb_s* p;
	for(p = pcb; p->next != pcb; p = p->next)
	{
		if (highestPrio>p->priority) {
			highestPrio = p->priority;
		}
	}
	highest_priority=highestPrio;
	p->next = pcb->next;
}

void create_process(func_t f, void* args, unsigned int stack_size, int priority)
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
	
	if(first_process == NULL){
		first_process = newPcb;
	}
	else{
		//TODO :parse the PCB list for individual processes priority and insert the newly created process in the list
		struct pcb_s* p;		
		for(p = first_process; p->next != NULL; p = p->next);
		p->next = newPcb;
		
	}

	init_pcb(newPcb, f, args, newStack, priority);
	((struct pcb_s*)newPcb)->state = READY;
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
}

void start_current_process()
{
	__asm("bx %0" : : "r"(current_process->lr));		// Goto current process' lr
}

void elect_with_fixed_priority(){
	while(1) {
		if (current_process->next->state == TERMINATED) {
			terminate_process(current_process->next);
		}
		current_process = current_process->next;
		if (current_process->priority == highest_priority) {
			break;
		}
	}
}

void elect_with_wait()
{
	if (current_process->next->state == TERMINATED) {
		terminate_process(current_process->next);
	}
	current_process = current_process->next;			// Elect a new process (i.e the next in the list)

	while(current_process->state == WAITING){
		(current_process->waitCounter)--;
		if (current_process->waitCounter == 0) {
			current_process->waitCounter = 0;
			current_process->state = READY;
			break;
		}
		else if (current_process->waitCounter == -1) {
			current_process->waitCounter = 0;
			break;
		}
		else {
			current_process = current_process->next;
		}
	}
}

void elect()
{
	elect_with_wait();
}

void waitAndSwitch(unsigned int nbQuantum)
{
	current_process->waitCounter = nbQuantum;
	ctx_switch_from_syscall();
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
	
	current_process->state = WAITING;
	
	elect();
	
	// Restore sp
	__asm("mov sp, %0" : : "r"(current_process->sp));
	// Destack
	__asm("pop {r0-r12}");
	
	
	//save_elect_restore();
	
	set_tick_and_enable_timer();
	ENABLE_IRQ();
	
	if(current_process->state == READY){
		current_process->state = RUNNING;
		__asm("bx %0" : : "r"(current_process->lr));		// Goto current process' lr
	}
	else{
		__asm("rfeia sp!");
	}
	
	
	//set_tick_and_enable_timer();
	//__asm("rfeia %0!" : : "r"(current_process->lr));
	//__asm("rfedb !");
	
	// Set tick and enable timer
	// Enable IRQ ??
}

void __attribute__ ((naked)) ctx_switch_from_syscall()
{
	DISABLE_IRQ();
	
	// Stack
	__asm("push {r0-r12}");
	// Store sp and lr
	__asm("mov %0, sp" : "=r"(current_process->sp));
	__asm("mov %0, lr" : "=r"(current_process->lr));
	
	current_process->state = WAITING;
	
	elect();
	
	// Restore sp
	__asm("mov sp, %0" : : "r"(current_process->sp));
	// Destack
	__asm("pop {r0-r12}");
	
	
	set_tick_and_enable_timer();
	ENABLE_IRQ();
	
	if(current_process->state == READY){
		current_process->state = RUNNING;
		__asm("bx %0" : : "r"(current_process->lr));		// Goto current process' lr
	}
	else{
		start_current_process();
	}
	
}


void start_sched()
{
	// Loop the chained list of process
	struct pcb_s* p;
	for(p = first_process; p->next != NULL; p = p->next);
	p->next = first_process;
	
	// Create a blank init process that will serve as entrance to the loop
	int sizePcb = sizeof(struct pcb_s);
	init_process = phyAlloc_alloc(sizePcb);
	init_process->next = first_process;
	current_process = init_process;		// Set it as first process
	
	ENABLE_IRQ();
	set_tick_and_enable_timer();
}
