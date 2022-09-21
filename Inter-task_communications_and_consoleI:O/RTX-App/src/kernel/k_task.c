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
 * @file        k_task.c
 * @brief       task management C file
 * @version     V1.2021.05
 * @authors     Yiqing Huang
 * @date        2021 MAY
 *
 * @attention   assumes NO HARDWARE INTERRUPTS
 * @details     The starter code shows one way of implementing context switching.
 *              The code only has minimal sanity check.
 *              There is no stack overflow check.
 *              The implementation assumes only three simple tasks and
 *              NO HARDWARE INTERRUPTS.
 *              The purpose is to show how context switch could be done
 *              under stated assumptions.
 *              These assumptions are not true in the required RTX Project!!!
 *              Understand the assumptions and the limitations of the code before
 *              using the code piece in your own project!!!
 *
 *****************************************************************************/


#include "k_inc.h"
#include "k_mem.h"
#include "k_task.h"
#include "k_rtx.h"
/*
 *==========================================================================
 *                            GLOBAL VARIABLES
 *==========================================================================
 */

TCB             *gp_current_task = NULL;    // the current RUNNING task
TCB             g_tcbs[MAX_TASKS];          // an array of TCBs
//TASK_INIT       g_null_task_info;           // The null task info
U32             g_num_active_tasks = 0;     // number of non-dormant tasks
TCB queue[4];
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
    g_k_stacks[MAX_TASKS-1]-->|---------------------------|     |
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
 *                            FUNCTIONS
 *===========================================================================
 */
void q_init_list_head(TCB* list){ //double linked list init
	list->next = list;	
	list->prev = list;
}	
 void *q_list_first_entry_or_null(TCB *head){
	return head->next == head ? NULL:head->next; // maybe you wen ti
}
 void __q_delete_node(TCB *prev,TCB *next){ // delete a node from the freelist
	prev->next = next;
	next->prev = prev;
} 

void q_delete_node(TCB *block){// delete a node from the freelist
	__q_delete_node(block->prev,block->next);
	block->prev = NULL;
	block->next = NULL;
}
TCB* q_delete_first_node(TCB* head){
	TCB* temp = head->next;
	if(!q_list_first_entry_or_null(head))
		return NULL;
	q_delete_node(head->next);
	return temp;
}
void q_add_to_list_last(TCB *new_node, TCB *head, TCB *prev) { // add entry to the dlist
    prev->next = new_node; 
    new_node->next = head; 
    new_node->prev = prev; 
    head->prev = new_node; 
} 
void q_add_to_list_head(TCB *new_node, TCB *prev, TCB *next) { // add entry to the dlist
    next->prev = new_node; 
    new_node->next = next; 
    new_node->prev = prev; 
    prev->next = new_node; 
} 
task_t get_valid_tid(void){
	for(int i = 1 ; i< MAX_TASKS; i++){
		if(g_tcbs[i].state == DORMANT){
			return i;
		}
	}
	return -1;
}
/*int check_if_prio_highest(U8 prio){
	for(int i =1 ;i<MAX_TASKS;i++){
		if(g_tcbs[i].state!=DORMANT&&g_tcbs[])
	}
}*/
/**************************************************************************//**
 * @brief   scheduler, pick the TCB of the next to run task
 *
 * @return  TCB pointer of the next to run task
 * @post    gp_curret_task is updated
 * @note    you need to change this one to be a priority scheduler
 *
 *****************************************************************************/

TCB *scheduler(void)
{
    /* *****MODIFY THIS FUNCTION ***** */

		int prio = gp_current_task->prio - HIGH;
		if(prio == 0x7F){
			prio = LOWEST -HIGH;
		}
			for(int i = HIGH;i <= LOWEST;i++){
				if(q_list_first_entry_or_null(&queue[i-HIGH])){
					return q_delete_first_node(&queue[i-HIGH]);			
				}
			}
			/*if(prio!=LOWEST+1 -HIGH){
			TCB* old_node = q_delete_first_node(&queue[prio]);
			q_add_to_list_last(old_node,&queue[prio],queue[prio].prev);
			return q_delete_first_node(&queue[prio]);
			}*/
			
    //return &g_tcbs[(++tid)%g_num_active_tasks];
		
	return gp_current_task;
}

