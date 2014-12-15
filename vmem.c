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
#define OCCUPATION_TABLE_START_ADDR	 (TOTAL_TT_SIZE + PAGE_TABLE_START_ADDR)	// 0x44c000
#define OCCUPATION_TABLE_SIZE	(131072)

#define	KERNEL_SPACE_MAX_ADDR	(0x500000)

#define DEVICE_FIRST_TABLE_INDEX	(512)
#define	LAST_KERNEL_FIRST_TABLE_INDEX	(KERNEL_SPACE_MAX_ADDR / PAGE_SIZE / SECON_LVL_TT_COUN)

#define	PROCESS_ALLOCATED_MEMORY	(10*1024*1024)
#define	PROCESS_SECOND_LVL_COUNT	(PROCESS_ALLOCATED_MEMORY / PAGE_SIZE +1)
#define	PROCESS_SPACE_SIZE	(FIRST_LVL_TT_SIZE + PROCESS_ALLOCATED_MEMORY + PROCESS_SECOND_LVL_COUNT * SECON_LVL_TT_SIZE)

#define	PROCESSES_SPACE_START	((PAGE_TABLE_START_ADDR) + TOTAL_TT_SIZE)

uint32_t* process_space = (uint32_t*)PROCESSES_SPACE_START;

uint32_t device_flags = 0b00000111110;
/*
	Domain : 0 (Large page address : useless on raspPi)
	P : 0 (??)
	SBZ : Should Be Zero
	NS : right in the middle of SBZ field on ARM doc
	 */
uint32_t t1_flags = 0b0000000001;
uint32_t memory_flags = 0b000001110010;
	
	
void start_mmu_C()
{
    register unsigned int control;
    __asm volatile("cpsie a");
    __asm volatile("cpsie i");
    
    __asm("mcr p15, 0, %[zero], c1 , c0 , 0" : : [zero] "r" (0)); // Disable cache 
    __asm("mcr p15, 0, r0 , c7 , c7 , 0"); // Invalidate cache (  data and instructions )
    __asm("mcr p15, 0, r0 , c8 , c7 , 0"); // Invalidate TLB entries
    
    __asm volatile("mrc p15, 0, %[control], c1, c0, 0" : [control] "=r" (control));
    
    /* Enable ARMv6 MMU features ( disable  sub - page AP ) */
    control |= (1<<23) | (1 << 15) | (1 << 4) | 1;
    /*  * Invalidate the translation lookaside buffer ( TLB ) */ 
    __asm volatile ("mcr p15, 0, %[data], c8 , c7 , 0" : : [data] "r"  (0));
    /* Write control register */ 
    __asm volatile ("mcr p15, 0,  %[control], c1 , c0 , 0" : : [control] "r" (control));
}
void  configure_mmu_C(register unsigned int pt_addr)
{
    //total++;
    /* Translation table 0 */ 
    __asm volatile ("mcr p15,  0, %[addr], c2 , c0 , 0" : : [addr] "r" (pt_addr));
    /*  Translation able 1 */ 
    __asm volatile ("mcr p15, 0, %[addr], c2 , c0 , 1" : : [addr] "r" (pt_addr));
    /* Use  translation table 0 for everything */ 
    __asm volatile ("mcr  p15, 0, %[n], c2 , c0 , 2" : : [n] "r" (0));
    /* Set  Domain 0 ACL to " Manager ", not enforcing memory permissions *  
     * Every mapped section / page is in domain 0 */ 
    __asm volatile  ("mcr p15, 0, %[r], c3 , c0 , 0" : : [r] "r" (0x3));
}

uint32_t computePageAddr(int first_level_idx, int second_level_idx){
	return ((first_level_idx<<8)+(second_level_idx))<<12;
}

uint32_t computeProcessPageAddr(int first_level_idx, int second_level_idx, int nbCoarse, int memory_sizeKB, uint32_t* process_space_start){
	
	uint32_t memory_start = (uint32_t)process_space_start + FIRST_LVL_TT_SIZE + nbCoarse*SECON_LVL_TT_SIZE;
	
	return memory_start + (((first_level_idx<<8)+(second_level_idx))<<12);
	
}

void* init_process_memory_space(uint32_t* start, int memory_sizeKB)
{
	int nbFrames = memory_sizeKB/PAGE_SIZE +1;
	int nbCoarse = nbFrames / SECON_LVL_TT_COUN +1;
	
	
	// Init first level tables
	int i;
	uint32_t* p = start;
	
	
	for(i=0; i<FIRST_LVL_TT_COUN; i++)
	{
		
		// Add entry to first level table
		uint32_t addr = ((((uint32_t)start)+FIRST_LVL_TT_SIZE) + i*SECON_LVL_TT_SIZE);
		
		if(i <= LAST_KERNEL_FIRST_TABLE_INDEX || i >= DEVICE_FIRST_TABLE_INDEX){
			uint32_t kernel_table_addr = (SECOND_TABLE_START_ADDR + i*SECON_LVL_TT_SIZE);
			*p = (uint32_t)((kernel_table_addr) | t1_flags);
			p++;
			continue;
		}
		else if(i > nbCoarse + LAST_KERNEL_FIRST_TABLE_INDEX && i < DEVICE_FIRST_TABLE_INDEX)	// between nbCoarse and first device frame
		{
			*p = 0;
			p++;
			continue;
		}
		else   // between 0x500000 and nbCoarse frames
		{
			*p = (uint32_t)((addr) | t1_flags);
		}
		
		p++;
		
		
		// Init second level tables
		int j;
		uint32_t* q= (uint32_t*)(addr);
		for(j=0;j<SECON_LVL_TT_COUN;j++)
		{
			
			// Add entry to second level table
			uint32_t page_addr = computeProcessPageAddr(i,j,nbCoarse, memory_sizeKB, start);
			
			*q = page_addr | device_flags;
			
			q++;
		}
		
		
	}
	
	process_space += PROCESS_SPACE_SIZE / 4;
	
	return (void*)(start);
}

