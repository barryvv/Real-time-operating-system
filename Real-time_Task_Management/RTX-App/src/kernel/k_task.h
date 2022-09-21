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
 * @file        k_task.h
 * @brief       Task Management Header File
 *
 * @version     V1.2021.05
 * @authors     Yiqing Huang
 * @date        2021 MAY
 *
 * @details
 * @note        Starter code assumes there are only two tasks and a null task
 *
 *****************************************************************************/

#ifndef K_TASK_H_
#define K_TASK_H_

#include "k_inc.h"

/*
 *===========================================================================
 *                             MACROS
 *===========================================================================
 */

#define INITIAL_xPSR 0x01000000        /* user process initial xPSR value */

/*
 *==========================================================================
 *                            GLOBAL VARIABLES
 *==========================================================================
 */

extern TCB *gp_current_task;
extern TCB queue[4];
/*
 *===========================================================================
 *                            FUNCTION PROTOTYPES
 *===========================================================================
 */

extern void task_null	(void);         /* added in lab2 */
extern void task_kcd    (void);         /* added in lab3 */
extern void task_cdisp  (void);         /* added in lab3 */
extern void task_wall_clock(void);      /* added in lab4 */


// Implemented by Starter Code
int update_timeout_and_release(int diff,uint32_t current_usec);
void q_init_list_head(TCB* list);
void *q_list_first_entry_or_null(TCB *head);
void q_add_to_list_last(TCB *new_node, TCB *head, TCB *prev);
 void __q_delete_node(TCB *prev,TCB *next);
void q_delete_node(TCB *block);
TCB* q_delete_first_node(TCB* head);
int  k_tsk_init         (TASK_INIT *task_info, int num_tasks);
                                 /* initialize all tasks in the system */
int  k_tsk_create_new   (TASK_INIT *p_taskinfo, TCB *p_tcb, task_t tid);
                                 /* create a new task with initial context sitting on a dummy stack frame */
TCB  *scheduler         (void);  /* return the TCB of the next ready to run task */
void k_tsk_switch       (TCB *); /* kernel thread context switch, two stacks */
int  k_tsk_run_new      (void);  /* kernel runs a new thread  */
int  k_tsk_yield        (void);  /* kernel tsk_yield function */
void task_null          (void);  /* the null task */
void k_tsk_init_first   (TASK_INIT *p_task);    /* init the first task */
void k_tsk_start        (void);  /* start the first task */
task_t k_tsk_gettid     (void);  /* get tid of the current running task */

// Not implemented, to be done by students
int  k_tsk_create       (task_t *task, void (*task_entry)(void), U8 prio, U32 stack_size);
void k_tsk_exit         (void);
int  k_tsk_set_prio     (task_t task_id, U8 prio);
int  k_tsk_get          (task_t task_id, RTX_TASK_INFO *buffer);
TCB  *scheduler         (void);  /* student needs to change this function */
int  k_tsk_ls           (task_t *buf, size_t count);
//int  k_rt_tsk_set       (TASK_RT *p_rt_task);
int  k_rt_tsk_set       (TIMEVAL *p_tv);
int  k_rt_tsk_susp      (void);
int  k_rt_tsk_get       (task_t task_id, TIMEVAL *buffer);
#endif // ! K_TASK_H_

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