/**
 * @brief initialzie the first task in the system
 */
void k_tsk_init_first(TASK_INIT *p_task)
{
    p_task->prio         = PRIO_NULL;
    p_task->priv         = 0;
    p_task->tid          = TID_NULL;
    p_task->ptask        = &task_null;
    p_task->u_stack_size = PROC_STACK_SIZE;
}

void k_tsk_init_kcd(TASK_INIT *p_task)
{
    p_task->prio         = HIGH;
    p_task->priv         = 0;
    p_task->tid          = TID_KCD;
    p_task->ptask        = &task_kcd;
    p_task->u_stack_size = PROC_STACK_SIZE;
}
void k_tsk_init_dsp(TASK_INIT *p_task)
{
    p_task->prio         = HIGH;
    p_task->priv         = 1;
    p_task->tid          = TID_CON;
    p_task->ptask        = &task_cdisp;
    p_task->u_stack_size = PROC_STACK_SIZE;
}

/**************************************************************************//**
 * @brief       initialize all boot-time tasks in the system,
 *
 *
 * @return      RTX_OK on success; RTX_ERR on failure
 * @param       task_info   boot-time task information structure pointer
 * @param       num_tasks   boot-time number of tasks
 * @pre         memory has been properly initialized
 * @post        none
 * @see         k_tsk_create_first
 * @see         k_tsk_create_new
 *****************************************************************************/

int k_tsk_init(TASK_INIT *task, int num_tasks)
{
		for(int i = 0; i< 4 ; i++){
		q_init_list_head(&queue[i]);
		}
    if (num_tasks > MAX_TASKS - 1&&task==NULL) {
			errno = EINVAL;
        return RTX_ERR;
    }
    
    TASK_INIT taskinfo;
		TASK_INIT taskinfo_kcd;
		TASK_INIT taskinfo_dsp;
		
    
    k_tsk_init_first(&taskinfo);
    if ( k_tsk_create_new(&taskinfo, &g_tcbs[TID_NULL], TID_NULL) == RTX_OK ) {
        g_num_active_tasks = 1;
        gp_current_task = &g_tcbs[TID_NULL];
    } else {
        g_num_active_tasks = 0;
        return RTX_ERR;
    }
		// kcd init
    k_tsk_init_kcd(&taskinfo_kcd);
    if ( k_tsk_create_new(&taskinfo_kcd, &g_tcbs[TID_KCD], TID_KCD) == RTX_OK ) {
        g_num_active_tasks = 2;
        gp_current_task = &g_tcbs[TID_KCD];
    } else {
        g_num_active_tasks = 0;
        return RTX_ERR;
    }
		//dsp init
		k_tsk_init_dsp(&taskinfo_dsp);
    if ( k_tsk_create_new(&taskinfo_dsp, &g_tcbs[TID_CON], TID_CON) == RTX_OK ) {
        g_num_active_tasks = 3;
        gp_current_task = &g_tcbs[TID_CON];
    } else {
        g_num_active_tasks = 0;
        return RTX_ERR;
    }
    // create the rest of the tasks
    for ( int i = 0; i < num_tasks; i++ ) {
        TCB *p_tcb = &g_tcbs[i+1];
        if (k_tsk_create_new(&task[i], p_tcb, i+1) == RTX_OK) {
            g_num_active_tasks++;
        }
				U8 prio = task[i].prio;
				if(prio != HIGH && prio!= MEDIUM&&prio!=LOW&&prio!=LOWEST){
					errno = EINVAL;
					return RTX_ERR;
				}
				q_add_to_list_last(p_tcb,&queue[prio-HIGH],queue[prio-HIGH].prev);
    }
    q_add_to_list_last(&g_tcbs[TID_KCD],&queue[0],queue[0].prev);
		q_add_to_list_last(&g_tcbs[TID_CON],&queue[0],queue[0].prev);
    return RTX_OK;
}
/**************************************************************************//**
 * @brief       initialize a new task in the system,
 *              one dummy kernel stack frame, one dummy user stack frame
 *
 * @return      RTX_OK on success; RTX_ERR on failure
 * @param       p_taskinfo  task initialization structure pointer
 * @param       p_tcb       the tcb the task is assigned to
 * @param       tid         the tid the task is assigned to
 *
 * @details     From bottom of the stack,
 *              we have user initial context (xPSR, PC, SP_USR, uR0-uR3)
 *              then we stack up the kernel initial context (kLR, kR4-kR12, PSP, CONTROL)
 *              The PC is the entry point of the user task
 *              The kLR is set to SVC_RESTORE
 *              20 registers in total
 * @note        YOU NEED TO MODIFY THIS FILE!!!
 *****************************************************************************/
