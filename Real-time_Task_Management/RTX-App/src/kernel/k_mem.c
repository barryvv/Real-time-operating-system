/*
 ****************************************************************************
 *
 *                  UNIVERSITY OF WATERLOO ECE 350 RTOS LAB
 *
 *                     Copyright 2020-2022 Yiqing Huang
 *                          All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice and the following disclaimer.
 *
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 ****************************************************************************
 */

/**************************************************************************//**
 * @file        k_mem.c
 * @brief       Kernel Memory Management API C Code
 *
 * @version     V1.2021.01.lab2
 * @authors     Yiqing Huang
 * @date        2021 JAN
 *
 * @note        skeleton code
 *
 *****************************************************************************/

#include "k_inc.h"
#include "k_mem.h"
/*---------------------------------------------------------------------------
The memory map of the OS image may look like the following:
                   RAM1_END-->+---------------------------+ High Address
                              |                           |
                              |                           |
                              |       MPID_IRAM1          |
                              |   (for user space heap  ) |
                              |                           |
                 RAM1_START-->|---------------------------|
                              |                           |
                              |  unmanaged free space     |
                              |                           |
&Image$$RW_IRAM1$$ZI$$Limit-->|---------------------------|-----+-----
                              |         ......            |     ^
                              |---------------------------|     |
                              |                           |     |
                              |---------------------------|     |
                              |                           |     |
                              |      other data           |     |
                              |---------------------------|     |
                              |      PROC_STACK_SIZE      |  OS Image
              g_p_stacks[2]-->|---------------------------|     |
                              |      PROC_STACK_SIZE      |     |
              g_p_stacks[1]-->|---------------------------|     |
                              |      PROC_STACK_SIZE      |     |
              g_p_stacks[0]-->|---------------------------|     |
                              |   other  global vars      |     |
                              |                           |  OS Image
                              |---------------------------|     |
                              |      KERN_STACK_SIZE      |     |                
             g_k_stacks[15]-->|---------------------------|     |
                              |                           |     |
                              |     other kernel stacks   |     |                              
                              |---------------------------|     |
                              |      KERN_STACK_SIZE      |  OS Image
              g_k_stacks[2]-->|---------------------------|     |
                              |      KERN_STACK_SIZE      |     |                      
              g_k_stacks[1]-->|---------------------------|     |
                              |      KERN_STACK_SIZE      |     |
              g_k_stacks[0]-->|---------------------------|     |
                              |   other  global vars      |     |
                              |---------------------------|     |
                              |        TCBs               |  OS Image
                      g_tcbs->|---------------------------|     |
                              |        global vars        |     |
                              |---------------------------|     |
                              |                           |     |          
                              |                           |     |
                              |        Code + RO          |     |
                              |                           |     V
                 IRAM1_BASE-->+---------------------------+ Low Address
    
---------------------------------------------------------------------------*/ 

/*
 *===========================================================================
 *                            GLOBAL VARIABLES
 *===========================================================================
 */
// kernel stack size, referred by startup_a9.s
const U32 g_k_stack_size = KERN_STACK_SIZE;
// task proc space stack size in bytes, referred by system_a9.c
const U32 g_p_stack_size = PROC_STACK_SIZE;

// task kernel stacks
U32 g_k_stacks[MAX_TASKS][KERN_STACK_SIZE >> 2] __attribute__((aligned(8)));
struct free_area_t free_area_1[12];//free area for RAM1
struct free_area_t free_area_2[16];//free area for RAM2

//struct block blocks_1[RAM1_SIZE/WORD_SIZE];//blocks for RAM1
//struct block blocks_2[RAM2_SIZE/WORD_SIZE];//blocks for RAM2
// task process stack (i.e. user stack) for tasks in thread mode
// remove this bug array in your lab2 code
// the user stack should come from MPID_IRAM2 memory pool
//U32 g_p_stacks[MAX_TASKS][PROC_STACK_SIZE >> 2] __attribute__((aligned(8)));
U32 g_p_stacks[NUM_TASKS][PROC_STACK_SIZE >> 2] __attribute__((aligned(8)));
U8 bit_array_1[32];//bit array for ram1
U8 bit_array_2[256];//bit array for ram2
list_head* ptr_2;
int h_1;
int h_2;
/*
 *===========================================================================
 *                            FUNCTIONS
 *===========================================================================
 */

