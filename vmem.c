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
