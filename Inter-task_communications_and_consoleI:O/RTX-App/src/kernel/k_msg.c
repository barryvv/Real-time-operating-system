/*
 ****************************************************************************
 *
 *                  UNIVERSITY OF WATERLOO ECE 350 RTX LAB  
 *
 *                     Copyright 2020-2022 Yiqing Huang
 *                          All rights reserved.
 *---------------------------------------------------------------------------
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
 *---------------------------------------------------------------------------*/
 

/**************************************************************************//**
 * @file        k_msg.c
 * @brief       kernel message passing routines          
 * @version     V1.2021.06
 * @authors     Yiqing Huang
 * @date        2021 JUN
 *****************************************************************************/

#include "k_inc.h"
#include "k_rtx.h"
#include "k_task.h"
#include "k_msg.h"
RB mailboxes[MAX_TASKS];

void init_rb (RB* rb, U32 size){

		rb->size = size;
		rb->roffset = 0;
		rb->writesize = 0;
		rb->readsize = 0;
		rb->woffset = 0;
}

void write_rb(RB* rb,U32 size, const void* buf){
		if(size+rb->woffset>rb->size){
			int first_size = rb->size - rb->woffset;
			int second_size = size - first_size;
			cpymem(((char *)rb->buffer+rb->woffset),buf,first_size);
			cpymem((char *)rb->buffer,(char *)buf+first_size,second_size);
			rb->woffset = second_size;

		}
		else{
			cpymem(((char *)rb->buffer+rb->woffset),buf,size);
			rb->woffset+= size;
		}
			rb->writesize += size;
}
void read_rb(RB* rb,U32 size, void* buf){
			if(size+rb->roffset>rb->size){
			int first_size = rb->size - rb->roffset;
			int second_size = size - first_size;
			cpymem(buf,((char *)rb->buffer+rb->roffset),first_size);
			cpymem((char *)buf+first_size,(char *)rb->buffer,second_size);
			rb->roffset = second_size;
		}else{
			cpymem(buf,((char *)rb->buffer+rb->roffset),size);
			rb->roffset+= size;
		}
			rb->readsize += size;
}
int check_rb_empty(RB* rb){
	return rb->writesize == rb->readsize;
}
int check_rb_full(RB* rb){
	return rb->writesize-rb->readsize == rb->size;
}
int get_rb_free_size(RB* rb){
	return rb->size-(rb->writesize-rb->readsize);
}
void cpymem(void *dest, const void *src, size_t count)
{
 char *tmp = dest;
 const char *s = src;
 while (count--)
  *tmp++ = *s++ ;
}
void setmem(void *dst, int val, size_t count){
	char* ret = (char*)dst;
	while (count--)
	{
		*ret++ = (char)val;
	}
}
int check_queue_empty(task_t tid){
		for(int i = HIGH;i <= LOWEST;i++){
				if(q_list_first_entry_or_null(&(mailboxes[tid].queue[i-HIGH]))){
					return 0;		
				}
			}
				return 1;
		}
TCB* get_waiting_highest(task_t tid, int type){
		for(int i = HIGH;i <= LOWEST;i++){
				if(q_list_first_entry_or_null(&(mailboxes[tid].queue[i-HIGH]))){
					return q_list_first_entry_or_null(&(mailboxes[tid].queue[i-HIGH]));	
				}
			}
				return NULL;
}
int get_msg_size(RB* rb){
 int size = 0;
  int count = 0;
  if(rb->roffset+4>rb->size){
   int first_size = rb->size - rb->roffset;
   int second_size = 4 - first_size;
   int tmp = rb->roffset;
   for(int i = 0;i<first_size;i++){
    size+= ((char *)rb->buffer+tmp)[i]<<(count*8);    
    count++;
   }
   for(int i =0;i<second_size;i++){
    size+= ((char *)rb->buffer)[i]<<(count*8);
    count++;
   }
  }else{
   for(int i =0;i<4;i++){
    size+= ((char *)rb->buffer+rb->roffset)[i]<<(count*8);   
    count++;
   };
  }
  return size;
}
int get_waiting_size(task_t id){
		return g_tcbs[id].length_of_task_buf;
}

void free_rb(RB* rb){
		for(int i =0; i<4;i++){
			q_init_list_head(&(rb->queue[i]));
		}
		rb->buffer = NULL;
		rb->readsize = 0;
		rb->roffset = 0;
		rb->size = 0;
		rb->woffset = 0;
		rb->writesize = 0;
}
int k_mbx_create(size_t size) {
#ifdef DEBUG_0
    printf("k_mbx_create: size = %u\r\n", size);
#endif /* DEBUG_0 */
		if(mailboxes[gp_current_task->tid].size != 0){
			errno = EEXIST;
			return RTX_ERR;
		}
		if(size < MIN_MSG_SIZE){
			errno = EINVAL;
			return RTX_ERR;
		}
		mailboxes[gp_current_task->tid].buffer = k_mpool_alloc(MPID_IRAM2, size);

		if(mailboxes[gp_current_task->tid].buffer==NULL){
			errno = ENOMEM;
			return RTX_ERR;
		}
		init_rb(&mailboxes[gp_current_task->tid],size);
	for(int i = 0 ;i<4;i++){
		q_init_list_head(&mailboxes[gp_current_task->tid].queue[i]);
	}
    return gp_current_task->tid;
}

