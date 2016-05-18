/******************** (C) COPYRIGHT 2012 Belling Inc. ********************
* File Name          : powermesh_management.h
* Author             : Lv Haifeng
* Version            : V 2.0
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
#ifndef _POWERMESH_MGNT_H
#define _POWERMESH_MGNT_H


/* Includes ------------------------------------------------------------------*/

/* Basic Exported types ------------------------------------------------------*/
STATUS mgnt_ping(u16 pipe_id, ARRAY_HANDLE return_buffer);
STATUS mgnt_diag(u16 pipe_id, DLL_SEND_HANDLE pds, u8 * pt_return_buffer, AUTOHEAL_LEVEL autoheal_level);
STATUS mgnt_ebc_broadcast(u16 pipe_id, EBC_BROADCAST_STRUCT * pt_ebc, u8 * num_bsrf, AUTOHEAL_LEVEL autoheal_level);
STATUS mgnt_ebc_identify(u16 pipe_id, EBC_IDENTIFY_STRUCT * pt_ebc, u8 * num_node, AUTOHEAL_LEVEL autoheal_level);
STATUS mgnt_ebc_acquire(u16 pipe_id, EBC_ACQUIRE_STRUCT * pt_ebc, ARRAY_HANDLE buffer, AUTOHEAL_LEVEL autoheal_level);
STATUS mgnt_ebc_set_build_id(u16 pipe_id, u8 build_id, AUTOHEAL_LEVEL autoheal_level);
STATUS mgnt_ebc_inphase_confirm(u16 pipe_id, EBC_BROADCAST_HANDLE pt_ebc, ARRAY_HANDLE target_uid, AUTOHEAL_LEVEL autoheal_level);
STATUS ebc_inphase_confirm(EBC_BROADCAST_STRUCT * pt_ebc, ARRAY_HANDLE target_uid);

#endif
