/******************** (C) COPYRIGHT 2012 Belling Inc. ********************
* File Name          : dst.h
* Author             : Lv Haifeng
* Version            : V2.0
* Date               : 2013-03-14
* Description        : a customized (20,10) rs coder and decoder in GF(2^8)
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODECTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, BELLING INC. SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAINMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _DST_H
#define _DST_H

/* Includes ------------------------------------------------------------------*/

/* Exported constants & macro --------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/


/* Exported variants ------------------------------------------------------- */
#ifdef DEBUG_DST
extern u8 xdata debug_dst_fixed_window;
extern u8 xdata debug_dst_fixed_jumps;

#endif

/* Exported functions ------------------------------------------------------- */
void init_dst(void);
SEND_ID_TYPE dst_send(DST_SEND_HANDLE dst_handle) reentrant;
void dst_rcv_proc(DLL_RCV_HANDLE pd);

#if DEVICE_TYPE==DEVICE_CC || (defined DEVICE_READING_CONTROLLER) || DEVICE_TYPE==DEVICE_CV
s8 get_dst_index(u8 phase);
#endif

STATUS config_dst_flooding(COMM_MODE_TYPE comm_mode, u8 max_jumps, u8 max_window_index, u8 acps_constrained, u8 network_constrained);
#endif







