#include "sched.h"
#include "hw.h"
#include "syscall.h"

void
funcA()
{
	int cptA = 0;
	while ( 1 ) {
		cptA ++;
	}
}


void
funcB()
{
	int cptB = 1;
	while ( cptB < 20 ) {
		cptB += 2 ;
	}
	enum SYSCALL reboot = REBOOT;
	doSysCall(reboot);
	sched_exit();
}

//------------------------------------------------------------------------
int
kmain ( void )
{
	init_hw();
	create_process(funcB, NULL, STACK_SIZE);
	create_process(funcA, NULL, STACK_SIZE);
	start_sched();
	while(1);
	/* Pas atteignable vues nos 2 fonctions */
	return 0;
}