unsigned int init_kern_translation_table(void){
	
	
	// Init first level tables
	int i;
	uint32_t* p = (uint32_t*)PAGE_TABLE_START_ADDR;
	
	
	//for(p = (uint32_t*)PAGE_TABLE_START_ADDR; p< (uint32_t*)(PAGE_TABLE_START_ADDR+FIRST_LVL_TT_SIZE);p++)
	for(i=0; i<FIRST_LVL_TT_COUN; i++)
	{
		// Init second level tables
		int j;
		uint32_t* q= (uint32_t*)(SECOND_TABLE_START_ADDR + i*SECON_LVL_TT_SIZE);
		//for(q= (uint32_t*)SECOND_TABLE_START_ADDR + i*SECON_LVL_TT_SIZE; q< (uint32_t*)(SECOND_TABLE_START_ADDR + (i+1)*SECON_LVL_TT_SIZE); q++)
		for(j=0;j<256;j++)
		{
			
			// Add entry to second level table
			uint32_t page_addr = computePageAddr(i,j);
			
			
			//if(page_addr < 0x500000)
			if(page_addr < 0x20FFFFFF)
			{
				//*q = page_addr<<12	 | 0b00000111110; 	// same as devices for the moment
				*q = page_addr | memory_flags;
				
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
			//j++;
			q++;
		}
		
		// Add entry to first level table
		uint32_t addr = (SECOND_TABLE_START_ADDR + i*SECON_LVL_TT_SIZE);
		*p = (uint32_t)((addr) | t1_flags);
		//i++;
		p++;
	}
	
	return 0;
}

void init_occupation_table(){
	uint8_t* q;
	int i=0;
	for(q= (uint8_t*)OCCUPATION_TABLE_START_ADDR; q< (uint8_t*)(OCCUPATION_TABLE_START_ADDR + OCCUPATION_TABLE_SIZE); q++)
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
	
	return (void*)(page_number*PAGE_SIZE);
}

void vMem_free(void *ptr, unsigned int nbPage)
{
		uint32_t frame = ((uint32_t)ptr)/PAGE_SIZE;
		uint32_t max = frame + nbPage;
		for(; frame < max; frame++)
		{
			uint8_t* q= (uint8_t*)(OCCUPATION_TABLE_START_ADDR + frame);
			*q = 0;
		}
}
/*
unsigned int
translate(unsigned int va)
{
  unsigned int pa; // result

  // Addresses
  unsigned int table_base;
  unsigned int second_level_table;

  // Indexes 
  unsigned int first_level_index;
  unsigned int second_level_index;
  unsigned int page_index;
  
  // Descriptors 
  unsigned int first_level_descriptor;
  unsigned int* first_level_descriptor_address;
  unsigned int second_level_descriptor;
  unsigned int* second_level_descriptor_address;

  __asm("mrc p15, 0, %[tb], c2, c0, 0" : [tb] "=r"(table_base));
  
  table_base = table_base & 0xFFFFC000;
  
  // Indexes 
  first_level_index = (va >> 20);
  second_level_index = ((va << 12) >> 24);
  page_index = (va & 0x00000FFF);

  // First level descriptor
  first_level_descriptor_address = (unsigned int*) (table_base | (first_level_index << 2));
  first_level_descriptor = *(first_level_descriptor_address);

  // Second level descriptor
  second_level_table = first_level_descriptor & 0xFFFFFC00;
  second_level_descriptor_address = (unsigned int*) (second_level_table | (second_level_index << 2));
  second_level_descriptor = *((unsigned int*) second_level_descriptor_address);    

  // Physical address 
  pa = (second_level_descriptor & 0xFFFFF000) | page_index;

  return pa;
}*/


void init_mem()
{	
	void* alloc;
	init_kern_translation_table();
	init_occupation_table();
	configure_mmu_C(PAGE_TABLE_START_ADDR);
	start_mmu_C();
	alloc = allocate_new_process();
}

void __attribute__ ((naked)) restart_mmu(register unsigned int pt_addr)
{
	__asm("mcr p15, 0, %[zero], c1 , c0 , 0" : : [zero] "r" (0)); // Disable cache 
    __asm("mcr p15, 0, r0 , c7 , c7 , 0"); // Invalidate cache (  data and instructions )
	__asm("mcr p15, 0, r0 , c8 , c7 , 0"); // Invalidate TLB entries
	
	configure_mmu_C(pt_addr);
	start_mmu_C();
}

void __attribute__ ((naked)) fdata_handler(){
	volatile uint8_t dfsr;
	__asm volatile("MRC p15, 0, %[DFSR], c5, c0, 0" : [DFSR] "=r" (dfsr));
	
	// Exit kernel
	volatile int i=0;
	for(;;){
		i++;
	}
}

void* allocate_new_process(){
	return init_process_memory_space(process_space,PROCESS_ALLOCATED_MEMORY);
}
