#include "syscall.h"

void __attribute__ ((naked)) SWIHandler ()
{
	
}

void doSysCall(SYSCALL index) {
	// on stocke le numéro de l'appel système à executer
	__asm("mov r0, %0" : : "r"(index) : "r0");
	// SoftWare Interupt
	__asm("SWI 0" : : : "lr");
}
