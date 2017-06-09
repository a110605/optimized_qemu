/*
 *  (C) 2010 by Computer System Laboratory, IIS, Academia Sinica, Taiwan.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdlib.h>
#include "exec-all.h"
#include "tcg-op.h"
#include "helper.h"
#define GEN_HELPER 1
#include "helper.h"
#include "optimization.h"



extern uint8_t *optimization_ret_addr;

/*
 * Shadow Stack
 */

/*
 * Shadow Stack
 */
list_t *shadow_hash_list; //linked list with data sp, prev* next*

static inline void shack_init(CPUState *env)
{
	fprintf(stderr,"initializing shack\n");
	env->shack = (uint64_t*) malloc (SHACK_SIZE * sizeof(uint64_t));
	env->shadow_ret_addr = (unsigned long *) malloc (SHACK_SIZE * sizeof(unsigned long));
	env->shack_top = env->shack;
	env->shack_end = env->shack+SHACK_SIZE;
	env->shadow_ret_count = 0;
	env->shadow_hash_list = NULL;
	
	//shadow_hash_list = NULL;
}

/*
 * shack_set_shadow()
 *  Insert a guest eip to host eip pair if it is not yet created.
 */
inline void shack_set_shadow(CPUState *env, target_ulong guest_eip, unsigned long *host_eip)
{
	fprintf(stderr,"check for wanted tb\n");
	list_t *target;
	unsigned long * updatetarget;
	fprintf(stderr,"looping to find it\n");
	list_t *head;
	head = (list_t *)env->shadow_hash_list;
	if (head != NULL)
	{
		for (target = head; target->next !=NULL; target= target->next)
		{
			fprintf(stderr,"comparing %ul to %ul",target->sp.guest_eip ,guest_eip);
			if (target->sp.guest_eip == guest_eip)
			{
				fprintf(stderr,"target found\n");
				updatetarget = target->sp.shadow_slot;
				*updatetarget = (uintptr_t)host_eip;
				//delete current target
				fprintf(stderr,"update set, deleting target\n");
				if (target->prev == NULL) // is the first item
				{
					fprintf(stderr,"target is the first item\n");
					if (target->next == NULL) // is the only item
					{
						fprintf(stderr,"target is the only item\n");
						free((list_t *)env->shadow_hash_list);
						free (target);
					}else
					{
						fprintf(stderr,"target is item in between head and second\n");
						head = target->next;
						target->next->prev = NULL;
						free(target);
					}
				}else
				{
					//is not the first item
					fprintf(stderr,"target is not the first item\n");
					if (target->next == NULL)  // is the last item
					{
						fprintf(stderr,"target is the last item\n");
						target->prev->next = NULL;
						free(target);
					}else
					{
						fprintf(stderr,"target is item in between prev and next\n");
						target->prev->next = target->next;
						target->next->prev = target->prev; 
						free(target);
					}
				}
				break;
			}
			fprintf(stderr,"missed~\n");
		}
		fprintf(stderr,"exiting shack_set_shadow\n");
	}else
	{
		fprintf(stderr,"no target to look for in list! exiting\n");
	}
}

/*
 * helper_shack_flush()
 *  Reset shadow stack.
 */
void helper_shack_flush(CPUState *env)
{
	fprintf(stderr,"flushing...");
	env->shack = (uint64_t*) malloc (SHACK_SIZE * sizeof(uint64_t));
        env->shadow_ret_addr = (unsigned long *) malloc (SHACK_SIZE * sizeof(unsigned long));
        env->shack_top = env->shack;
        env->shack_end = env->shack+SHACK_SIZE;
	env->shadow_ret_count = 0;
	//flush shadow hash list
	list_t *temp;
	list_t *head;
	head = (list_t *)env->shadow_hash_list;
	while(head != NULL)
	{
		temp = head;
		head = head->next;
		free(temp);
	}
}




/*
 * push_shack()
 *  Push next guest eip into shadow stack.
 */
 
