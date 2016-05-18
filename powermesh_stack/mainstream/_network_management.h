/******************** (C) COPYRIGHT 2012 Belling Inc. ********************
* File Name          : _network_management.h
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
#ifndef _NETWORK_MANAGEMENT_H
#define _NETWORK_MANAGEMENT_H
/* Includes ------------------------------------------------------------------*/
#include "_topology.h"
#include "_network_misc.h"
#include "_psr_manipulation.h"
#include "_network_optimization.h"
#include "_powermesh_management.h"



/* Basic Exported types ------------------------------------------------------*/
void init_network_management(void);
u8 root_explore(EBC_BROADCAST_STRUCT * pt_ebc);
STATUS psr_explore(u16 pipe_id, EBC_BROADCAST_HANDLE pt_ebc_struct, u8 * return_new_nodes_num);
u16 build_network_complete(u16 max_nodes_to_search);
u16 build_network(u16 max_nodes_to_search);
BUILD_RESULT build_network_by_step(u8 phase, u16 left_nodes_to_search, BUILD_CONTROL build_option);
void set_build_config(BUILD_CONFIG_STRUCT new_build_config);
BUILD_CONFIG_STRUCT get_build_config(void);
u8 get_default_broad_window(void);
void set_current_broad_class(u8 build_class);
STATUS proceed_new_node_link(NODE_HANDLE exe_node_handle, UID_HANDLE target_uid);
u8 get_max_build_depth(void);
STATUS add_pipe_for_node(NODE_HANDLE node);
STATUS add_pipe_under_node(NODE_HANDLE parent_node, LINK_HANDLE link);

STATUS build_network_process_explored_uid(NODE_HANDLE exe_node_handle, UID_HANDLE target_uid);
STATUS optimization_process_explored_uid(NODE_HANDLE exe_node_handle, UID_HANDLE target_uid);

#endif

