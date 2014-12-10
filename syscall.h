#ifndef SYSCALL_H
#define SYSCALL_H

enum SYSCALL {
	REBOOT, WAIT, EXIT
};

void __attribute__ ((naked)) doSysCall(enum SYSCALL index, unsigned int param);

void __attribute__ ((naked)) doSysCallReboot ();
void doSysCallWait (unsigned int param); // params nbQuantums to add
void doSysCallExit (unsigned int param);

void __attribute__ ((naked)) sys_reboot();
void sys_wait(unsigned int nbQuantums);
void sys_exit(unsigned int errorCOde);

void __attribute__ ((naked)) SWIHandler ();

#endif