/* note list[n] is for blocks with order of n */

unsigned int get_log_2_down (U32 val){
	unsigned int r = 0;

	while (val >>= 1)
	{
		r++;
	}
	return r;
}
unsigned int get_log_2_up (int val){
	unsigned int r = 0;
  int dummy=val;
	while (val >>= 1)
	{
		r++;
	}
	int temp = 1;
	for(int i = 0;i<r;i++){
		temp = temp*2;
	}
	if(temp == dummy){
		return r;
	}else{ 
		return r+1;
	}

}
U32 get_mpool_size (U32 start, U32 end){
	return end-start+1;
}


void init_list_head(struct list_head* list){ //double linked list init
	list->n = list;	
	list->p = list;
}	
void add_to_list(struct list_head *new_node, struct list_head *prev, struct list_head *next) { // add entry to the dlist
    next->p = new_node; 
    new_node->n = next; 
    new_node->p = prev; 
    prev->n = new_node; 
} 
int ptr_to_pidx(void* ptr,mpool_t mpid){ // give a address, return a block index
		long pidx;
		if(mpid){
			pidx= (((long)ptr - RAM2_START )/ WORD_SIZE);
		}else{
			pidx= (((long)ptr - RAM1_START )/ WORD_SIZE);
		}
		return pidx;
}
unsigned int idx_to_ptr(int index, mpool_t mpid){
	if(mpid){
			int level = get_log_2_down(index+1);
		int level_size = 1<<level;
		int block_size = 1<<(h_2-level);
	  int pos = index+1-level_size;
		return (pos*block_size*WORD_SIZE)+RAM2_START;
	}else{
			int level = get_log_2_down(index+1);
		int level_size = 1<<level;
		int block_size = 1<<(h_1-level);
	  int pos = index+1-level_size;
		return (pos*block_size*WORD_SIZE)+RAM1_START;
	}

}

/*void __delete_node(struct list_head *prev,struct list_head *next){ // delete a node from the freelist
	prev->next = next;
	next->prev = prev;
} */
void delete_node(struct list_head *node){// delete a node from the freelist
	//__delete_node(block->prev,block->next);
	node->p->n = node->n;
	node->n->p = node->p;
	node->p = NULL;
	node->n = NULL;
}
U8 bitmap_scan_test(U8 * bit_array, int bit_idx) { //give a index of bit array, check the bit_array[bit_idx] is 0 or 1;
	uint32_t byte_idx = bit_idx/8;
	uint32_t bit_odd= bit_idx%8;
	return (bit_array[byte_idx]&(1<<bit_odd));
 }
U8 bitmap_xor_and_test(U8 * bit_array, int bit_idx) {//give a index of bit array, xor the bit_array[bit_idx], check the bit_array[bit_idx] is 0 or 1, ;
	uint32_t byte_idx = bit_idx/8;
	uint32_t bit_odd= bit_idx%8;
	U8 temp = bit_array[byte_idx]^(1<<bit_odd);
	bit_array[byte_idx] = temp;
	return (bit_array[byte_idx]&(1<<bit_odd));
}

int get_bit_idx(int order, int block_index,mpool_t mpid){
			if(mpid){
					int temp = (1<<order)-1;
		temp+=block_index>>(h_2-order);
		return temp;
		}else{
				int temp = (1<<order)-1;
		temp+=block_index>>(h_1-order);
		return temp;
		}
}
int get_root_idx(int order,int block_index,mpool_t mpid){
		if(mpid){
					int temp = (1<<order)-1;
		temp+=block_index>>(h_2-order);
		return (temp-1)/2;
		}else{
				int temp = (1<<order)-1;
		temp+=block_index>>(h_1-order);
		return (temp-1)/2;
		}

}
void *list_first_entry_or_null(struct list_head *head){
	return head->n == head ? NULL:head->n; // maybe you wen ti
}
void * list_entry(struct list_head *block_list){
	return block_list;
}
 unsigned int get_buddy_index(int block_idx, int order,mpool_t mpid)
{
	if(mpid){
			return block_idx ^ (1 << (h_2-order));
	}else{
			return block_idx ^ (1 << (h_1-order));
	}

}

