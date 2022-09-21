/** 
 * @brief The KCD Task Template File 
 * @note  The file name and the function name can be changed
 * @see   k_tasks.h
 */

#include "rtx.h"
#include "k_mem.h"

// this struct and queue was used as string
typedef struct node{
        struct node* next;
        char input;
    } node;

node* head;
node* tail;
int command_length = 0;

void init(){
    head = NULL;
    tail = NULL;
    command_length = 0;
}
// change malloc 
node* NewNode(char input){
    node* new = k_mpool_alloc(MPID_IRAM2,sizeof(node));
    new->next = NULL;
    new->input = input;
    return new;
}

void inQueue(char input){
    if(command_length == 0){
        head = NewNode(input);
        tail = head;
        command_length++;
    } else {
        node* tmp = NewNode(input);
        tail->next = tmp;
        tail = tmp;
        command_length++;
    }
}

char deQueue(){
    if(command_length != 1){
				if(command_length==0){
					return 0;
				}
        char tmp_char = head->input;
        node* tmp = head;
        head = head->next;
        k_mpool_dealloc(MPID_IRAM2,tmp);
        command_length--;
        return tmp_char;
    } else{
        char tmp_char = head->input;
        node* tmp = head;
        head = head->next;
        k_mpool_dealloc(MPID_IRAM2,tmp);
        head = NULL;
        tail = NULL;
        command_length--;
        return tmp_char;
    }
}

