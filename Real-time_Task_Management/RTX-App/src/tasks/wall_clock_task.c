#include "rtx.h"
#include "uart_irq.h"
#include "uart_polling.h"
#include "rtx_errno.h"
#include "k_mem.h"
#include "k_msg.h"
#include "timer.h"
#include "k_inc.h"
long get_sec(int sec){
	return sec%60;
}
long get_min(int sec){
	return (sec/60)%60;
}
long get_hour(int sec){
	return (sec/3600)%24;
}
void task_wall_clock(void)
{
    // create mailbox 
    mbx_t a = mbx_create(CON_MBX_SIZE);
    if(a == (mbx_t) -1){
        //failed to create a mbx (KCD)
        return;
    }
    //
		long sec = 0;//not 0 for WS!!!!
    // only response to input
		int send_flag = 0;
		TIMEVAL tv;
		tv.sec = 1;
		tv.usec = 0;
		int add_flag = 0;
    rt_tsk_set(&tv);
		void* buf = k_mpool_alloc(MPID_IRAM2, KCD_CMD_BUF_SIZE);

    while(1){
			
        int recv_check = recv_msg_nb(buf, KCD_CMD_BUF_SIZE);
				send_flag = recv_check!=RTX_ERR?1:0;
        LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef*) LPC_UART0;

				int length = (((char *)buf)[3]<<24)+(((char*)buf)[2]<<16)+(((char*)buf)[1]<<8)+(((char*)buf)[0]);				
        if (length!=0&&((char*)buf)[5] == KCD_CMD){
            U8* finder = (U8*)buf;
						if(*(finder+6) == 0x52){//%WR
                            //\033\033[s\033[1;27HCommand not found\033[u\0
                            //%c%c:%c%c:%c%c
														if(send_flag){
																sec = 0;
														}
                            long send_sec = get_sec(sec);
                            U8 sec1 = send_sec%10+48;
                            U8 sec2 = send_sec/10+48;
                            long send_min = get_min(sec);
                            U8 min1 = send_min%10+48;
														U8 min2 = send_min/10+48;
                            long send_hour = get_hour(sec);
                            U8 hour1 = send_hour%10+48;
                            U8 hour2 = send_hour/10+48;
                                        //send to display
                            char time[23];

														U8* buffer_LT = k_mpool_alloc(MPID_IRAM2 ,sizeof(RTX_MSG_HDR) + 23);
														RTX_MSG_HDR* ptr = (void*)buffer_LT;
														ptr->length = sizeof(RTX_MSG_HDR) + 23;
														ptr->sender_tid = TID_WCLCK;
														ptr->type = DISPLAY;
														buffer_LT += 6;  
														sprintf(time, "\033\033[s\033[1;72H%c%c:%c%c:%c%c\033[u\0", hour2,hour1,min2,min1,sec2,sec1);

														for(int i = 0; i < 23; i++){
																*buffer_LT = time[i];
																buffer_LT++;
																
														}  

														send_msg(TID_CON, (void*) ptr);
														rt_tsk_susp();
														k_mpool_dealloc(MPID_IRAM2, ptr);
									}else if(*(finder+6) == 0x53){//%WS
								//get new sec from recv buf!!!
							                  U8 *time_locate = (U8*)(finder+7);
																if(send_flag){
																	sec = (time_locate[8]-48)+10*(time_locate[7]-48)+60*(time_locate[5]-48
																	)+600*(time_locate[4]-48)+3600*(time_locate[2]-48)+36000*(time_locate[1]-48);
																}
																//printf("%c%c:%c%c:%c%c\r\n", time_locate[0],time_locate[1],time_locate[3],time_locate[4],time_locate[6],time_locate[7]);
																long send_sec = get_sec(sec);
                                U8 sec1 = send_sec%10+48;
                                U8 sec2 = send_sec/10+48;
                                long send_min = get_min(sec);
                                U8 min1 = send_min%10+48;
                                U8 min2 = send_min/10+48;
                                long send_hour = get_hour(sec);
                                U8 hour1 = send_hour%10+48;
                                U8 hour2 = send_hour/10+48;
                                char time[23];
														U8* buffer_LT = k_mpool_alloc(MPID_IRAM2 ,sizeof(RTX_MSG_HDR) + 23);
														RTX_MSG_HDR* ptr = (void*)buffer_LT;
														ptr->length = sizeof(RTX_MSG_HDR) + 23;
														ptr->sender_tid = TID_WCLCK;
														ptr->type = DISPLAY;
														buffer_LT += 6;  
														sprintf(time, "\033\033[s\033[1;72H%c%c:%c%c:%c%c\033[u\0", hour2,hour1,min2,min1,sec2,sec1);

														for(int i = 0; i < 23; i++){
																*buffer_LT = time[i];
																buffer_LT++;
														}  
														send_msg(TID_CON, (void*) ptr);
														 rt_tsk_susp();
														k_mpool_dealloc(MPID_IRAM2, ptr);

						}else if(*(finder+6) == 0x54){//%WT
								//directly send a remove clock ansi
                            char time[23];

														U8* buffer_LT = k_mpool_alloc(MPID_IRAM2 ,sizeof(RTX_MSG_HDR) + 23);
														RTX_MSG_HDR* ptr = (void*)buffer_LT;
														ptr->length = sizeof(RTX_MSG_HDR) + 23;
														ptr->sender_tid = TID_WCLCK;
														ptr->type = DISPLAY;
														buffer_LT += 6;  
														sprintf(time, "\033\033[s\033[1;72H%c%c%c%c%c%c%c%c\033[u\0", 32,32,32,32,32,32,32,32);

														for(int i = 0; i < 23; i++){
																*buffer_LT = time[i];
																buffer_LT++;
																
														}  

														send_msg(TID_CON, (void*) ptr);
														rt_tsk_susp();
														k_mpool_dealloc(MPID_IRAM2, ptr);
						}else if(*(finder+6) == 0x57){//%W
										//no need to do anything to sec
																long send_sec = get_sec(sec);
                                U8 sec1 = send_sec%10+48;
                                U8 sec2 = send_sec/10+48;
                                long send_min = get_min(sec);
                                U8 min1 = send_min%10+48;
                                U8 min2 = send_min/10+48;
                                long send_hour = get_hour(sec);
                                U8 hour1 = send_hour%10+48;
                                U8 hour2 = send_hour/10+48;
                                char time[23];
														U8* buffer_LT = k_mpool_alloc(MPID_IRAM2 ,sizeof(RTX_MSG_HDR) + 23);
														RTX_MSG_HDR* ptr = (void*)buffer_LT;
														ptr->length = sizeof(RTX_MSG_HDR) + 23;
														ptr->sender_tid = TID_WCLCK;
														ptr->type = DISPLAY;
														buffer_LT += 6;  
														sprintf(time, "\033\033[s\033[1;72H%c%c:%c%c:%c%c\033[u\0", hour2,hour1,min2,min1,sec2,sec1);

														for(int i = 0; i < 23; i++){
																*buffer_LT = time[i];
																buffer_LT++;
														}  
														send_msg(TID_CON, (void*) ptr);
														 rt_tsk_susp();
														k_mpool_dealloc(MPID_IRAM2, ptr);
						}else{
						                        rt_tsk_susp();
						}


        }else{
							                        rt_tsk_susp();			
				}
                        sec++;

    }
		       k_mpool_dealloc(MPID_IRAM2, buf);
}

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

