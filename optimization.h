/*
 *  (C) 2010 by Computer System Laboratory, IIS, Academia Sinica, Taiwan.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef __OPTIMIZATION_H
#define __OPTIMIZATION_H

/* Comment the next line to disable optimizations. */
#define ENABLE_OPTIMIZATION

/*
 * Link list facilities
 */
 
struct shadow_pair
{
    //struct list_head l;
    target_ulong guest_eip;
    unsigned long *shadow_slot;
};
 
struct list_head {
    struct list_head *next, *prev;
	struct shadow_pair sp;
};
typedef struct list_head list_t;
typedef struct shadow_pair shadow_pair;

/*
 * Shadow Stack
 */

#if TCG_TARGET_REG_BITS == 32
#define tcg_gen_st_ptr          tcg_gen_st_i32
#define tcg_gen_brcond_ptr      tcg_gen_brcond_i32
#define tcg_temp_free_ptr       tcg_temp_free_i32
#define tcg_temp_local_new_ptr  tcg_temp_local_new_i32
#else
#define tcg_gen_st_ptr          tcg_gen_st_i64
#define tcg_gen_brcond_ptr      tcg_gen_brcond_i64
#define tcg_temp_free_ptr       tcg_temp_free_i64
#define tcg_temp_local_new_ptr  tcg_temp_local_new_i64
#endif

#if TARGET_LONG_BITS == 32
#define TCGv TCGv_i32
#else
#define TCGv TCGv_i64
#endif

#define MAX_CALL_SLOT   (16 * 1024)
#define SHACK_SIZE      (16 * 1024)



inline void shack_set_shadow(CPUState *env, target_ulong guest_eip, unsigned long *host_eip);
inline void insert_unresolved_eip(CPUState *env, target_ulong next_eip, unsigned long *slot);
unsigned long lookup_shadow_ret_addr(CPUState *env, target_ulong pc);
void push_shack(CPUState *env, TCGv_ptr cpu_env, target_ulong next_eip);
void pop_shack(TCGv_ptr cpu_env, TCGv next_eip);
TranslationBlock *tb_find_slow_nocreate(CPUState *env,target_ulong pc,target_ulong cs_base,uint64_t flags);
/*
 * Indirect Branch Target Cache
 */
#define IBTC_CACHE_BITS     (16)
#define IBTC_CACHE_SIZE     (1U << IBTC_CACHE_BITS)
#define IBTC_CACHE_MASK     (IBTC_CACHE_SIZE - 1)

struct jmp_pair
{
    target_ulong guest_eip;
    TranslationBlock *tb;
};

struct ibtc_table
{
    struct jmp_pair htable[IBTC_CACHE_SIZE];
};

int init_optimizations(CPUState *env);
void update_ibtc_entry(TranslationBlock *tb);

#endif

/*
 * vim: ts=8 sts=4 sw=4 expandtab
 */
