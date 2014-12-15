#ifndef	VMEM_H
#define	VMEM_H
	#include <stdint.h>
	
	void init_mem();
	void* vMem_alloc(unsigned int);
	void init_occupation_table();
	
	
	
#endif	//VMEM_H
