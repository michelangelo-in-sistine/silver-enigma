/******************** (C) COPYRIGHT 2016 ********************
* File Name          : app.h
* Author             : Lv Haifeng
* Version            : V1.0.3
* Date               : 2016-05-23
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
#ifndef _APP_H
#define _APP_H

void init_app(void);
STATUS save_id_info_into_app_nvr(void);

void set_comm_mode(u8 comm_mode);
STATUS call_vid_for_current_parameter(UID_HANDLE target_uid, u8 mask, s16* return_parameter);
void acp_rcv_proc(APP_RCV_HANDLE pt_app_rcv);


#endif