TranslationBlock *tb_find_slow_nocreate(CPUState *env, target_ulong pc,
                                      target_ulong cs_base,
                                      uint64_t flags)
{
    fprintf(stderr,"tb_find_slow_nocreated functioning... \n");
    TranslationBlock *tb, **ptb1;
    unsigned int h;
    tb_page_addr_t phys_pc, phys_page1, phys_page2;
    target_ulong virt_page2;
    tb_invalidated_flag = 0;
    /* find translated block using physical mappings */
    phys_pc = get_page_addr_code(env, pc);
    phys_page1 = phys_pc & TARGET_PAGE_MASK;
    phys_page2 = -1;
    h = tb_phys_hash_func(phys_pc);
    ptb1 = &tb_phys_hash[h];
    for(;;) {
        tb = *ptb1;
        if (!tb)
            goto not_found;
        if (tb->pc == pc &&
            tb->page_addr[0] == phys_page1 &&
            tb->cs_base == cs_base &&
            tb->flags == flags) {
            /* check next page if needed */
            if (tb->page_addr[1] != -1) {
                virt_page2 = (pc & TARGET_PAGE_MASK) +
                    TARGET_PAGE_SIZE;
                phys_page2 = get_page_addr_code(env, virt_page2);
                if (tb->page_addr[1] == phys_page2)
                    goto found;
            } else {
                goto found;
            }
        }
        ptb1 = &tb->phys_hash_next;
    }
 not_found:
   /* do not create*/
	fprintf(stderr,"tb not found, wait for create\n");
    return NULL;
 found:
    /* we add the TB in the virtual pc hash table */
    env->tb_jmp_cache[tb_jmp_cache_hash_func(pc)] = tb;
    return tb;
}
void push_shack(CPUState *env, TCGv_ptr cpu_env, target_ulong next_eip)
{
	//load count
	fprintf(stderr,"push_shack functioning...\n");
	fprintf(stderr,"check shack size...\n");
	TCGv host_shack_count = tcg_temp_new();
	tcg_gen_ld_tl(host_shack_count,cpu_env, offsetof(CPUState,shadow_ret_count));
	TCGv shack_count_limit = tcg_temp_new();
	tcg_gen_movi_tl (shack_count_limit, SHACK_SIZE);
	int no_flush_label = gen_new_label();
	tcg_gen_brcond_ptr (TCG_COND_NE, host_shack_count, shack_count_limit, no_flush_label);
	gen_helper_shack_flush(cpu_env);
	gen_set_label(no_flush_label);
	//push guest to shack
	fprintf(stderr,"push shack (guest)...\n");
    TCGv guest_shack_top = tcg_temp_new();
    tcg_gen_ld_tl(guest_shack_top , cpu_env , offsetof(CPUState,shack_top));
    tcg_gen_addi_ptr(guest_shack_top,guest_shack_top,sizeof(uint64_t));  // shacktop move towards shackend
    tcg_gen_st_tl(guest_shack_top,cpu_env, offsetof(CPUState,shack_top));
    tcg_gen_st_tl (tcg_const_tl(next_eip), guest_shack_top, 0);
    //push host to shack
	fprintf(stderr,"push shack (host)...\n");
    TCGv host_shack_top = tcg_temp_new();
    tcg_gen_ld_tl(host_shack_top, cpu_env , offsetof(CPUState,shadow_ret_addr));
    tcg_gen_addi_ptr(host_shack_top,host_shack_top,sizeof(unsigned long));
    tcg_gen_st_tl(host_shack_top,host_shack_top,offsetof(CPUState,shadow_ret_addr));
	//add count
	tcg_gen_ld_tl(host_shack_count,cpu_env, offsetof(CPUState,shadow_ret_count));
	tcg_gen_addi_ptr(host_shack_count,host_shack_count,1);
	tcg_gen_st_tl(host_shack_count,host_shack_count,offsetof(CPUState,shadow_ret_count));
	//find proper tb
	fprintf(stderr,"find tb ...\n");
	TranslationBlock *tb;
	target_ulong cs_base, pc;
    int flags;
	cpu_get_tb_cpu_state(env, &pc, &cs_base, &flags);
	tb = tb_find_slow_nocreate(env,pc, cs_base, flags);
	TCGv host_ret_addr = tcg_temp_new();
	if (tb != NULL)
	{
		//tb found
		fprintf(stderr,"tb found...\n");
		tcg_gen_mov_tl (host_ret_addr, tcg_const_tl(tb->pc));
		tcg_gen_st_tl (host_ret_addr, host_shack_top, 0);
	}else
	{
		//tb not found
		fprintf(stderr,"tb not found...\n");
		//create shadow pair
		shadow_pair new_shadow_pair;
		new_shadow_pair.guest_eip = next_eip;
		new_shadow_pair.shadow_slot = env->shadow_ret_addr;
		fprintf(stderr,"add to shadow_hash_list \n");
		//add to shadow_hash_list
		list_t * head;
		head = (list_t *)env->shadow_hash_list;
		if (head != NULL) //if not empty
		{
			list_t *tail;
			for (tail = head;tail->next !=NULL;tail=tail->next)
			{
				fprintf(stderr,"has data\n");
			}
			tail->next = malloc(sizeof(list_t));
			tail->next->sp = new_shadow_pair;
			tail->next->next = NULL;
			tail->next->prev = tail;
		}else
		{
			fprintf(stderr,"creating shadow_hash_list...\n");
			env->shadow_hash_list = (list_t *)malloc(sizeof(list_t));
			((list_t *)env->shadow_hash_list)->sp = new_shadow_pair;
			((list_t *)env->shadow_hash_list)->next = NULL;
			((list_t *)env->shadow_hash_list)->prev = NULL;
			if (env->shadow_hash_list != NULL)
			{
				fprintf(stderr,"shadow has list successfully created\n");
			}
			
		}
		fprintf(stderr,"store value anyway...\n");
		tcg_gen_st_tl (host_ret_addr, host_shack_top, 0);
		fprintf(stderr,"store value done...\n");
	}
    //free all tcg variable
	fprintf(stderr,"freeing variable(push)\n");
    tcg_temp_free(guest_shack_top);
    tcg_temp_free(host_shack_top);
	tcg_temp_free(host_ret_addr);
	tcg_temp_free(host_shack_count);
	tcg_temp_free(shack_count_limit);
	//tcg_temp_free(no_flush_label);
	fprintf(stderr,"freeing variable done(push)\n");
}