mpool_t k_mpool_create (int algo, U32 start, U32 end)
{
    mpool_t mpid = MPID_IRAM1; // 0
#ifdef DEBUG_0
    printf("k_mpool_init: algo = %d\r\n", algo);
    printf("k_mpool_init: RAM range: [0x%x, 0x%x].\r\n", start, end);
#endif /* DEBUG_0 */    
    
    if (algo != BUDDY ) {
        errno = EINVAL;
        return RTX_ERR;
    }
   				U32 size = get_mpool_size(start,end);

	
    if ( start == RAM1_START) {
        // add your own code
				const unsigned int h = get_log_2_down(size/WORD_SIZE); 
				h_1 = h;
        for (int i = 0; i <= h; i++) {
							init_list_head(&free_area_1[i].free_list);
							free_area_1[i].nr_free=0;		
        }
					memset(bit_array_1,0,sizeof(bit_array_1));
					list_head* block_node = (void*)RAM1_START;
				  add_to_list(block_node,&free_area_1[0].free_list,free_area_1[0].free_list.n);
				free_area_1[0].nr_free ++;
    } else if ( start == RAM2_START) {
        mpid = MPID_IRAM2;
        // add your own code
				
				const unsigned int h = get_log_2_down(size/32); 
				h_2 = h;
				for (int i = 0; i <= h; i++) {
								init_list_head(&free_area_2[i].free_list);
								free_area_2[i].nr_free=0;
					}
				
					memset(bit_array_2,0,sizeof(bit_array_2));
					list_head* block_node = (void*)RAM2_START;
				  add_to_list(block_node,&free_area_2[0].free_list,free_area_2[0].free_list.n);
					free_area_2[0].nr_free ++;
    } else {
        errno = EINVAL;
        return RTX_ERR;
    }
    
    return mpid;
}
int adr_to_ba_idx(mpool_t mpid, void* adr, int order){
	int idx = -1;
	if(!mpid){
		int pos = ((U32)adr - RAM1_START)/((1<<(h_1-order))*WORD_SIZE);
		idx = ((1<<order) -1)+pos;
	}else{
		int pos = ((U32)adr - RAM2_START)/((1<<(h_2-order))*WORD_SIZE);
		idx = ((1<<order) -1)+pos;
	}
		return idx;
}
void *k_mpool_alloc (mpool_t mpid, size_t size)
{
#ifdef DEBUG_0
    printf("k_mpool_alloc: mpid = %d, size = %d, 0x%x\r\n", mpid, size, size);
#endif /* DEBUG_0 */

	if(mpid!=0&&mpid!=1){
		return (void*)EINVAL;
	}
	if(!mpid){ //1

				if(size>RAM1_SIZE){
					errno = ENOMEM;
					return NULL;
				}

				int current_order;
				struct free_area_t *area;
				struct list_head* block;
				int index = 0;
				//struct list_head *entry;
				size = (1<<get_log_2_up(size));
				int order = h_1 - get_log_2_down(size/WORD_SIZE);
						for (current_order = order; current_order >= 0;current_order-- ){
							area = &(free_area_1[current_order]);
							block = list_first_entry_or_null(&area-> free_list);
							if(!block){
								continue;
							}
							delete_node(block); 
							area->nr_free--;
							int bit_idx = adr_to_ba_idx(mpid,block,current_order);
								bitmap_xor_and_test(bit_array_1, bit_idx);
		//					int pg_idx = ptr_to_pidx(ptr,mpid);
							for(current_order += 1 ;current_order <= order;current_order++){//current_order +=1 problem !!!!!!!
									bit_idx = bit_idx*2+1;
									int test = bitmap_scan_test(bit_array_1, bit_idx);
									test = bitmap_xor_and_test(bit_array_1, bit_idx);
									area++; 
									list_head* block_node = (void*)idx_to_ptr(bit_idx+1,mpid);
									add_to_list(block_node, &area->free_list,area->free_list.n);
								//order0:0-512(2^9) h_1 = 9 log2 = 9 order = 0
								//order1:0-255 (2^8)256-512(found) 256=freelisthead
								//order2:             256(0x1000)-383(0x1128)(block[index])384(freelisthead)-512
								//0-127      128-255        128=freelisthead 
									area ->nr_free++;
								//	block[index].order = current_order;
									//bit array
								//order0:0-512(bit_idx=0) 
								//order1:0-255(1)256-512(2=(1<<order)-1)(found)(0->1) 256=freelisthead
								//order2:0-127(3)128-255(4)     256(0x1000)-383(0x1128)(block[index])384(freelisthead)-512
								//order3:           128=freelisthead 

					
							}

							return (void*)idx_to_ptr(bit_idx,mpid);
						}
						return NULL;
	}else{ //2

					if(size>RAM2_SIZE){
					errno = ENOMEM;
						return NULL;
				}
				int current_order;
				struct free_area_t *area;
				struct list_head* block;
				int index = 0;
				//struct list_head *entry;
				size = (1<<get_log_2_up(size));
				int order = h_2 - get_log_2_down(size/WORD_SIZE);
						for (current_order = order; current_order >= 0;current_order-- ){
							area = &(free_area_2[current_order]);
							block = list_first_entry_or_null(&area-> free_list);
							if(!block){
								continue;
							}
							delete_node(block); 
							area->nr_free--;
							int bit_idx = adr_to_ba_idx(mpid,block,current_order);
								bitmap_xor_and_test(bit_array_2, bit_idx);
		//					int pg_idx = ptr_to_pidx(ptr,mpid);
							for(current_order += 1 ;current_order <= order;current_order++){//current_order +=1 problem !!!!!!!
									bit_idx = bit_idx*2+1;
									int test = bitmap_scan_test(bit_array_2, bit_idx);
									test = bitmap_xor_and_test(bit_array_2, bit_idx);
									area++; 
									list_head* block_node = (void*)idx_to_ptr(bit_idx+1,mpid);
									add_to_list(block_node, &area->free_list,area->free_list.n);
								//order0:0-512(2^9) h_1 = 9 log2 = 9 order = 0
								//order1:0-255 (2^8)256-512(found) 256=freelisthead
								//order2:             256(0x1000)-383(0x1128)(block[index])384(freelisthead)-512
								//0-127      128-255        128=freelisthead 
									area ->nr_free++;
								//	block[index].order = current_order;
									//bit array
								//order0:0-512(bit_idx=0) 
								//order1:0-255(1)256-512(2=(1<<order)-1)(found)(0->1) 256=freelisthead
								//order2:0-127(3)128-255(4)     256(0x1000)-383(0x1128)(block[index])384(freelisthead)-512
								//order3:           128=freelisthead 

					
							}
							return (void*)idx_to_ptr(bit_idx,mpid);
						}
						return NULL;
	}
    return NULL;
}


