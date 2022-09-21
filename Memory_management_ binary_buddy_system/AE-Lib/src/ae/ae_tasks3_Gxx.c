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
 * @file        ae_tasks100.c
 * @brief       P1 public testing suite 100
 *
 * @version     V1.2022.05
 * @authors     Yiqing Huang
 * @date        2022 May
 * *
 *****************************************************************************/

#include "ae_tasks.h"
#include "uart_polling.h"
#include "printf.h"
#include "ae.h"
#include "ae_util.h"
#include "ae_tasks_util.h"

/*
 *===========================================================================
 *                             MACROS
 *===========================================================================
 */
    
#define NUM_TESTS       1       // number of tests
#define NUM_INIT_TASKS  1       // number of tasks during initialization

/*
 *===========================================================================
 *                             GLOBAL VARIABLES 
 *===========================================================================
 */

TASK_INIT    g_init_tasks[NUM_INIT_TASKS];
const char   PREFIX[]      = "G99-TS100";
const char   PREFIX_LOG[]  = "G99-TS100-LOG ";
const char   PREFIX_LOG2[] = "G99-TS100-LOG2";

AE_XTEST     g_ae_xtest;                // test data, re-use for each test
AE_CASE      g_ae_cases[NUM_TESTS];
AE_CASE_TSK  g_tsk_cases[NUM_TESTS];

void set_ae_init_tasks (TASK_INIT **pp_tasks, int *p_num)
{
    *p_num = NUM_INIT_TASKS;
    *pp_tasks = g_init_tasks;
    set_ae_tasks(*pp_tasks, *p_num);
}

// initial task configuration
void set_ae_tasks(TASK_INIT *tasks, int num)
{
    for (int i = 0; i < num; i++ ) {                                                 
        tasks[i].u_stack_size = PROC_STACK_SIZE;    
        tasks[i].prio = HIGH + i;
        tasks[i].priv = 1;
    }
    tasks[0].priv  = 1;
    tasks[0].ptask = &priv_task1;
    
    init_ae_tsk_test();
}

void init_ae_tsk_test(void)
{
    g_ae_xtest.test_id = 0;
    g_ae_xtest.index = 0;
    g_ae_xtest.num_tests = NUM_TESTS;
    g_ae_xtest.num_tests_run = 0;
    
    for ( int i = 0; i< NUM_TESTS; i++ ) {
        g_tsk_cases[i].p_ae_case = &g_ae_cases[i];
        g_tsk_cases[i].p_ae_case->results  = 0x0;
        g_tsk_cases[i].p_ae_case->test_id  = i;
        g_tsk_cases[i].p_ae_case->num_bits = 0;
        g_tsk_cases[i].pos = 0;  // first avaiable slot to write exec seq tid
        // *_expt fields are case specific, deligate to specific test case to initialize
    }
    printf("%s: START\r\n", PREFIX);
}

void update_ae_xtest(int test_id)
{
    g_ae_xtest.test_id = test_id;
    g_ae_xtest.index = 0;
    g_ae_xtest.num_tests_run++;
}

void gen_req0(int test_id)
{
    g_tsk_cases[test_id].p_ae_case->num_bits = 5;  
    g_tsk_cases[test_id].p_ae_case->results = 0;
    g_tsk_cases[test_id].p_ae_case->test_id = test_id;
    g_tsk_cases[test_id].len = 16; // assign a value no greater than MAX_LEN_SEQ
    g_tsk_cases[test_id].pos_expt = 0; // N/A for P1 tests
       
    g_ae_xtest.test_id = test_id;
    g_ae_xtest.index = 0;
    g_ae_xtest.num_tests_run++;
}

/**
 * @brief a basic memory test on IRAM2
 */

#ifdef ECE350_P1
int test0_start(int test_id)
{
    static void *p[4];
    int ret_val    = 10;
    U8 *p_index    = &(g_ae_xtest.index);
    int sub_result = 0;
    
    gen_req0(test_id);
     
    for ( int i = 0; i < 4; i++ ) {
        p[i] = (void *)0x1;
    }

    //test 0-[0]
    p[0] = mem_alloc(0x1000);
    
    *p_index = 0;
    strcpy(g_ae_xtest.msg, "Allocating all the RAM, check non-NULL return");
    sub_result = (p[0] == NULL) ? 0 : 1;
    process_sub_result(test_id, *p_index, sub_result);  
    
    //test 0-[1]
    (*p_index)++;
    ret_val = mem_dump();
    strcpy(g_ae_xtest.msg, "Check mem2_dump return value is 1");
    sub_result = (ret_val == 0) ? 1: 0;
    process_sub_result(test_id, *p_index, sub_result);
    
    //test 0-[2]
		
    (*p_index)++;
    mem_dealloc(p[0]);
    p[0] = NULL;
    ret_val = mem_dump();
    strcpy(g_ae_xtest.msg, "Free p[0], checking mem2_dump return val");
				printf("\n%d hhhhhhh %d\n",p[0],ret_val);
    sub_result = (ret_val == 1) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);
		
		//test 0-[3]
    (*p_index)++;
    ret_val = mem_dealloc((void*)RAM1_START);
    strcpy(g_ae_xtest.msg, "Deallocating again, check error return");
    sub_result = (ret_val == -1) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);
		
    //test 0-[4]
    (*p_index)++;
    ret_val = mem_dump();
    strcpy(g_ae_xtest.msg, "Check mem2_dump return value is zero");
    sub_result = (ret_val == 1) ? 1: 0;
    process_sub_result(test_id, *p_index, sub_result);
 
		

		
		
    
}
#else 
int test0_start(int test_id)
{
    return RTX_ERR;
}
#endif // ECE350_P1


/**************************************************************************//**
 * @brief       a task that prints AAAAA, BBBBB on each line.
 *              it yields the cpu after each line
 *****************************************************************************/

void priv_task1(void)
{
    int test_id = 0;
    
    printf("%s: priv_task1: basic memory test on IRAM2\r\n", PREFIX_LOG2);
#ifdef ECE350_P1    
    test0_start(test_id);
#endif // ECE350_P1
    test_exit();
}




/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */
