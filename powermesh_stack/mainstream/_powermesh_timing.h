/******************** (C) COPYRIGHT 2012 Belling Inc. ********************
* File Name          : powermesh_timing.h
* Author             : Lv Haifeng
* Version            : V 2.0
* Date               : 2013-03-11
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
#ifndef _POWERMESH_TIMING_H
#define _POWERMESH_TIMING_H

/* Includes ------------------------------------------------------------------*/


/* Exported Functions --------------------------------------------------------*/
u32 scan_expiring_sticks(BASE_LEN_TYPE ppdu_len, u8 rate, u8 ch);
u32 non_scan_expiring_sticks(u8 rate);
u32 dll_ack_expiring_sticks(BASE_LEN_TYPE ppdu_len, unsigned char rate, unsigned char scan);
u32 diag_expiring_sticks(DLL_SEND_HANDLE pdss);
u32 windows_delay_sticks(BASE_LEN_TYPE ppdu_len, u8 r_mode, u8 scan, u8 srf, u8 window) reentrant;

u32 psr_trans_sticks(PIPE_REF pipe, BASE_LEN_TYPE downlink_nsdu_len, BASE_LEN_TYPE uplink_nsdu_len);
u32 psr_setup_expiring_sticks(u8 * script, u8 script_len);
u32 psr_transaction_sticks(PIPE_REF pipe, BASE_LEN_TYPE nsdu_downlink_len, BASE_LEN_TYPE nsdu_uplink_len, u16 resp_delay);
u32 psr_ping_sticks(PIPE_REF pipe);
u32 psr_diag_sticks(PIPE_REF pipe_ref, DLL_SEND_HANDLE pds);
u32 psr_broadcast_sticks(PIPE_REF pipe_ref, EBC_BROADCAST_HANDLE pt_ebc_prop);
u32 psr_identify_sticks(PIPE_REF pipe_ref, EBC_IDENTIFY_HANDLE pt_ebc_prop);
u32 psr_acquire_sticks(PIPE_REF pipe_ref, EBC_ACQUIRE_HANDLE pt_ebc_prop);

u32 app_transaction_sticks(u16 pipe_id, BASE_LEN_TYPE asdu_downlink_len, BASE_LEN_TYPE asdu_uplink_len, u16 resp_delay);
u32 app_psr_expiring_sticks(u16 pipe_id, BASE_LEN_TYPE asdu_downlink_len, BASE_LEN_TYPE asdu_uplink_len, u16 resp_delay);

u32 psr_patrol_sticks(PIPE_REF pipe_ref);

u32 dst_flooding_sticks(BASE_LEN_TYPE apdu_len);
u32 dst_flooding_search_sticks(void);
u32 dst_transaction_sticks(BASE_LEN_TYPE asdu_downlink_len, BASE_LEN_TYPE asdu_uplink_len, u16 resp_delay);
u32 app_dst_expiring_sticks(BASE_LEN_TYPE asdu_downlink_len, BASE_LEN_TYPE asdu_uplink_len, u16 resp_delay);



#endif

