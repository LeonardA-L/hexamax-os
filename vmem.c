#define PAGE_SIZE	4096

#define	SECON_LVL_TT_COUN	256		// Second level page indexes are stored on 8b
#define SECON_LVL_TT_SIZE	(4*SECON_LVL_TT_COUN)

#define FIRST_LVL_TT_COUN	4096		// First level page indexes are stored on 12b
#define FIRST_LVL_TT_SIZE	(4*FIRST_LVL_TT_COUN)

#define	SECTION_SIZE	(SECON_LVL_TT_COUN * PAGE_SIZE)		// 
#define TOTAL_TT_SIZE	(FIRST_LVL_TT_SIZE + SECON_LVL_TT_SIZE * FIRST_LVL_TT_COUN)			// 42Mo

#define PAGE_TABLE_START_ADDR	0x48001

unsigned int init_kern_translation_table(void){
	//for(void* p = 0x0; p<0x50000
}
