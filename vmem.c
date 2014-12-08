#include "vmem.h"

#define PAGE_SIZE	4096

#define	SECON_LVL_TT_COUN	256		// Second level page indexes are stored on 8b
#define SECON_LVL_TT_SIZE	(4*SECON_LVL_TT_COUN)

#define FIRST_LVL_TT_COUN	4096		// First level page indexes are stored on 12b
#define FIRST_LVL_TT_SIZE	(4*FIRST_LVL_TT_COUN)

#define	SECTION_SIZE	(SECON_LVL_TT_COUN * PAGE_SIZE)		// 
#define TOTAL_TT_SIZE	(FIRST_LVL_TT_SIZE + SECON_LVL_TT_SIZE * FIRST_LVL_TT_COUN)			// 42Mo

#define PAGE_TABLE_START_ADDR	(0x48000)
#define SECOND_TABLE_START_ADDR (PAGE_TABLE_START_ADDR+FIRST_LVL_TT_SIZE)

// #define OCCUPATION_TABLE_START_ADDR	(SECOND_TABLE_START_ADDR + (FIRST_LVL_TT_COUN+1)*SECON_LVL_TT_SIZE)
#define OCCUPATION_TABLE_START_ADDR	 (TOTAL_TT_SIZE + PAGE_TABLE_START_ADDR)
#define OCCUPATION_TABLE_SIZE	(131072)


void start_mmu_C()
{
	register unsigned int control;
	__asm("mcr p15, 0, %[zero], c1, c0, 0" : : [zero] "r"(0)); //Disable cache
	__asm("mcr p15, 0, r0, c7, c7, 0"); //Invalidate cache (data and instructions) */
	__asm("mcr p15, 0, r0, c8, c7, 0"); //Invalidate TLB entries
	/* Enable ARMv6 MMU features (disable sub-page AP) */
	control = (1<<23) | (1 << 15) | (1 << 4) | 1;
	/* Invalidate the translation lookaside buffer (TLB) */
	__asm volatile("mcr p15, 0, %[data], c8, c7, 0" : : [data] "r" (0));
	/* Write control register */
	__asm volatile("mcr p15, 0, %[control], c1, c0, 0" : : [control] "r" (control));
}
void configure_mmu_C()
{
	register unsigned int pt_addr = PAGE_TABLE_START_ADDR;
	//total++;
	/* Translation table 0 */
	__asm volatile("mcr p15, 0, %[addr], c2, c0, 0" : : [addr] "r" (pt_addr));
	/* Translation table 1 */
	__asm volatile("mcr p15, 0, %[addr], c2, c0, 1" : : [addr] "r" (pt_addr));
	/* Use translation table 0 for everything */
	__asm volatile("mcr p15, 0, %[n], c2, c0, 2" : : [n] "r" (0));
	/* Set Domain 0 ACL to "Manager", not enforcing memory permissions
	* Every mapped section/page is in domain 0
	*/
	__asm volatile("mcr p15, 0, %[r], c3, c0, 0" : : [r] "r" (0x3));
}


uint32_t computePageAddr(int first_level_idx, int second_level_idx){
	return (first_level_idx<<20)+(second_level_idx<<12);
}


unsigned int init_kern_translation_table(void){
	uint32_t device_flags = 0b00000111111;
	/*
	Domain : 0 (Large page address : useless on raspPi)
	P : 0 (??)
	SBZ : Should Be Zero
	NS : right in the middle of SBZ field on ARM doc
	 */
	uint32_t t1_flags = 0b0000001;
	// Init first level tables
	int i=0;
	uint32_t* p;
	for(p = (uint32_t*)PAGE_TABLE_START_ADDR; p< (uint32_t*)(PAGE_TABLE_START_ADDR+FIRST_LVL_TT_SIZE);p++)
	{
		// Init second level tables
		int j=0;
		uint32_t* q;
		for(q= (uint32_t*)SECOND_TABLE_START_ADDR + i*SECON_LVL_TT_SIZE; q< (uint32_t*)(SECOND_TABLE_START_ADDR + (i+1)*SECON_LVL_TT_SIZE); q++)
		{
			
			// Add entry to second level table
			uint32_t page_addr = computePageAddr(i,j);
			if(page_addr < 0x500000)
			{
				*q = page_addr | 0b00000111110; 	// same as devices for the moment
			}
			else if(page_addr > 0x20000000 && page_addr < 0x20FFFFFF )
			{
				*q = page_addr | device_flags;
			}
			else
			{
				// Translation fault
				*q = 0;
			}
			j++;
		}
		
		// Add entry to first level table
		*p = (uint32_t)((SECOND_TABLE_START_ADDR + i*SECON_LVL_TT_SIZE) | t1_flags);
		i++;
	}
	
	return 0;
}

void init_occupation_table(){
	uint8_t* q;
	int i=0;
	for(q= (uint8_t*)OCCUPATION_TABLE_START_ADDR;; q< (uint8_t*)(OCCUPATION_TABLE_START_ADDR + OCCUPATION_TABLE_SIZE); q++)
	{
		if(i<1133){	// address of the last occupied frame at init
			*q = 1;
		}
		else{
			*q = 0;
		}
		i++;
	}
	
}

void* vMem_alloc(unsigned int nbPages)
{
	uint8_t* q;
	int j = 0;
	uint8_t* segment = 0;
	int page_number = 0;
	int i=0;
	for(q= (uint8_t*)OCCUPATION_TABLE_START_ADDR; q< (uint8_t*)(OCCUPATION_TABLE_START_ADDR + OCCUPATION_TABLE_SIZE); q++)
	{
		if(*q == 0){
			j++;
		}
		else{
			j=0;
		}
		
		if(j== nbPages){
			segment = q-nbPages;
			page_number = i-nbPages;
			break;
		}
		i++;
	}
	
	if(segment == 0) return (void*)0;
	
	for(q = segment;q<segment+nbPages;q++){
		*q = 1;
	}
	
	return (void*)(page_number*4096);
}

void init_mem()
{	
	void* alloc;
	init_kern_translation_table();
	init_occupation_table();
	configure_mmu_C();
	start_mmu_C();
	alloc = vMem_alloc(4);
}