void task_kcd(void)
{
    // creating a mbx for the first time
    // ACSII array
    int ascii[74];
    // char command[10];
    // int command_len = 0;
		//printf("start running kcd\n");
    for (int i = 0; i < 74; i++){
        ascii[i] = 0;
    }
    // 'L' is reserved
    ascii[29] = TID_KCD;
    // 2?
    mbx_t a = mbx_create(KCD_MBX_SIZE);
    if(a == (mbx_t) -1){
        //failed to create a mbx (KCD)
        return;
    }
		init();
    while(1){
        // KCD responds for only KCD_REG and KCD_IN
        void* buf = k_mpool_alloc(MPID_IRAM2,sizeof(RTX_MSG_HDR) + 1);
        int recv_check = recv_msg(buf,sizeof(RTX_MSG_HDR) + 1);
        if(recv_check == -1){
            //failed to recv (KCD)
            return;
        }
        // offset by 5 to get type?
        // buf + 4 senderid
        // buf + 5 type   ((int*)buf)[5]
        // buf + 6 data
        int type= ((char*)buf)[5];
        int sender_id = ((char*)buf)[4];
        int get_char = ((char*)buf)[6];
        if(((char*)buf)[5] == KCD_REG){
            // mapping char to ascii code
            int index = ((char*)buf)[6];
            // assign corresponding tid input should not be 'L'
            if( index - 48 != 29 ){
                ascii[index - 48] = ((char*)buf)[4];
            }
        } else if (((char*)buf)[5] == KEY_IN){
            // if input is not 'enter'

            if(((char*)buf)[6] != 13){
                inQueue(((char*)buf)[6]);
                // send to display
                U8* buffer1 = k_mpool_alloc(MPID_IRAM2 ,sizeof(RTX_MSG_HDR) + 4);
                RTX_MSG_HDR* ptr = (void*)buffer1;
                ptr->length = sizeof(RTX_MSG_HDR) + 4;
                ptr->sender_tid = TID_KCD;
                ptr->type = DISPLAY;
				buffer1 += 6;  
				*buffer1 = ((char*)buf)[6];  
							*(++buffer1) = '\r';
							*(++buffer1) = '\n';
							*(++buffer1) = '\0';
                // ready to send
                send_msg(TID_CON, (void*)ptr);
                // deallocate
                k_mpool_dealloc(MPID_IRAM2, ptr);
            }
            // input is 'enter'
            else{
                // restore string

                char* string = k_mpool_alloc(MPID_IRAM2 ,command_length);
								int temp = command_length;
                for(int i = 0; i < temp; i++){
                    string[i] = deQueue();
                }
                // LT
                if (temp == 3 && string[0] == 0x25 && string[1] == 0x4c && string[2] == 0x54){
                    task_t buf[10];
                    for(int i = 0 ; i < 10; i++){
                        buf[i] = 0;
                    }
                    int num_task = tsk_ls(buf, 10);
                    for(int j = 0; j < num_task; j++){
                        char LT[33];
                        RTX_TASK_INFO a;
                        task_t tmp = buf[j];
                        tsk_get(tmp, &a);
                        sprintf(LT, "task ids: %d, task status: %d\r\n\0", buf[j], a.state);
                        U8* buffer_LT = k_mpool_alloc(MPID_IRAM2 ,sizeof(RTX_MSG_HDR) + 33);
                        RTX_MSG_HDR* ptr = (void*)buffer_LT;
                        ptr->length = sizeof(RTX_MSG_HDR) + 33;
                        ptr->sender_tid = TID_KCD;
                        ptr->type = DISPLAY;
                        buffer_LT += 6;  
                        for(int i = 0; i < 33; i++){
                            *buffer_LT = LT[i];
                            buffer_LT++;
                        }  
                        // ready to send
                        send_msg(TID_CON, (void*)ptr);
                        // deallocate
                        k_mpool_dealloc(MPID_IRAM2, ptr);
                    }
                } 
                // LM
                else if (temp== 3 && string[0] == 0x25 && string [1] == 0x4c && string[2] == 0x4d){
                    task_t buf_tsk[10];
                    for(int i = 0 ; i < 10; i++){
                        buf_tsk[i] = 0;
                    }
                    int num_task = tsk_ls(buf_tsk, 10);
                    for(int j = 0; j < num_task; j++){
                        char LM[59];
                        RTX_TASK_INFO a;
                        task_t tmp = buf_tsk[j];
                        tsk_get(tmp, &a);
                        int freespace = mbx_get(tmp);
											  if(freespace == RTX_ERR){
													sprintf(LM, "task ids: %d, task status: %d, mbx does not exist\r\n\0", buf_tsk[j], a.state);
												} else {
													sprintf(LM, "task ids: %d, task status: %d, mbx reamaining space: %d\r\n\0", buf_tsk[j], a.state, freespace);
												}
                        U8* buffer_LM = k_mpool_alloc(MPID_IRAM2 ,sizeof(RTX_MSG_HDR) + 59);
                        RTX_MSG_HDR* ptr = (void*)buffer_LM;
                        ptr->length = sizeof(RTX_MSG_HDR) + 59;
                        ptr->sender_tid = TID_KCD;
                        ptr->type = DISPLAY;
                        buffer_LM += 6;  
                        for(int i = 0; i < 59; i++){
                            *buffer_LM = LM[i];
                            buffer_LM++;
                        }  
                        // ready to send
                        send_msg(TID_CON, (void*)ptr);
                        // deallocate
                        k_mpool_dealloc(MPID_IRAM2, ptr);
                    }

                }
                else if (string[0] != 0x25){
                    char IC[19] = "Invalid command\r\n\0";
                    U8* buffer1 = k_mpool_alloc(MPID_IRAM2 ,sizeof(RTX_MSG_HDR) + 19);
                    RTX_MSG_HDR* ptr = (void*)buffer1;
                    ptr->length = sizeof(RTX_MSG_HDR) + 19;
                    ptr->sender_tid = TID_KCD;
                    ptr->type = DISPLAY;
                    buffer1 += 6;
                    //sprintf(buffer1, "Invalid command\n");
                    for (int i = 0; i < 19; i++){
                        *buffer1 = IC[i];
                        buffer1++;
                    }
                    //printf("Invalid command\n");
                    int send_IC = send_msg(TID_CON, ptr);
                    if (send_IC == -1){
                        tsk_yield();
                        continue;
                    }

                }
                // forward command
                else {
                    // find corresponding tasks and send_msg
									int tmp_index;
									if(temp>1){
									   tmp_index = (int) string[1] - 48;
									}else{
											tmp_index = 94-48;
									}

                    if(ascii[tmp_index] != 0 ){
                        //construct sending buf
                        U8* buffer2 = k_mpool_alloc(MPID_IRAM2 ,sizeof(RTX_MSG_HDR) + temp);
                        // U8* tmp = (void*) buffer2;
                        RTX_MSG_HDR* ptr = (void*)buffer2;
                        ptr->length = sizeof(RTX_MSG_HDR) + temp;
                        ptr->sender_tid = TID_KCD;
                        ptr->type = KCD_CMD;
                        buffer2 += 6;
                        for(int i = 2; i < temp; i++){
                            buffer2[0] = string[2];
                            buffer2++;
                        }
                        //ready to send
                        send_msg(ascii[tmp_index], ptr);
                        // deallocte
                        k_mpool_dealloc(MPID_IRAM2, ptr);
                    } 
                    // move below L185 
                    else if (ascii[tmp_index] == 0){
                        char CNF[21] = "Command not found\r\n\0";
                        U8* buffer_CNF = k_mpool_alloc(MPID_IRAM2 ,sizeof(RTX_MSG_HDR) + 21);
                        RTX_MSG_HDR* ptr = (void*)buffer_CNF;
                        ptr->length = sizeof(RTX_MSG_HDR) + 21;
                        ptr->sender_tid = TID_KCD;
                        ptr->type = DISPLAY;
                        buffer_CNF += 6;
                        //sprintf(buffer1, "Invalid command\n");
                        for (int i = 0; i < 21; i++){
                            *buffer_CNF = CNF[i];
                            buffer_CNF++;
                        }
                        int send_IC = send_msg(TID_CON, ptr);
                        if (send_IC == RTX_ERR){
                            tsk_yield();
                            continue;
                        }
                        k_mpool_dealloc(MPID_IRAM2, ptr);
                        //printf("Command not found\n");
                    }
                }
                k_mpool_dealloc(MPID_IRAM2, string);
            }
            
        }
        k_mpool_dealloc(MPID_IRAM2, buf);
     }
}

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

                                           