int k_tsk_create_new(TASK_INIT *p_taskinfo, TCB *p_tcb, task_t tid)
{
    extern U32 SVC_RTE;

    U32 *usp;
    U32 *ksp;

    if (p_taskinfo == NULL || p_tcb == NULL)
    {
        return RTX_ERR;
    }

    p_tcb->tid   = tid;
    p_tcb->state = READY;
    p_tcb->prio  = p_taskinfo->prio;
    p_tcb->priv  = p_taskinfo->priv;
    p_tcb->ptask = p_taskinfo->ptask;
    /*---------------------------------------------------------------
     *  Step1: allocate user stack for the task
     *         stacks grows down, stack base is at the high address
     * ATTENTION: you need to modify the following three lines of code
     *            so that you use your own dynamic memory allocator
     *            to allocate variable size user stack.
     * -------------------------------------------------------------*/
    p_taskinfo->u_stack_size =  p_taskinfo->u_stack_size<PROC_STACK_SIZE ? PROC_STACK_SIZE:p_taskinfo->u_stack_size;
    usp = k_mpool_alloc(MPID_IRAM2,p_taskinfo->u_stack_size);             // ***you need to change this line***
		if(usp == NULL ){
			errno = ENOMEM;
			return -1;
		}
	p_tcb->u_sp = (U32)usp;
		usp = (U32*)((U32)usp+p_taskinfo->u_stack_size)-1;
    if (usp == NULL) {
        return RTX_ERR;
    }
		p_tcb->u_stack_size = p_taskinfo->u_stack_size;
		p_tcb->u_sp_base = (U32)(usp+1);
		
    /*-------------------------------------------------------------------
     *  Step2: create task's thread mode initial context on the user stack.
     *         fabricate the stack so that the stack looks like that
     *         task executed and entered kernel from the SVC handler
     *         hence had the exception stack frame saved on the user stack.
     *         This fabrication allows the task to return
     *         to SVC_Handler before its execution.
     *
     *         8 registers listed in push order
     *         <xPSR, PC, uLR, uR12, uR3, uR2, uR1, uR0>
     * -------------------------------------------------------------*/

    // if kernel task runs under SVC mode, then no need to create user context stack frame for SVC handler entering
    // since we never enter from SVC handler in this case
    
    *(--usp) = INITIAL_xPSR;             // xPSR: Initial Processor State
    *(--usp) = (U32) (p_taskinfo->ptask);// PC: task entry point
    
    // uR14(LR), uR12, uR3, uR3, uR1, uR0, 6 registers
    for ( int j = 0; j < 6; j++ ) {
        
#ifdef DEBUG_0
        *(--usp) = 0xDEADAAA0 + j;
#else
        *(--usp) = 0x0;
#endif
    }
        p_tcb->u_sp  = (U32)usp;
    // allocate kernel stack for the task
    ksp = k_alloc_k_stack(tid);

    if ( ksp == NULL ) {
        return RTX_ERR;
    }

    /*---------------------------------------------------------------
     *  Step3: create task kernel initial context on kernel stack
     *
     *         12 registers listed in push order
     *         <kLR, kR4-kR12, PSP, CONTROL>
     * -------------------------------------------------------------*/
    // a task never run before directly exit
    *(--ksp) = (U32) (&SVC_RTE);
    // kernel stack R4 - R12, 9 registers
#define NUM_REGS 9    // number of registers to push
      for ( int j = 0; j < NUM_REGS; j++) {        
#ifdef DEBUG_0
        *(--ksp) = 0xDEADCCC0 + j;
#else
        *(--ksp) = 0x0;
#endif
    }

    // put user sp on to the kernel stack
    *(--ksp) = (U32) usp;
    
    // save control register so that we return with correct access level
    if (p_taskinfo->priv == 1) {  // privileged 
        *(--ksp) = __get_CONTROL() & ~BIT(0); 
    } else {                      // unprivileged
        *(--ksp) = __get_CONTROL() | BIT(0);
    }

    p_tcb->msp = ksp;

    return RTX_OK;
}

