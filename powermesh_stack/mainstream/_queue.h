/******************** (C) COPYRIGHT 2016 ***************************************
* File Name          : queue.h
* Author             : Lv Haifeng
* Version            : V 1.6.0
* Date               : 02/26/2013
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
#ifndef _QUEUE_H
#define _QUEUE_H
/* Includes ------------------------------------------------------------------*/
//#include "phy.h"

/* Basic Exported types ------------------------------------------------------*/


/* Exported Macros ------------------------------------------------------- */



/* Exported functions ------------------------------------------------------- */
void init_queue(void);
u8 check_available_queue(void);
SEND_ID_TYPE req_send_queue(void);
STATUS push_send_queue(SEND_ID_TYPE sid, PHY_SEND_HANDLE pss) reentrant;
void send_queue_proc(void);
SEND_STATUS get_send_status(SEND_ID_TYPE sid);

#ifdef USE_MAC
BOOL is_queue_pending(void);
STATUS cancel_pending_queue(SEND_ID_TYPE sid);
void cancel_all_pending_queue(void);
SEND_ID_TYPE is_queue_holding(void);
STATUS set_queue_mac_hold(SEND_ID_TYPE sid, BOOL enable, u32 nav_timing);
u8 get_queue_phase(SEND_ID_TYPE sid);
u16 get_queue_nav_value(SEND_ID_TYPE sid);
u32 calc_queue_sticks(SEND_ID_TYPE sid);
#endif
void set_queue_delay(SEND_ID_TYPE sid, u32 delay);

#endif
