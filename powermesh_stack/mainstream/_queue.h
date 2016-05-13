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

#ifdef USE_MAC
BOOL is_queue_pending(u8 phase);
STATUS cancel_pending_queue(SEND_ID_TYPE sid);
void cancel_all_pending_queue(void);

#endif
#endif
