#ifndef	VMEM_H
#define	VMEM_H
	#include <stdint.h>
	
	void init_mem();
	void* vMem_alloc(unsigned int);
	void init_occupation_table();
	void* allocate_new_process();
	void restart_mmu(register unsigned int pt_addr);
	
	
	
#endif	//VMEM_H
