/**
 * @brief The Console Display Task Template File 
 * @note  The file name and the function name can be changed
 * @see   k_tasks.h
 */

#include "rtx.h"
#include "uart_irq.h"
#include "uart_polling.h"
#include "rtx_errno.h"
#include "k_mem.h"
#include "k_msg.h"
void task_cdisp(void)
{
    // create mailbox 
    mbx_t a = mbx_create(CON_MBX_SIZE);
    if(a == (mbx_t) -1){
        //failed to create a mbx (KCD)
        return;
    }

    // only response to input

    while(1){

		    void* buf = k_mpool_alloc(MPID_IRAM2, KCD_CMD_BUF_SIZE);
        int recv_check = recv_msg(buf, KCD_CMD_BUF_SIZE);
        if (recv_check == RTX_ERR){
            tsk_yield();
            continue;
        }
        LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef*) LPC_UART0;

        if (((char*)buf)[5] == DISPLAY){
            U8* finder = (U8*)buf;
            finder += 4;
            *finder = TID_CON;
						int length = (((char *)buf)[3]<<24)+(((char*)buf)[2]<<16)+(((char*)buf)[1]<<8)+(((char*)buf)[0]);				
            int ret = k_send_to_uart( buf);
						if(ret==RTX_OK){
						    finder += 2;
								pUart->THR = finder[0];
								pUart->IER |= IER_THRE;

							while(pUart->IER != 0x05){
                continue;
            }
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

