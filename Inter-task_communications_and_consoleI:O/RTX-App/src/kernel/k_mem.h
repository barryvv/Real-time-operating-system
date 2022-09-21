/*
 ****************************************************************************
 *
 *                  UNIVERSITY OF WATERLOO ECE 350 RTOS LAB
 *
 *                     Copyright 2020-2022 Yiqing Huang
 *
 *          This software is subject to an open source license and
 *          may be freely redistributed under the terms of MIT License.
 ****************************************************************************
 */

/**************************************************************************//**
 * @file        k_mem.h
 * @brief       Kernel Memory Management API Header File
 *
 * @version     V1.2021.04
 * @authors     Yiqing Huang
 * @date        2021 APR 
 *
 * @note        skeleton code
 *
 *****************************************************************************/


#ifndef K_MEM_H_
#define K_MEM_H_
#include "k_inc.h"
#include "lpc1768_mem.h"        // board memory map

/*
 * ------------------------------------------------------------------------
 *                             FUNCTION PROTOTYPES
 * ------------------------------------------------------------------------
 */
// kernel API that requires mpool ID
typedef struct list_head{
	struct list_head *n, *p;
}list_head;
struct block {
    struct list_head block_list;
    int order; 
};
typedef struct free_area_t{
	struct list_head free_list;
	int nr_free;
}free_area_t;
mpool_t k_mpool_create  (int algo, U32 strat, U32 end);
void   *k_mpool_alloc   (mpool_t mpid, size_t size);
int     k_mpool_dealloc (mpool_t mpid, void *ptr);
int     k_mpool_dump    (mpool_t mpid);

int     k_mem_init      (int algo);
U32    *k_alloc_k_stack (task_t tid);
U32    *k_alloc_p_stack (task_t tid);
// declare newly added functions here
int ptr_to_pidx(void* ptr,mpool_t mpid);
void add_to_list(struct list_head *new_node, struct list_head *prev, struct list_head *next);
void init_list_head(struct list_head* list);
void bitarray_init(U8* bit_array);
void *memset(void *s, int c, size_t n);
U32 get_mpool_size (U32 start, U32 end);
unsigned int get_log_2_down (U32 val);
/*
 * ------------------------------------------------------------------------
 *                             FUNCTION MACROS
 * ------------------------------------------------------------------------
 */

#endif // ! K_MEM_H_

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

