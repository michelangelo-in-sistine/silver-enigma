/******************** (C) COPYRIGHT 2012 Belling Inc. ********************
* File Name          : dll.h
* Author             : Lv Haifeng
* Version            : V1.0.3
* Date               : 10/28/2012
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
#ifndef _DLL_H
#define _DLL_H

/* Includes ------------------------------------------------------------------*/

/* Exported constants & macro --------------------------------------------------------*/

#define GET_DLL_HANDLE(pt)	&_dll_rcv_obj[pt->phase]		//得到同相的PHY对象

/* Exported types ------------------------------------------------------------*/


/* Exported variants ------------------------------------------------------- */
extern DLL_RCV_STRUCT xdata _dll_rcv_obj[];

/* Exported functions ------------------------------------------------------- */
void init_dll(void);
SEND_ID_TYPE dll_send(DLL_SEND_HANDLE pdss) reentrant;
DLL_RCV_HANDLE dll_rcv(void);
void dll_rcv_proc(DLL_RCV_HANDLE pp);
void dll_rcv_resume(DLL_RCV_HANDLE pd) reentrant;
STATUS dll_diag(DLL_SEND_HANDLE pdss, ARRAY_HANDLE buffer);
RESULT check_uid(u8 phase, ARRAY_HANDLE target_uid);
void set_ebc_response_enable(BOOL enable);
#endif






