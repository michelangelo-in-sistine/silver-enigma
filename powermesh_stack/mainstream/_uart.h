/******************** (C) COPYRIGHT 2016 ***************************************
* File Name          : _uart.h
* Author             : Lv Haifeng
* Version            : V 2.0
* Date               : 2016-05-17
* Description        : 
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODECTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, BELLING INC. SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAINMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _UART_H
#define _UART_H

#ifdef USE_UART

void init_uart(void);
void my_printf(const char code * fmt, ...) reentrant;
void uart_rcv_int_svr(u8 rec_data);

void debug_uart_cmd_proc(void);
void uart_send_asc(ARRAY_HANDLE pt, u16 len) reentrant;
void print_phy_rcv(PHY_RCV_HANDLE pp) reentrant;

#if NODE_TYPE==NODE_MASTER
void print_transaction_timing(TIMER_ID_TYPE tid, u32 expiring_stick);
void print_psr_err(ERROR_STRUCT * psr_err);
void print_network(void);
void print_build_network(u8 phase);
void print_pipe(u16 pipe_id);
void print_uid(UID_HANDLE uid_handle);
void print_node(NODE_HANDLE node_handle);
void print_error_node(NODE_HANDLE error_node);
#endif 

#endif
#endif
