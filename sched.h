#ifndef	SCHED_H
#define SCHED_H

#define STACK_SIZE	2000
#define REGISTERS_SIZE	56
#define	NULL	0x0
#define LOW_PRIORITY 19
#define HIGH_PRIORITY (-20)



typedef void (*func_t) ( void);


enum pState {NEW, READY, RUNNING, WAITING, TERMINATED};

struct arg_s {
	void* arg;
	struct arg_s* next;
};

struct pcb_s {
	func_t function;		// a pointer to the process' function
	struct arg_s* arg;		// a pointer to a list of arguments for the function
	void* sp;				// a saved value of the process' sp
	void* lr;				// a saved value of the process' lr
	enum pState state;		// The state of the process
	int waitCounter;		// nb call this proc
	struct pcb_s* next;		// The next process in the chained list
	int priority;			//priority (for fixed-priority scheduler; ranges from -20 to +19)
	void* start_stack;      // first adress of stack
};

void __attribute__ ((naked)) ctx_switch_from_irq();
void __attribute__ ((naked)) ctx_switch_from_syscall();
void __attribute__ ((naked)) ctx_switch();
void waitAndSwitch(unsigned int nbQuantum);

void init_sched();
void init_pcb(struct pcb_s* pcb, func_t f, struct arg_s* arg, void* sp, int prio);
void create_process(func_t f, void* args, unsigned int stack_size, int priority);
void create_process_dynamically (func_t f, void* args, unsigned int stack_size, int priority);
void copy_stack(struct pcb_s* pcbFrom, struct pcb_s* pcbTo);

void elect();
void elect_with_wait();
void elect_with_fixed_priority();

void start_sched();

void terminate_process();

void sched_exit();


#endif // SCHED_H
