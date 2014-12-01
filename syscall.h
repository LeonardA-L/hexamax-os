#ifndef SYSCALL_H
#define SYSCALL_H

enum SYSCALL {
	REBOOT, READ, WAIT
};

void doSysCall(enum SYSCALL index, unsigned int param);
void doSysCallReboot ();
void doSysCallRead ();
void doSysCallWait (unsigned int param); // params nbQuantums to add

void sys_reboot();
void sys_wait(unsigned int nbQuantums);

void __attribute__ ((naked)) SWIHandler ();

#endif
