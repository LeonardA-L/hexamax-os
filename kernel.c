#include "sched.h"
#include "hw.h";

void
funcA()
{
	int cptA = 0;
	while ( 1 ) {
		cptA ++;
		//ctx_switch();
	}
}


void
funcB()
{
	int cptB = 1;
	while ( 1 ) {
		cptB += 2 ;
		//ctx_switch();
	}
	//sched_exit();
}


//------------------------------------------------------------------------
int
kmain ( void )
{
	init_hw();
	init_sched();
	create_process(funcB, NULL, STACK_SIZE, 1);
	create_process(funcA, NULL, STACK_SIZE, 0);
	start_sched();
	while(1);
	/* Pas atteignable vues nos 2 fonctions */
	return 0;
}