/*
 * pop_shack()
 *  Pop next host eip from shadow stack.
 */
void pop_shack(TCGv_ptr cpu_env, TCGv next_eip)
{
	fprintf(stderr,"pop shack functioning...\n");
	//load guest eip value
	fprintf(stderr,"pop shack (guest)...\n");
    TCGv pop_stack = tcg_temp_new();
    tcg_gen_ld_tl (pop_stack, cpu_env, offsetof(CPUState, shack_top) );
    TCGv pop_value = tcg_temp_new();
    tcg_gen_ld_tl (pop_value, pop_stack, 0);
	//reduce size - guest
	tcg_gen_addi_ptr (pop_stack, pop_stack, -sizeof(uint64_t));
    tcg_gen_st_tl (pop_stack, cpu_env, offsetof(CPUState,shack_top) );
	
	//if guest_eip != next_eip jump to flush
	int flush_label =  gen_new_label();
    tcg_gen_brcond_ptr (TCG_COND_NE, next_eip, pop_value, flush_label);
	
	//load host ret value
	fprintf(stderr,"pop shack (host)...\n");
	TCGv ret_addr_stack = tcg_temp_new();
    tcg_gen_ld_tl (ret_addr_stack, cpu_env, offsetof(CPUState, shadow_ret_addr) );
    TCGv ret_addr = tcg_temp_new();
    tcg_gen_ld_tl (ret_addr, ret_addr_stack, 0);

	//reduce size - host
	tcg_gen_addi_ptr (ret_addr_stack, ret_addr_stack, -sizeof(unsigned long));
    tcg_gen_st_tl (ret_addr_stack, cpu_env, offsetof(CPUState, shadow_ret_addr) );
    tcg_temp_free (ret_addr_stack);
	//reduce count
	TCGv host_shack_count = tcg_temp_new();
	tcg_gen_ld_tl(host_shack_count,cpu_env, offsetof(CPUState,shadow_ret_count));
	tcg_gen_addi_ptr(host_shack_count,host_shack_count,-1);
	tcg_gen_st_tl(host_shack_count,host_shack_count,offsetof(CPUState,shadow_ret_count));
	
	//success case
	fprintf(stderr,"updating empty entry...\n");
	TCGv zero = tcg_temp_new();
    tcg_gen_movi_tl (zero, 0);
    int null_addr = gen_new_label();
    tcg_gen_brcond_ptr (TCG_COND_EQ, zero, ret_addr, null_addr);
	*gen_opc_ptr++ = INDEX_op_jmp;
    *gen_opparam_ptr++ = GET_TCGV_PTR(ret_addr);
	
	
	gen_set_label(flush_label);
	gen_helper_shack_flush(cpu_env);
	//do nothing beacuse i dont know what cause this
	fprintf(stderr,"null addr  why?...\n");
	gen_set_label(null_addr);
	
    //free variable
	fprintf(stderr,"freeing variable(pop)\n");
	tcg_temp_free(pop_stack);
	tcg_temp_free(pop_value);
	tcg_temp_free(ret_addr_stack);
	tcg_temp_free(ret_addr);
	tcg_temp_free(host_shack_count);
	tcg_temp_free(zero);
	fprintf(stderr,"freeing variable done(pop)\n");
}

