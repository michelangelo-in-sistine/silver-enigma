/******************** (C) COPYRIGHT 2012 Belling Inc. ********************
* File Name          : mgnt_misc.h
* Author             : Lv Haifeng
* Version            : V 2.0
* Date               : 2013-03-28
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
#ifndef _MGNT_MISC_H
#define _MGNT_MISC_H
/* Includes ------------------------------------------------------------------*/


/* Basic Exported types ------------------------------------------------------*/
u16 uid2pipeid(UID_HANDLE uid);
UID_HANDLE pipeid2uid(u16 pipe_id);
NODE_HANDLE uid2node(UID_HANDLE uid);
UID_HANDLE node2uid(NODE_HANDLE node_handle);
u16 node2pipeid(NODE_HANDLE node_handle);
NODE_HANDLE pipeid2node(u16 pipe_id);
u8 get_node_phase(NODE_HANDLE node_handle);
STATUS choose_optimized_ch_by_scan_diag_data(u8 * scan_diag_data, u8 * ch_downlink, u8 * ch_uplink);

u8 search_min_broad_class_in_node_list(NODE_HANDLE * node_list, u8 node_list_len);
NODE_HANDLE search_min_broad_class_node_in_node_list(NODE_HANDLE * node_list, u8 node_list_len);

u8 filter_list_by_build_depth(NODE_HANDLE * node_list, u8 node_list_len, NODE_HANDLE * target_list, u8 max_build_depth);
u8 get_list_from_list_by_broad_class(NODE_HANDLE * node_list, u8 node_list_len, NODE_HANDLE * target_list, u8 broad_class);
void set_broadcast_struct(u8 phase, EBC_BROADCAST_STRUCT * pes, u8 build_id, u8 broad_class);
BOOL is_self_node(u8 phase, NODE_HANDLE node);
BOOL is_root_node(NODE_HANDLE node);
NODE_HANDLE add_link_to_topology(NODE_HANDLE parent_node, LINK_HANDLE new_link);

// User's Interface
u16 inquire_network_new_nodes_count(u8 phase);
u16 inquire_network_nodes_count(u8 phase);
u16 inquire_network_all_nodes_count(void);

u16 acquire_network_new_nodes(u8 phase, u8 * nodes_buffer, u16 nodes_buffer_size);
u16 acquire_network_nodes(u8 phase, u8 * nodes_buffer, u16 nodes_buffer_size);
u16 get_network_total_subnodes_count(void);
u16 acquire_network_all_nodes(u8 * nodes_buffer, u16 nodes_buffer_size);
u16 acquire_network_refound_nodes(u8 phase, u8 * nodes_buffer, u16 nodes_buffer_size);

u8 inquire_phase_of_uid(UID_HANDLE uid);
u16 inquire_pipe_of_uid(UID_HANDLE uid);
STATUS acquire_target_uid_of_pipe(u16 pipe_id, u8 * target_uid_buffer);

STATUS set_node_state(NODE_HANDLE node, u16 set_bits);
STATUS clr_node_state(NODE_HANDLE node, u16 clr_bits);
BOOL check_node_state(NODE_HANDLE node, u16 state_bit);
BOOL check_uid_state(UID_HANDLE target_uid, u16 state_bit);
BOOL check_node_version(ARRAY_HANDLE ping_return_buffer);
BOOL check_node_version_num(u32 ver_num);

NODE_HANDLE search_uid_in_network(UID_HANDLE uid_handle);

// Link Evaluation
float calc_ptrp(u8 * rate_snr_description, u8 stage, float * pt_ppcr);
float add_ptrp(float ptrp1, float ppcr1, float ptrp2, float ppcr2, float * pt_add_ppcr);
float calc_link_ptrp(u8 downlink_mode, u8 downlink_snr, u8 uplink_mode, u8 uplink_snr, float * pt_ppcr);
float calc_node_ptrp(NODE_HANDLE node, float * pt_ppcr);
float calc_node_add_link_ptrp(NODE_HANDLE node, LINK_HANDLE link, float * pt_ppcr);
float calc_path_add_link_ptrp(float path_ptrp, float path_ppcr, LINK_HANDLE link, float * pt_ppcr);


STATUS sync_pipe_to_topology(u16 pipe_id);
STATUS sync_path_script_to_topology(u8 phase, ARRAY_HANDLE script, u8 script_len);
BOOL is_path_concordant_with_topology(u8 phase, ARRAY_HANDLE script, u8 script_len);

RESULT phase_diag(u8 phase, UID_HANDLE target_uid_handle, u8 diag_rate);
RESULT acps_diag(u8 phase, UID_HANDLE target_uid_handle, u8 diag_rate);



s8 acquire_node_user_data(NODE_HANDLE target_node, ARRAY_HANDLE user_data_buffer);
s8 acquire_uid_user_data(UID_HANDLE target_uid, ARRAY_HANDLE user_data_buffer);
s8 acquire_pipe_user_data(u16 pipe_id, ARRAY_HANDLE user_data_buffer);
STATUS update_pipe_user_data(u16 pipe_id);
STATUS update_uid_user_data(UID_HANDLE target_uid);
STATUS update_topology_link(NODE_HANDLE target_node, LINK_HANDLE new_link_info);
u16 get_topology_subnodes_cnt(u8 phase);
u8 get_dst_optimal_windows(u16 num_nodes);
STATUS set_uid_disconnect(UID_HANDLE uid_handle);
STATUS clr_uid_disconnect(UID_HANDLE uid_handle);


STATUS update_pipe_db_in_branch(NODE_HANDLE update_branch_node);

#endif

