#ifndef SYSCALL_H
#define SYSCALL_H

enum SYSCALL {
	REBOOT, READ
};

void doSysCall(enum SYSCALL index);
void doSysCallReboot ();

void __attribute__ ((naked)) SWIHandler ();

#endif
