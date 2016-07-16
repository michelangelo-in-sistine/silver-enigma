/******************** (C) COPYRIGHT 2016 ***************************************
* File Name          : _ptp.h
* Author             : Lv Haifeng
* Version            : V 2.0
* Date               : 2016-07-16
* Description        : 
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODECTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, BELLING INC. SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAINMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/
#ifndef _PTP_H
#define _PTP_H

extern PTP_STACK_RCV_STRUCT xdata _ptp_rcv_obj[CFG_PHASE_CNT];


/* Exported functions ------------------------------------------------------- */
void init_ptp(void);
void set_ptp_comm_mode(u8 comm_mode);
u8 get_ptp_comm_mode(void);
void ptp_rcv_resume(PTP_STACK_RCV_HANDLE pptp);
void ptp_rcv_proc(DLL_RCV_HANDLE pd);

#endif
