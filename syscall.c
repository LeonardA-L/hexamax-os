#include "syscall.h"

#define SIG_REBOOT 1

void __attribute__ ((naked)) SWIHandler ()
{
	int param;
	__asm("r0 %0" : : "r"param);
	switch (param)
	{
	  case SIG_REBOOT :
		doSysCallReboot();
		break;
	}
}

void doSysCall(SYSCALL index) {
	// on stocke le numéro de l'appel système à executer
	__asm("mov r0, %0" : : "r"(index) : "r0");
	// SoftWare Interupt
	__asm("SWI 0" : : : "lr");
}

void doSysCallReboot () {
	const int PM_RSTC = 0x2010001c;
	const int PM_WDOG = 0x20100024;
	const int PM_PASSWORD = 0x5a000000;
	const int PM_RSTC_WRCFG_FULL_RESET = 0x00000020;
	
	PUT32(PM_WDOG, PM_PASSWORD | 1);
	PUT32(PM_RSTC, PM_PASSWORD | PM_RSTC_WRCFG_FULL_RESET);

	while (1);
}
