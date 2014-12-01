#include "sched.h"
//#include "dispatcher.h"
#include "hw.h"

#define	STACK_SIZE	32

struct ctx_s ctx_ping;
struct ctx_s ctx_pong;
struct ctx_s ctx_init;

void
ping()
{
	while ( 1 ) {
		switch_to(&ctx_pong);
	}
}
void
pong()
{
	while ( 1 ) {
		switch_to(&ctx_ping);
	}
}
//------------------------------------------------------------------------
int
kmain ( void )
{
	init_hw();
	init_ctx(&ctx_ping, ping, STACK_SIZE);
	init_ctx(&ctx_pong, pong, STACK_SIZE);
	current_ctx = &ctx_init;
	switch_to(&ctx_ping);
	/* Pas atteignable vues nos 2 fonctions */
	return(0);
}