int k_mpool_dealloc(mpool_t mpid, void *ptr)
{
#ifdef DEBUG_0
    printf("k_mpool_dealloc: mpid = %d, ptr = 0x%x\r\n", mpid, ptr);
#endif /* DEBUG_0 */
	if(mpid!=0&&mpid!=1){
		errno =  EINVAL;
		return RTX_ERR;
	}

		int pidx = ptr_to_pidx(ptr,mpid); //give an address return a index
		list_head* block_node = ptr;
		if(mpid == 0){ // 1
			if((long)ptr<RAM1_START||(long)ptr>(RAM1_END-WORD_SIZE+1)){
					errno = EFAULT;
					return RTX_ERR;
			};
		//unsigned int order = get_log_2_down(blocks_1->block_size);
			int cur_order = h_1;
			while (cur_order>0){
				int p_ba_idx = get_bit_idx(cur_order,pidx,mpid);
			int root_idx = get_root_idx(cur_order,pidx,mpid);	
				// when parent is 1 and children are two 0
				if(bitmap_scan_test(bit_array_1,root_idx)==0 ){//000
							cur_order--;
							continue;
				}else{  	
					int buddy_block_idx = get_buddy_index(pidx,cur_order,mpid);
					int buddy_ba_idx = get_bit_idx(cur_order,buddy_block_idx,mpid);
					if(bitmap_scan_test(bit_array_1,p_ba_idx)){
						if(bitmap_scan_test(bit_array_1, buddy_ba_idx)){//111
							bitmap_xor_and_test(bit_array_1,p_ba_idx);
							add_to_list(block_node, &free_area_1[cur_order].free_list,free_area_1[cur_order].free_list.n);
							free_area_1[cur_order].nr_free++;
							return RTX_OK;
						}else{//110
							list_head* buddy_node = (void *)idx_to_ptr(buddy_ba_idx,mpid);
							delete_node(buddy_node);
							free_area_1[cur_order].nr_free--;
							cur_order--;
							int next_idx = pidx&buddy_block_idx;
							pidx = next_idx;
							block_node = (void *)idx_to_ptr(root_idx,mpid);
							add_to_list(block_node,&free_area_1[cur_order].free_list,free_area_1[cur_order].free_list.n);
							free_area_1[cur_order].nr_free++;
							bitmap_xor_and_test(bit_array_1,p_ba_idx);
							bitmap_xor_and_test(bit_array_1,root_idx);
							while(1){
								int root_buddy_idx = get_buddy_index(pidx,cur_order,mpid);
								int root_buddy_ba_idx = get_bit_idx(cur_order,root_buddy_idx,mpid);
								if(bitmap_scan_test(bit_array_1, root_buddy_ba_idx)){
									return RTX_OK;
								}else{
									list_head* root_buddy_node = (void *)idx_to_ptr(root_buddy_ba_idx,mpid);
									delete_node(root_buddy_node);
									delete_node(block_node);
									free_area_1[cur_order].nr_free-= 2;
									cur_order--;
									next_idx = pidx&root_buddy_idx;
									pidx = next_idx ;
									root_idx = get_bit_idx(cur_order,pidx,mpid);
									bitmap_xor_and_test(bit_array_1,root_idx);
									block_node = (void *)idx_to_ptr(root_idx,mpid);
									add_to_list(block_node,&free_area_1[cur_order].free_list,free_area_1[cur_order].free_list.n);
									free_area_1[cur_order].nr_free++;
									if(cur_order==0){
										return RTX_OK;
									}
								}
							}
						}
					}else{//100
						if(bitmap_scan_test(bit_array_1, buddy_ba_idx)){
							return -1;
						}
							if(cur_order ==1){
								bitmap_xor_and_test(bit_array_1,root_idx);
								add_to_list(block_node,&free_area_1[0].free_list,free_area_1[0].free_list.n);
								free_area_1[0].nr_free++;
								return RTX_OK;
							}else{
								cur_order--;
								continue;
							}
					}

				}

			}
		}else{ //2
		if((long)ptr<RAM2_START||(long)ptr>(RAM2_END-WORD_SIZE+1)){
					errno = EFAULT;
					return RTX_ERR;
		};
		//unsigned int order = get_log_2_down(blocks_1->block_size);
			int cur_order = h_2;
			while (cur_order>0){
				int p_ba_idx = get_bit_idx(cur_order,pidx,mpid);
			int root_idx = get_root_idx(cur_order,pidx,mpid);	
				// when parent is 1 and children are two 0
				if(bitmap_scan_test(bit_array_2,root_idx)==0 ){//000
							cur_order--;
							continue;
				}else{  	
					int buddy_block_idx = get_buddy_index(pidx,cur_order,mpid);
					int buddy_ba_idx = get_bit_idx(cur_order,buddy_block_idx,mpid);
					if(bitmap_scan_test(bit_array_2,p_ba_idx)){
						if(bitmap_scan_test(bit_array_2, buddy_ba_idx)){//111
							bitmap_xor_and_test(bit_array_2,p_ba_idx);
							add_to_list(block_node, &free_area_2[cur_order].free_list,free_area_2[cur_order].free_list.n);
							free_area_2[cur_order].nr_free++;
							return RTX_OK;
						}else{//110
							list_head* buddy_node = (void *)idx_to_ptr(buddy_ba_idx,mpid);
							delete_node(buddy_node);
							free_area_2[cur_order].nr_free--;
							cur_order--;
							int next_idx = pidx&buddy_block_idx;
							pidx = next_idx;
							block_node = (void *)idx_to_ptr(root_idx,mpid);
							add_to_list(block_node,&free_area_2[cur_order].free_list,free_area_2[cur_order].free_list.n);
							free_area_2[cur_order].nr_free++;
							bitmap_xor_and_test(bit_array_2,p_ba_idx);
							bitmap_xor_and_test(bit_array_2,root_idx);
							while(1){
								int root_buddy_idx = get_buddy_index(pidx,cur_order,mpid);
								int root_buddy_ba_idx = get_bit_idx(cur_order,root_buddy_idx,mpid);
								if(bitmap_scan_test(bit_array_2, root_buddy_ba_idx)){
									return RTX_OK;
								}else{
									list_head* root_buddy_node = (void *)idx_to_ptr(root_buddy_ba_idx,mpid);
									delete_node(root_buddy_node);
									delete_node(block_node);
									free_area_2[cur_order].nr_free-= 2;
									cur_order--;
									next_idx = pidx&root_buddy_idx;
									pidx = next_idx ;
									root_idx = get_bit_idx(cur_order,pidx,mpid);
									bitmap_xor_and_test(bit_array_2,root_idx);
									block_node = (void *)idx_to_ptr(root_idx,mpid);
									add_to_list(block_node,&free_area_2[cur_order].free_list,free_area_2[cur_order].free_list.n);
									free_area_2[cur_order].nr_free++;
									if(cur_order==0){
										return RTX_OK;
									}
								}
							}
						}
					}else{//100
						if(bitmap_scan_test(bit_array_2, buddy_ba_idx)){
							return -1;
						}
							if(cur_order ==1){
								bitmap_xor_and_test(bit_array_2,root_idx);
								add_to_list(block_node,&free_area_2[0].free_list,free_area_2[0].free_list.n);
								free_area_2[0].nr_free++;
							}else{
								cur_order--;
								continue;
							}
					}

				}

			}
		}
    return RTX_ERR; 
}