/**************************************************************************//**
 * @brief       switching kernel stacks of two TCBs
 * @param       p_tcb_old, the old tcb that was in RUNNING
 * @return      RTX_OK upon success
 *              RTX_ERR upon failure
 * @pre         gp_current_task is pointing to a valid TCB
 *              gp_current_task->state = RUNNING
 *              gp_crrent_task != p_tcb_old
 *              p_tcb_old == NULL or p_tcb_old->state updated
 * @note        caller must ensure the pre-conditions are met before calling.
 *              the function does not check the pre-condition!
 * @note        The control register setting will be done by the caller
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * @attention   CRITICAL SECTION
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 *****************************************************************************/
__asm void k_tsk_switch(TCB *p_tcb_old)
{
        PRESERVE8
        EXPORT  K_RESTORE
        
        PUSH    {R4-R12, LR}                // save general pupose registers and return address
        MRS     R4, CONTROL                 
        MRS     R5, PSP
        PUSH    {R4-R5}                     // save CONTROL, PSP
                                           STR     SP, [R0, #TCB_MSP_OFFSET]   // save SP to p_old_tcb->msp
K_RESTORE
        LDR     R1, =__cpp(&gp_current_task)
        LDR     R2, [R1]
        LDR     SP, [R2, #TCB_MSP_OFFSET]   // restore msp of the gp_current_task
        POP     {R4-R5}
        MSR     PSP, R5                     // restore PSP
        MSR     CONTROL, R4                 // restore CONTROL
        ISB                                 // flush pipeline, not needed for CM3 (architectural recommendation)
        POP     {R4-R12, PC}                // restore general purpose registers and return address
}


__asm void k_tsk_start(void)
{
        PRESERVE8
        B K_RESTORE
}

/**************************************************************************//**
 * @brief       run a new thread. The caller becomes READY and
 *              the scheduler picks the next ready to run task.
 * @return      RTX_ERR on error and zero on success
 * @pre         gp_current_task != NULL && gp_current_task == RUNNING
 * @post        gp_current_task gets updated to next to run task
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * @attention   CRITICAL SECTION
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *****************************************************************************/
int k_tsk_run_new(void)
{
    TCB *p_tcb_old = NULL;
    
    if (gp_current_task == NULL) {
        return RTX_ERR;
    }
		
    p_tcb_old = gp_current_task;
		gp_current_task->u_sp = __get_PSP();
		
    gp_current_task = scheduler();
    
    if ( gp_current_task == NULL  ) {
        gp_current_task = p_tcb_old;        // revert back to the old task
        return RTX_ERR;
    }

    // at this point, gp_current_task != NULL and p_tcb_old != NULL
    if (gp_current_task != p_tcb_old) {
        gp_current_task->state = RUNNING;   // change state of the to-be-switched-in  tcb
  if(p_tcb_old->state!=DORMANT&&p_tcb_old->state!=BLK_SEND&&p_tcb_old->state!=BLK_RECV){
        p_tcb_old->state = READY;           // change state of the to-be-switched-out tcb
		}
        k_tsk_switch(p_tcb_old);            // switch kernel stacks       
    }

    return RTX_OK;
}

 
/**************************************************************************//**
 * @brief       yield the cpu
 * @return:     RTX_OK upon success
 *              RTX_ERR upon failure
 * @pre:        gp_current_task != NULL &&
 *              gp_current_task->state = RUNNING
 * @post        gp_current_task gets updated to next to run task
 * @note:       caller must ensure the pre-conditions before calling.
 *****************************************************************************/
int k_tsk_yield(void)
{
		if(gp_current_task ->tid == 0){
		  k_tsk_run_new();
			return 0;
		}
		q_add_to_list_last(gp_current_task,&queue[gp_current_task->prio-HIGH],queue[gp_current_task->prio-HIGH].prev);
    k_tsk_run_new();
		return 0;
}

/**
 * @brief   get task identification
 * @return  the task ID (TID) of the calling task
 */
task_t k_tsk_gettid(void)
{
    return gp_current_task->tid;
}

/*
 *===========================================================================
 *                             TO BE IMPLEMETED IN LAB2
 *===========================================================================
 */

int k_tsk_create(task_t *task, void (*task_entry)(void), U8 prio, U32 stack_size)
{
#ifdef DEBUG_0
    printf("k_tsk_create: entering...\n\r");
    printf("task = 0x%x, task_entry = 0x%x, prio=%d, stack_size = %d\n\r", task, task_entry, prio, stack_size);
#endif /* DEBUG_0 */
	if(prio!= HIGH && prio != MEDIUM && prio!= LOW && prio!= LOWEST){
		errno = EINVAL;
		return -1;
	}

		TASK_INIT p_taskinit;
	p_taskinit.prio = prio;
	p_taskinit.u_stack_size = stack_size;
	p_taskinit.priv = 0;
	task_t tid = get_valid_tid();
	if(tid==0xFF){
		errno = EAGAIN;
		return -1;
	}
	int ret = k_tsk_create_new(&p_taskinit, &g_tcbs[tid], tid);
	if(ret == -1){
		return ret;
	}
	//	U32 size = stack_size < PROC_STACK_SIZE ? PROC_STACK_SIZE : stack_size;
		*task = tid;
		int prio_index = prio - HIGH;
			q_add_to_list_last(&g_tcbs[*task], &queue[prio_index],queue[prio_index].prev);
			if(prio<gp_current_task->prio){
				q_add_to_list_last(gp_current_task, &queue[gp_current_task->prio-HIGH],queue[gp_current_task->prio-HIGH].prev);
				k_tsk_run_new();
				
			}
		g_num_active_tasks++;
    return RTX_OK;

}

void k_tsk_exit(void) 
{
#ifdef DEBUG_0
    printf("k_tsk_exit: entering...\n\r");
#endif /* DEBUG_0 */
		gp_current_task->state = DORMANT;
		k_mpool_dealloc(MPID_IRAM2,(void*)(gp_current_task->u_sp_base-gp_current_task->u_stack_size)); // debug dealloc!!!!!!
		//kernal stack settings!!!!!!!!!!!!!!
		k_mpool_dealloc(MPID_IRAM2,mailboxes[gp_current_task->tid].buffer);
		free_rb(&mailboxes[gp_current_task->tid]);
		g_num_active_tasks--;
		k_tsk_run_new();
    return;
}

int k_tsk_set_prio(task_t task_id, U8 prio) 
{
#ifdef DEBUG_0
    printf("k_tsk_set_prio: entering...\n\r");
    printf("task_id = %d, prio = %d.\n\r", task_id, prio);
#endif /* DEBUG_0 */
	if(prio!= HIGH && prio != MEDIUM && prio!= LOW && prio!= LOWEST ){
		errno =  EINVAL;
		return RTX_ERR;
	}
	if(task_id <1 || task_id>MAX_TASKS-1){
		errno=EINVAL;
		return RTX_ERR;
	}
	if(gp_current_task->priv == 0 && g_tcbs[task_id].priv == 1){
		errno = EPERM;
		return RTX_ERR;
	}
	if(g_tcbs[task_id].state== DORMANT){
		return 0;
	}
	if(task_id == gp_current_task->tid){
			if(prio > gp_current_task->prio){ //not sure!!!!
				q_add_to_list_last(&g_tcbs[task_id],&queue[prio-HIGH],queue[prio-HIGH].prev);
				gp_current_task->prio = prio;
				k_tsk_run_new();
			}else{
				gp_current_task->prio = prio;
			}
	}else{
			if(prio!=g_tcbs[task_id].prio){
				q_delete_node(&g_tcbs[task_id]);
				q_add_to_list_last(&g_tcbs[task_id],&queue[prio-HIGH],queue[prio-HIGH].prev);
				if(prio < gp_current_task->prio){
					q_add_to_list_head(gp_current_task,&queue[gp_current_task->prio-HIGH],queue[gp_current_task->prio-HIGH].next);
					g_tcbs[task_id].prio = prio;
					k_tsk_run_new();
				}else{
					g_tcbs[task_id].prio = prio;
				}
				
			}
	
    return RTX_OK;    
}
}
/**
 * @brief   Retrieve task internal information 
 * @note    this is a dummy implementation, you need to change the code
 */
int k_tsk_get(task_t tid, RTX_TASK_INFO *buffer)
{
#ifdef DEBUG_0
    printf("k_tsk_get: entering...\n\r");
    printf("tid = %d, buffer = 0x%x.\n\r", tid, buffer);
#endif /* DEBUG_0 */    
    if (buffer == NULL) {
			errno = EFAULT;
        return RTX_ERR;
    }
		if (tid>=MAX_TASKS||tid<0){
			errno = EINVAL;
			return RTX_ERR;
		}
    /* The code fills the buffer with some fake task information. 
       You should fill the buffer with correct information    */
    
    buffer->tid           = tid;
    buffer->prio          = g_tcbs[tid].prio;
    buffer->u_stack_size  = g_tcbs[tid].u_stack_size;
    buffer->priv          = g_tcbs[tid].priv;
    buffer->ptask         = g_tcbs[tid].ptask;
    buffer->k_sp          = (U32)g_tcbs[tid].msp;
    buffer->state         = g_tcbs[tid].state;
    buffer->u_sp_base     = g_tcbs[tid].u_sp_base;
    return RTX_OK;     
}

int k_tsk_ls(task_t *buf, size_t count){
#ifdef DEBUG_0
    printf("k_tsk_ls: buf=0x%x, count=%u\r\n", buf, count);
#endif /* DEBUG_0 */
    int tmp_count = 0;
	if(buf==NULL||count==0){
		errno = EFAULT;
		return -1;
	}
    for(int i = 0; i<MAX_TASKS; i++){
        if(g_tcbs[i].state!=DORMANT){
            buf[tmp_count] = i;
            tmp_count++;
            if(tmp_count==count){
                break;
            }
        }
    }
    return tmp_count<count?tmp_count:count;
}

int k_rt_tsk_set(TIMEVAL *p_tv)
{
#ifdef DEBUG_0
    printf("k_rt_tsk_set: p_tv = 0x%x\r\n", p_tv);
#endif /* DEBUG_0 */
    return RTX_OK;   
}

int k_rt_tsk_susp(void)
{
#ifdef DEBUG_0
    printf("k_rt_tsk_susp: entering\r\n");
#endif /* DEBUG_0 */
    return RTX_OK;
}

int k_rt_tsk_get(task_t tid, TIMEVAL *buffer)
{
#ifdef DEBUG_0
    printf("k_rt_tsk_get: entering...\n\r");
    printf("tid = %d, buffer = 0x%x.\n\r", tid, buffer);
#endif /* DEBUG_0 */    
    if (buffer == NULL) {
        return RTX_ERR;
    }   
    
    /* The code fills the buffer with some fake rt task information. 
       You should fill the buffer with correct information    */
    buffer->sec  = 0xABCD;
    buffer->usec = 0xEEFF;
    
    return RTX_OK;
}
/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