int k_send_msg(task_t receiver_tid, const void *buf) {
#ifdef DEBUG_0
    printf("k_send_msg: receiver_tid = %d, buf=0x%x\r\n", receiver_tid, buf);
#endif /* DEBUG_0 */
		if(g_tcbs[receiver_tid].state==DORMANT||receiver_tid<1||receiver_tid>MAX_TASKS-1){
			errno = EINVAL;
			return RTX_ERR;
		}
		if(buf==NULL){
			errno = EFAULT;
			return RTX_ERR;
		}
		if(mailboxes[receiver_tid].size == 0){
			errno = ENOENT;
			return RTX_ERR;
		}
		int length = (((char *)buf)[3]<<24)+(((char*)buf)[2]<<16)+(((char*)buf)[1]<<8)+(((char*)buf)[0]);
		if(length<MIN_MSG_SIZE){
			errno = EINVAL;
			return RTX_ERR;
		}
		if(length>mailboxes[receiver_tid].size){
			errno = EMSGSIZE;
			return RTX_ERR;
		}
		gp_current_task->length_of_task_buf = length;
		int cprio = gp_current_task->prio;
	while(1){
	TCB* waiting_send = get_waiting_highest(receiver_tid,BLK_SEND);
	if(get_rb_free_size(&mailboxes[receiver_tid])>=length&&waiting_send==NULL){
			write_rb(&mailboxes[receiver_tid],length,buf);
			printf("tid %d send_msg %d size to %d successed\n",gp_current_task->tid,length,receiver_tid);
	}else{
			gp_current_task->state = BLK_SEND;
					printf("tid %d send_msg %d size to %d blocked\n",gp_current_task->tid,length,receiver_tid);
			q_add_to_list_last(gp_current_task,&(mailboxes[receiver_tid].queue[cprio-HIGH]),mailboxes[receiver_tid].queue[cprio-HIGH].prev);
			k_tsk_run_new();
			printf("tid %d send_msg to %d unblocked\n",gp_current_task->tid,length,receiver_tid);
			continue;
		}
		//TCB* waiting_recv = get_waiting_highest(receiver_tid,BLK_RECV);
	if(g_tcbs[receiver_tid].state == BLK_RECV){
			g_tcbs[receiver_tid].state = READY;
			q_add_to_list_last(&g_tcbs[receiver_tid],&queue[g_tcbs[receiver_tid].prio-HIGH],queue[g_tcbs[receiver_tid].prio-HIGH].prev);
			printf("tid %d unblocked the reciever %d\n",gp_current_task->tid,receiver_tid);
	}
    return RTX_OK;
}
}

int k_send_msg_nb(task_t receiver_tid, const void *buf) {
#ifdef DEBUG_0
    printf("k_send_msg_nb: receiver_tid = %d, buf=0x%x\r\n", receiver_tid, buf);
#endif /* DEBUG_0 */
		if(g_tcbs[receiver_tid].state==DORMANT||receiver_tid<1||receiver_tid>MAX_TASKS-1){
			errno = EINVAL;
			return RTX_ERR;
		}
		if(buf==NULL){
			errno = EFAULT;
			return RTX_ERR;
		}
		if(mailboxes[receiver_tid].size == 0){
			errno = ENOENT;
			return RTX_ERR;
		}
		int length = (((char *)buf)[3]<<24)+(((char*)buf)[2]<<16)+(((char*)buf)[1]<<8)+(((char*)buf)[0]);
		if(length<MIN_MSG_SIZE){
			errno = EINVAL;
			return RTX_ERR;
		}
		if(length>mailboxes[receiver_tid].size){
			errno = EMSGSIZE;
			return RTX_ERR;
		}
		gp_current_task->length_of_task_buf = length;
		TCB* waiting_send = get_waiting_highest(receiver_tid,BLK_SEND);
	int free_size = get_rb_free_size(&mailboxes[receiver_tid]);
	if(free_size>=length&&waiting_send==NULL){
			write_rb(&mailboxes[receiver_tid],length,buf);
	}else{
			errno = ENOSPC;
			return RTX_ERR;
		}
		//TCB* waiting_recv = get_waiting_highest(receiver_tid,BLK_RECV);
	if(g_tcbs[receiver_tid].state == BLK_RECV){
			g_tcbs[receiver_tid].state = READY;
			q_add_to_list_last(&g_tcbs[receiver_tid],&queue[g_tcbs[receiver_tid].prio-HIGH],queue[g_tcbs[receiver_tid].prio-HIGH].prev);
			printf("unblocked %d receiver\n",receiver_tid);
	}
    return RTX_OK;
}