int k_mpool_dump (mpool_t mpid)   
{
#ifdef DEBUG_0
    printf("k_mpool_dump: mpid = %d\r\n", mpid);
#endif /* DEBUG_0 */
// output address: the address of the header of the block each line starts with the address of a free block header address
// The size of a memory block in the output includes the header size.
	int order;
	int free_list_count = 0;
	struct list_head* block_node;
	if(mpid==0){ // 1
		//printf("%d free memory block(s) found",free_list_count);
		
		for(order=0; order<=h_1;order++){ // 2
				if(list_first_entry_or_null(&free_area_1[order].free_list)){
						free_list_count += free_area_1[order].nr_free;
						block_node = free_area_1[order].free_list.n;
						while(1){
							if(block_node == &free_area_1[order].free_list){
								break;
							}
							
							printf("0x%x: 0x%x\r\n", (long)block_node, (1<<(h_1-order))*WORD_SIZE);
							block_node = block_node->n;
						}
				}
		}
		printf("%d free memory block(s) found\r\n",free_list_count);
	}else{
		for(order=0; order<=h_2;order++){ // 2
				if(list_first_entry_or_null(&free_area_2[order].free_list)){
						free_list_count += free_area_2[order].nr_free;
						block_node = free_area_2[order].free_list.n;
						while(1){
							if(block_node == &free_area_2[order].free_list){
								break;
							}
							
							printf("0x%x: 0x%x\r\n", (long)block_node, (1<<(h_2-order))*WORD_SIZE);
							block_node = block_node->n;
						}
				}
		}
		printf("%d free memory block(s) found\r\n",free_list_count);
	}
    return free_list_count;
}
 
