#include "sched.h"
#include "hw.h"
#include "syscall.h"

void
funcA()
{/*
	int cptA = 0;
	int waitDone = 0;
	while ( 1 ) {
		cptA ++;
		if (!waitDone && cptA%10 == 0) {
			enum SYSCALL wait = WAIT;
			doSysCall(wait, 3);
			waitDone = 1;
		}
	}*/
	int cptA = 1;
	while ( cptA < 3 ) {
		cptA ++;
	}
	enum SYSCALL wait = WAIT;
	doSysCall(wait, 3);
	sched_exit();
}


void
funcB()
{
	int cptB = 1;
	while ( 1 ) {
		cptB += 2 ;
	}
	//enum SYSCALL reboot = REBOOT;
	//doSysCall(reboot, NULL);
	sched_exit();
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
