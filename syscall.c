#include "syscall.h"
#include "sched.h"
#include "hw.h"

void __attribute__ ((naked)) SWIHandler ()
{

	__asm("srsdb sp!, #0x13");
	__asm("cps #0x13");

	int function;
	__asm("mov %0, r0" : "=r"(function));
	
	switch (function)
	{
	  	case REBOOT :
			doSysCallReboot();
		break;

		case WAIT :
        {
			unsigned int param;
			__asm("mov %0, r1" : "=r"(param));
			doSysCallWait(param);
        }
		break;
	}
	__asm("rfeia sp!");
}

void __attribute__ ((naked)) doSysCall(enum SYSCALL index, unsigned int param) {

	DISABLE_IRQ();

	__asm("srsdb sp!, #0x13");
	__asm("cps #0x13");

	// on stocke le numéro de l'appel système à executer
	__asm("mov r0, %0" : : "r"(index));

	if (param != 0x0) {
		__asm("mov r1, %0" : : "r"(param));
	}

	// SoftWare Interupt
	__asm("SWI 0" : : : "lr");

	set_tick_and_enable_timer();
	ENABLE_IRQ();
	__asm("rfeia sp!");
}


// ----------


void doSysCallReboot ()
{
	sys_reboot();
}

void doSysCallWait (unsigned int param) {
	sys_wait(param);
}

void doSysCallFork()
{
	sys_fork();
}


// -----------


void sys_reboot()
{
	const int PM_RSTC = 0x2010001c;
	const int PM_WDOG = 0x20100024;
	const int PM_PASSWORD = 0x5a000000;
	const int PM_RSTC_WRCFG_FULL_RESET = 0x00000020;
	
	PUT32(PM_WDOG, PM_PASSWORD | 1);
	PUT32(PM_RSTC, PM_PASSWORD | PM_RSTC_WRCFG_FULL_RESET);

	while (1);
}

void sys_wait (unsigned int nbQuantums)
{
    // appeler le scheduler : changer l'état du process => WAITING + switch
	waitAndSwitch(nbQuantums);
}

void sys_fork()
{

}