/******************** (C) COPYRIGHT 2012 Belling Inc. ********************
* File Name          : _network_optimization.h
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
#ifndef _NETWORK_OPTIMIZATION_H
#define _NETWORK_OPTIMIZATION_H

/* Includes ------------------------------------------------------------------*/


/* Basic Exported types ------------------------------------------------------*/
STATUS get_optimized_link_from_node_to_uid(NODE_HANDLE node, UID_HANDLE target_uid, u8 initial_rate, LINK_HANDLE optimized_link, u8 * err_code);
STATUS get_optimized_link_from_pipe_to_uid(u16 pipe_id, UID_HANDLE target_uid_handle, u8 initial_rate, LINK_STRUCT * optimized_link, u8 * err_code);

STATUS refresh_pipe(u16 pipe_id);
STATUS repair_pipe(u16 pipe_id);
u16 dst_find_xid(ARRAY_HANDLE target_xid, BOOL is_meter_id);

STATUS optimize_pipe_link(u16 pipe_id);
STATUS optimize_path_link(u8 phase, ARRAY_HANDLE script_buffer, u8 script_length);
STATUS flooding_search(u8 phase, ARRAY_HANDLE target_xid, u8 is_target_mid, u8 rate, u8 max_jumps, u8 max_window_index, u8 acps_constrained, ARRAY_HANDLE script_buffer, u8 * script_length);

STATUS get_path_script_from_dst_search_result(ARRAY_HANDLE nsdu, BASE_LEN_TYPE nsdu_len,ARRAY_HANDLE script_buffer, u8 * script_length);
STATUS rebuild_pipe(u16 pipe_id, u8 config);
u16 dst_find(u8 phase, u8 * target_xid, u8 find_mid);
void powermesh_optimizing_proc(void);
void powermesh_optimizing_disconnect_proc(u16 disconnect_usage);
void powermesh_optimizing_update_proc(void);

void enable_network_optimizing(u8 enable_option);
STATUS psr_find_xid(NODE_HANDLE exe_node, ARRAY_HANDLE target_xid, BOOL is_meter_id, u8 use_rate, u16 * return_pipe_id);

u16 s2u3b(s16 s);
s16 u2s3b(u16 u);
#endif