/*
 * Indirect Branch Target Cache
 */
__thread int update_ibtc;



struct ibtc_table *ibtc;
target_ulong guest_eiptemp;
int entrycount;
int entrylimit;
/*
 * helper_lookup_ibtc()
 *  Look up IBTC. Return next host eip if cache hit or
 *  back-to-dispatcher stub address if cache miss.
 */
void *helper_lookup_ibtc(target_ulong guest_eip)
{
    if (ibtc->htable != NULL)
    {
        if(ibtc->htable[guest_eip%entrylimit].guest_eip == guest_eip)
		{
			update_ibtc =0;
			return ibtc->htable[guest_eip%entrylimit].tb->tc_ptr;
		}
		else
		{
			guest_eiptemp = guest_eip;
			ibtc->htable[guest_eip%entrylimit].guest_eip = guest_eip;
			update_ibtc =1;
			return optimization_ret_addr;
		}
    }else
    {
        fprintf(stderr,"htable is empty...!?");
    }
    return optimization_ret_addr;

}

/*
 * update_ibtc_entry()
 *  Populate eip and tb pair in IBTC entry.
 */
void update_ibtc_entry(TranslationBlock *tb)
{

    fprintf(stderr,"updating record...\n"); 
    if (ibtc != NULL)
    {
		ibtc->htable[entrycount%entrylimit].guest_eip = guest_eiptemp;
		ibtc->htable[entrycount%entrylimit].tb = tb;
		entrycount++;
		fprintf(stderr, "update complete at: %d\n" , entrycount);
	if (entrycount > entrylimit)
	{
	    entrycount = entrycount % entrylimit;
	}
    }else
    {
        //fprintf(stderr,"ibtc ptr is null for some reason!?");
    }
	
}

/*
 * ibtc_init()
 *  Create and initialize indirect branch target cache.
 */
static inline void ibtc_init(CPUState *env)
{
    fprintf(stderr,"creating ibtc\n");
    ibtc = (struct ibtc_table*)malloc(sizeof(struct ibtc_table));
    entrylimit = sizeof(ibtc->htable) / sizeof(ibtc->htable[0]);
    entrycount = 0;
    int counter =0;
    for (counter =0;counter<IBTC_CACHE_SIZE;counter++)
    {
	ibtc->htable[counter].guest_eip = 0;
	ibtc->htable[counter].tb = 0;
    }	
}

/*
 * init_optimizations()
 *  Initialize optimization subsystem.
 */
int init_optimizations(CPUState *env)
{
    fprintf(stderr,"init_optimizations is correctly executed\n");
    shack_init(env);
    ibtc_init(env);

    return 0;
}

/*
 * vim: ts=8 sts=4 sw=4 expandtab
 */
