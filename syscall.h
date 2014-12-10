#ifndef SYSCALL_H
#define SYSCALL_H

enum SYSCALL {
	REBOOT, WAIT, FORK
};

void __attribute__ ((naked)) doSysCall(enum SYSCALL index, unsigned int param);

void doSysCallReboot ();
void doSysCallWait (unsigned int param); // params nbQuantums to add
void doSysCallFork ();

void sys_reboot();
void sys_wait(unsigned int nbQuantums);
void sys_fork();

void __attribute__ ((naked)) SWIHandler ();

#endif


/*
pid_t pid;

if ((pid = fork()) == 0) {
	// fils
}
else {
	// père
	// pid => pids du fils
}
// code commun
*/