int k_recv_msg(void *buf, size_t len) {
#ifdef DEBUG_0
    printf("k_recv_msg: buf=0x%x, len=%d\r\n", buf, len);
#endif /* DEBUG_0 */
		int ctid = gp_current_task->tid;
		if(buf==NULL){
			errno = EFAULT;
			return RTX_ERR;
		}
		if(mailboxes[ctid].size==0){
			errno = ENOENT;
			return RTX_ERR;
		}
	if(!check_rb_empty(&mailboxes[ctid])){
			int length = get_msg_size(&mailboxes[ctid]);
			if(len<length){
				errno = ENOSPC;
				return RTX_ERR;
			}
			read_rb(&mailboxes[ctid],length,buf);
					printf("%d receive %d bytes successed\n",gp_current_task->tid ,length);
	}else{
		gp_current_task->state = BLK_RECV;
					printf("%d receiveer blocked\n",gp_current_task->tid);
			k_tsk_run_new();
					int length = get_msg_size(&mailboxes[ctid]);
			printf("%d receiveer %d unblocked\n",gp_current_task->tid,length);

			read_rb(&mailboxes[ctid],length,buf);
			printf("%d receive %d bytes successed\n",gp_current_task->tid ,length);
		}

		TCB* waiting_task = get_waiting_highest(ctid,BLK_SEND);
		if(waiting_task){

			int waiting_size = waiting_task->length_of_task_buf;
			int waiting_prio = waiting_task->prio - HIGH;
			if(get_rb_free_size(&mailboxes[ctid])>=waiting_size){
				q_delete_node(waiting_task);
				waiting_task->state = READY;
				q_add_to_list_last(waiting_task,&queue[waiting_prio],queue[waiting_prio].prev);
				//printf("unblocked %d sender\n",waiting_task->tid);
			}
		}
    return RTX_OK;
}

int k_recv_msg_nb(void *buf, size_t len) {
#ifdef DEBUG_0
    printf("k_recv_msg_nb: buf=0x%x, len=%d\r\n", buf, len);
#endif /* DEBUG_0 */
		int ctid = gp_current_task->tid;
		if(buf==NULL){
			errno = EFAULT;
			return RTX_ERR;
		}
		if(mailboxes[ctid].size==0){
			errno = ENOENT;
			return RTX_ERR;
		}

	if(!check_rb_empty(&mailboxes[ctid])){
			int length = get_msg_size(&mailboxes[ctid]);
			if(len<length){
				errno = ENOSPC;
				return RTX_ERR;
			}
			read_rb(&mailboxes[ctid],length,buf);
			printf("%d receive %d bytes successed\n",gp_current_task->tid ,length);
	}else{
		errno = ENOMSG;
		return RTX_ERR;
		}
		TCB* waiting_task = get_waiting_highest(ctid,BLK_SEND);
		if(waiting_task){
			int waiting_size = waiting_task->length_of_task_buf;
			int waiting_prio = waiting_task->prio - HIGH;
			if(get_rb_free_size(&mailboxes[ctid])>=waiting_size){
				q_delete_node(waiting_task);
				waiting_task->state = READY;

				q_add_to_list_last(waiting_task,&queue[waiting_prio],queue[waiting_prio].prev);

			}
		}
    return RTX_OK;
}
int k_recv_uart(U8* buf, size_t len){
		if(!check_rb_empty(&uart_mb)){
			int length = get_msg_size(&uart_mb);
			read_rb(&uart_mb,length,buf);
			return RTX_OK;
	}else{
			return RTX_ERR;
	}
}
int k_send_to_uart(const void* buf){
		int length = (((char *)buf)[3]<<24)+(((char*)buf)[2]<<16)+(((char*)buf)[1]<<8)+(((char*)buf)[0]);
		gp_current_task->length_of_task_buf = length;
	int free_size = get_rb_free_size(&uart_mb);
	if(free_size>=length){
			write_rb(&uart_mb,length,buf);
	}else{
			errno = ENOSPC;
			return RTX_ERR;
		}
		//TCB* waiting_recv = get_waiting_highest(receiver_tid,BLK_RECV);

    return RTX_OK;
}
int k_mbx_ls(task_t *buf, size_t count) {
#ifdef DEBUG_0
    printf("k_mbx_ls: buf=0x%x, count=%u\r\n", buf, count);
#endif /* DEBUG_0 */
	    int tmp_count = 0;
	if(buf==NULL||count==0){
		errno = EFAULT;
		return -1;
	}
	    for(int i = 0; i<MAX_TASKS; i++){
        if(mailboxes[i].size!=0){
            buf[tmp_count] = i;
            tmp_count++;
            if(tmp_count==count){
                break;
            }
        }
    }
    return tmp_count<count?tmp_count:count;
}

int k_mbx_get(task_t tid)
{
#ifdef DEBUG_0
    printf("k_mbx_get: tid=%u\r\n", tid);
#endif /* DEBUG_0 */
		if(mailboxes[tid].size == 0){
			errno = ENOENT;
			return RTX_ERR;
		}
		return get_rb_free_size(&mailboxes[tid]);
}
/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

