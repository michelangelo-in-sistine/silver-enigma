/******************** (C) COPYRIGHT 2012 Belling Inc. ********************
* File Name          : mgnt.h
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
#ifndef _MGNT_H
#define _MGNT_H

/* Includes ------------------------------------------------------------------*/

/* Exported constants & macro --------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* Exported variants ------------------------------------------------------- */
extern APP_STACK_RCV_STRUCT	_app_rcv_obj[];
extern u8 xdata _user_data[];
extern u8 xdata _user_data_len;
/* Exported functions ------------------------------------------------------- */
void init_mgnt_app(void);
SEND_ID_TYPE app_send(APP_SEND_HANDLE pas);
BASE_LEN_TYPE app_rcv(APP_RCV_HANDLE pa);
void app_rcv_resume(APP_STACK_RCV_HANDLE pa) reentrant;
MGNT_RCV_HANDLE mgnt_rcv(void);
void mgnt_proc(MGNT_RCV_HANDLE pm);
void mgnt_rcv_resume(MGNT_RCV_HANDLE pm) reentrant;

#if APP_RCV_SS_SNR == 1
s8 get_phy_ss(PHY_RCV_HANDLE pp);
s8 get_phy_snr(PHY_RCV_HANDLE pp);
#endif


#if DEVICE_TYPE == DEVICE_CC
SEND_ID_TYPE app_uid_send(APP_SEND_STRUCT * pas);
STATUS app_transaction(APP_SEND_HANDLE pas, APP_RCV_HANDLE pv, BASE_LEN_TYPE asdu_uplink_len, u16 app_delay);
#ifdef USE_MAC
SEND_ID_TYPE app_send_ca(APP_SEND_HANDLE app_send_handle, u32 transaction_timing, u32 max_send_hold_timing);
#endif
#endif

#endif






