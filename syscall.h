#ifndef SYSCALL_H
#define SYSCALL_H

enum SYSCALL {
	REBOOT, READ
};

void doSysCall(SYSCALL index);

void __attribute__ ((naked)) SWIHandler ();

#endif