int k_mem_init(int algo)
{
#ifdef DEBUG_0
    printf("k_mem_init: algo = %d\r\n", algo);
#endif /* DEBUG_0 */
        
    if ( k_mpool_create(algo, RAM1_START, RAM1_END) < 0 ) {
        return RTX_ERR;
    }
    
    if ( k_mpool_create(algo, RAM2_START, RAM2_END) < 0 ) {
        return RTX_ERR;
    }
    
    return RTX_OK;
}

/**
 * @brief allocate kernel stack statically
 */
U32* k_alloc_k_stack(task_t tid)
{
    
    if ( tid >= MAX_TASKS) {
        errno = EAGAIN;
        return NULL;
    }
    U32 *sp = g_k_stacks[tid+1];
    
    // 8B stack alignment adjustment
    if ((U32)sp & 0x04) {   // if sp not 8B aligned, then it must be 4B aligned
        sp--;               // adjust it to 8B aligned
    }
    return sp;
}

/**
 * @brief allocate user/process stack statically
 * @attention  you should not use this function in your lab
 */

U32* k_alloc_p_stack(task_t tid)
{
    if ( tid >= NUM_TASKS ) {
        errno = EAGAIN;
        return NULL;
    }
    
    U32 *sp = g_p_stacks[tid+1];
    
    
    // 8B stack alignment adjustment
    if ((U32)sp & 0x04) {   // if sp not 8B aligned, then it must be 4B aligned
        sp--;               // adjust it to 8B aligned
    }
    return sp;
}

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

