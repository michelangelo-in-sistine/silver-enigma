/******************** (C) COPYRIGHT 2012 Belling Inc. ********************
* File Name          : network_management.c
* Author             : Lv Haifeng
* Version            : V2.0
* Date               : 2013-03-062013-03-28
* Description        : 网络组织, 管理
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODECTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, BELLING INC. SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAINMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "powermesh_include.h"
#include "stdlib.h"

/* Private define ------------------------------------------------------------*/
#define DEBUG_INDICATOR DEBUG_MGNT

/* Private typedef -----------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
NODE_HANDLE network_tree[CFG_PHASE_CNT];
NODE_HANDLE solo_list[CFG_MGNT_MAX_NEW_LINKS_BUFFER_CNT];
NODE_HANDLE non_solo_list[CFG_MGNT_MAX_NEW_LINKS_BUFFER_CNT];

BUILD_CONFIG_STRUCT global_build_config;
u8 psr_explore_new_node_uid_buffer[CFG_EBC_ACQUIRE_MAX_NODES*6];


/* Private function prototypes -----------------------------------------------*/
u8 get_max_broad_class(void);
STATUS proceed_new_node_link(NODE_HANDLE exe_node_handle, UID_HANDLE target_uid);
void reset_network(void);
STATUS build_network_process_explored_uid(NODE_HANDLE exe_node_handle, UID_HANDLE target_uid);
u16 update_temporary_nodes_into_new_nodes(NODE_STRUCT * branch_root_node);
STATUS update_node_ping_info(NODE_HANDLE target_node, ARRAY_HANDLE ping_return_buffer);


/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : init_network_management()
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void init_network_management(void)
{
	u8 i;
	u8 uid_buffer[6];
	BUILD_CONFIG_STRUCT build_config;

	init_topology();

	for(i=0;i<CFG_PHASE_CNT;i++)
	{
		network_tree[i] = creat_path_tree_root();
		get_uid(i, uid_buffer);
		set_node_uid(network_tree[i], uid_buffer);
		network_tree[i]->state = 0;						//root node is always online
	}

	build_config.max_broad_class = RATE_BPSK;
	build_config.max_build_depth = 4;
	build_config.default_broad_windows = 5;
	build_config.current_broad_class = RATE_BPSK;
	set_build_config(build_config);
}

/*******************************************************************************
* Function Name  : reset_network()
* Description    : 与init_network_management区别是: 用户设置过的set_build_config不改变
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void reset_network(void)
{
	u8 uid_buffer[6];
	u8 i;

//	for(i=0;i<CFG_PHASE_CNT;i++)
//	{
//#ifdef DEBUG_INDICATOR
//#endif

//		if(get_node_usage())
//		{
//			if(!delete_branch(network_tree[i]))
//			{
//#ifdef DEBUG_INDICATOR
//				my_printf("init_topology;\r\n");
//#endif
//				init_topology();		//当拓扑错误的时候,给警告,并直接删拓扑
//				break;
//			}
//		}
	//}

	my_printf("network re-started. initialized all network information\r\n");
	init_topology();
	
	for(i=0;i<CFG_PHASE_CNT;i++)
	{
		network_tree[i] = creat_path_tree_root();
		get_uid(i, uid_buffer);
		set_node_uid(network_tree[i], uid_buffer);
		network_tree[i]->state = 0;
	}
	
	init_psr_mnpl();
}


/*******************************************************************************
* Function Name  : set_build_config()
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void set_build_config(BUILD_CONFIG_STRUCT new_build_config)
{
	if(new_build_config.max_broad_class > CFG_MAX_BUILD_CLASS)
	{
#ifdef DEBUG_INDICATOR
		my_printf("error build rate %bx, set to max %bx.\r\n",new_build_config.max_broad_class,CFG_MAX_BUILD_CLASS);
#endif
		new_build_config.max_broad_class = CFG_MAX_BUILD_CLASS;
	}
	else if(new_build_config.max_build_depth > CFG_PIPE_STAGE_CNT)
	{
#ifdef DEBUG_INDICATOR
		my_printf("error build depth %bx, set to max %bx.\r\n",new_build_config.max_build_depth,CFG_PIPE_STAGE_CNT);
#endif
		new_build_config.max_build_depth = CFG_PIPE_STAGE_CNT;
	}
	else if(new_build_config.default_broad_windows > CFG_MAX_BUILD_WINDOW)
	{
#ifdef DEBUG_INDICATOR
		my_printf("too big ebc window %bx, set to max %bx.\r\n",new_build_config.default_broad_windows,CFG_MAX_BUILD_WINDOW);
#endif
		new_build_config.default_broad_windows = CFG_MAX_BUILD_WINDOW;
	}

	global_build_config = new_build_config;
}

/*******************************************************************************
* Function Name  : get_build_config()
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
BUILD_CONFIG_STRUCT get_build_config(void)
{
	return global_build_config;
}

/*******************************************************************************
* Function Name  : get_max_broad_class()
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
u8 get_max_broad_class(void)
{
	return global_build_config.max_broad_class;
}

/*******************************************************************************
* Function Name  : get_max_build_depth()
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
u8 get_max_build_depth(void)
{
	return global_build_config.max_build_depth;
}

/*******************************************************************************
* Function Name  : get_default_broad_window(void)
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
u8 get_default_broad_window(void)
{
	return global_build_config.default_broad_windows;
}

/*******************************************************************************
* Function Name  : set_current_broad_class(void)
* Description    : 
* Input          : u8 build_class
* Output         : None
* Return         : None
*******************************************************************************/
void set_current_broad_class(u8 broad_class)
{
	global_build_config.current_broad_class = (broad_class & 0x03);
}

/*******************************************************************************
* Function Name  : set_current_broad_class(void)
* Description    : 
* Input          : u8 build_class
* Output         : None
* Return         : None
*******************************************************************************/
u8 get_current_broad_class(void)
{
	return global_build_config.current_broad_class;
}


/*******************************************************************************
* Function Name  : node_explore()
* Description    : 指定节点,建立自身周围网络
* Input          : None
* Output         : None
* Return         : 发现的新节点数,新节点UID存放在
*******************************************************************************/
u8 node_explore(NODE_HANDLE exe_node)
{
	u8 phase;
	EBC_BROADCAST_STRUCT es;
	u8 new_nodes_num = 0;
	STATUS status;

	phase = get_node_phase(exe_node);
	set_broadcast_struct(phase, &es, get_build_id(phase), exe_node->broad_class);	//es是被配置对象
	
	if(is_root_node(exe_node))
	{
		new_nodes_num = root_explore(&es);
		if(new_nodes_num == 0)
		{
			if((es.xmode & 0x03) == RATE_BPSK)
			{
#ifdef DEBUG_INDICATOR
				my_printf("SEARCH 0, CONFIRM...\r\n");
#endif
				new_nodes_num = root_explore(&es);
			}
		}
	}
	else
	{
		u16 pipe_id;

		pipe_id = node2pipeid(exe_node);
		if(pipe_id)
		{
			status = psr_explore(pipe_id, &es, &new_nodes_num);
			if((status == OKAY) && new_nodes_num == 0)
			{
				if((es.xmode & 0x03) == 0)
				{
#ifdef DEBUG_INDICATOR
					my_printf("SEARCH 0, CONFIRM...\r\n");
#endif	
					status = psr_explore(pipe_id, &es, &new_nodes_num);
				}
			}
		}
	}

	return new_nodes_num;
}

/*******************************************************************************
* Function Name  : root_explore()
* Description    : 发现根节点周围节点
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
u8 root_explore(EBC_BROADCAST_STRUCT * pt_ebc)
{
	u8 num_node;
	u8 i;

	num_node = 0;

#ifdef DEBUG_INDICATOR
	my_printf("BUILD ID=%bX,WIN=%bX,RATE=%bX,BID=%bX\r\n",pt_ebc->build_id,pt_ebc->window,(pt_ebc->xmode&0x03),pt_ebc->bid);
	my_printf("BROADCASTING...\r\n");
#endif

	powermesh_main_thread_useless_resp_clear();
	num_node = ebc_broadcast(pt_ebc);
	
	if(num_node != 0)
	{
#ifdef DEBUG_INDICATOR
		my_printf("GOT %bd BSRF\r\n",num_node);
		my_printf("IDENTIFY...\r\n");
#endif
		powermesh_main_thread_useless_resp_clear();
		num_node = ebc_identify_transaction(pt_ebc, num_node);

#ifdef DEBUG_INDICATOR
		{
			u8 target_uid[6];

			my_printf("ACQUIRE %bd NODES UID.\r\n",num_node);
			for(i=0;i<num_node;i++)
			{
				SET_BREAK_THROUGH("quit root_explore().\r\n");
				inquire_neighbor_uid_db(i,target_uid);
				uart_send_asc(target_uid,6);
				my_printf(" ");
			}
			my_printf("\r\n");
		}
#endif
	}

	return num_node;
}
//		if(num_node)
//		{
//#ifdef DEBUG_INDICATOR
//			my_printf("\r\nself_explore(): optimize link to new nodes\r\n");
//#endif
//			for(i=0;i<num_node;i++)
//			{
//				NODE_HANDLE parent_node;
//				LINK_STRUCT link_info;
//				u8 target_uid[6];
//				u8 err_code;
//				NODE_HANDLE temp_node;

//				SET_BREAK_THROUGH("quit root_explore().\r\n")

//				parent_node = network_tree[phase];
//				inquire_neighbor_uid_db(i,target_uid);

//				temp_node = search_uid_in_network(target_uid);
//				if(temp_node)
//				{
//					if(get_node_phase(temp_node) == phase)		//2014-04-21 如果同相找到, 可认为节点复位
//					{
//#ifdef DEBUG_INDICATOR
//						my_printf("root_explore():uid [");
//						uart_send_asc(target_uid,6);
//						my_printf("] duplicated, reset build id and abandon\r\n");
//#endif					
//						mgnt_ebc_set_build_id(uid2pipeid(target_uid), get_build_id(0), AUTOHEAL_LEVEL1);		//有的节点发scan容易复位, 不论成败,对其设置build id省的影响别的节点组网
//						continue;
//					}
//					else			//2014-04-21 如果异相找到, 可认为三相接到同一路电源上
//					{
//#ifdef DEBUG_INDICATOR
//						my_printf("root_explore():uid [");
//						uart_send_asc(target_uid,6);
//						my_printf("] is found in phase %bu , delete original node to phase %bu\r\n", get_node_phase(temp_node), phase);
//#endif
//						delete_node(temp_node);
//					}
//				}
//				powermesh_main_thread_useless_resp_clear();
//				result = get_optimized_link_from_node_to_uid(parent_node, target_uid, pt_ebc->xmode&0x03, &link_info, &err_code);
//				if(result)
//				{
//					mem_cpy((u8*)(&new_links_buffer[valid_link]), (u8*)(&link_info), sizeof(LINK_STRUCT));
//#ifdef DEBUG_INDICATOR
//					my_printf("OK. Downlink Mode:%bX, Uplink Mode:%bX\r\n", new_links_buffer[valid_link].downlink_mode, new_links_buffer[valid_link].uplink_mode);
//#endif
//					valid_link++;
//					result = OKAY;
//				}
//				else
//				{
//#ifdef DEBUG_INDICATOR
//					my_printf("ABANDON.\r\n");
//#endif
//				}
//			}
//		}
//	}
//	return valid_link;
//}

/*******************************************************************************
* Function Name  : psr_explore()
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS psr_explore(u16 pipe_id, EBC_BROADCAST_HANDLE pt_ebc_struct, u8 * return_new_nodes_num)
{
	u8 num_bsrf;
	u8 num_node;
	EBC_IDENTIFY_STRUCT identify;
	EBC_ACQUIRE_STRUCT acquire;
	STATUS status;

	num_node = 0;
#ifdef DEBUG_INDICATOR
	my_printf("PIPE ");
	my_printf("%bX%bX",pipe_id>>8,pipe_id);
	my_printf(",BUILD ID=%bX,WIN=%bX,RATE=%bX,BID=%bX\r\n",pt_ebc_struct->build_id,pt_ebc_struct->window,(pt_ebc_struct->xmode&0x03),pt_ebc_struct->bid);
	my_printf("PSR BROADCASTING...\r\n");
#endif

	/******* Step 1. Got New Node UID *************/
	status = mgnt_ebc_broadcast(pipe_id, pt_ebc_struct, &num_bsrf, AUTOHEAL_LEVEL1);
	if(status)
	{
#ifdef DEBUG_INDICATOR
		my_printf("GOT %bd BSRF.",num_bsrf);
#endif
		if(num_bsrf == 0)
		{
			num_node = 0;
		}
		else
		{
#ifdef DEBUG_INDICATOR
			my_printf("IDENTIFY...\r\n");
#endif
			identify.phase = pt_ebc_struct->phase;
			identify.bid = pt_ebc_struct->bid;
			identify.xmode = pt_ebc_struct->xmode;
			identify.rmode = pt_ebc_struct->rmode;
			identify.scan = pt_ebc_struct->scan;
			identify.max_identify_try = pt_ebc_struct->max_identify_try;
			identify.num_bsrf = num_bsrf;

			status = mgnt_ebc_identify(pipe_id, &identify, &num_node, AUTOHEAL_LEVEL1);
			if(status)
			{
#ifdef DEBUG_INDICATOR
				my_printf("GOT %bd CONFIRMS.",num_node);
#endif
				if(num_node)
				{
#ifdef DEBUG_INDICATOR
					my_printf("ACQUIRE...\r\n");
#endif
					acquire.phase = identify.phase;
					acquire.bid = identify.bid;
					acquire.xmode = identify.xmode;
					acquire.rmode = identify.rmode;
					acquire.scan = identify.scan;
					acquire.node_index = 0;					// 由于超过CFG_EBC_ACQUIRE_MAX_NODES的下一级节点不会被组网,因此可以在下一次组网时被发现,不会丢失
					acquire.node_num = num_node;
					status = mgnt_ebc_acquire(pipe_id, &acquire, psr_explore_new_node_uid_buffer, AUTOHEAL_LEVEL1);
					if(status)
					{
#ifdef DEBUG_INDICATOR
						my_printf("psr_explore():ACQUIRE %bd NODES UID.\r\n",(psr_explore_new_node_uid_buffer[0]-1)/6);
						uart_send_asc(&psr_explore_new_node_uid_buffer[2],(psr_explore_new_node_uid_buffer[0]-1)); //2015-10-19 第一字节为长度,第二字节为bid,第三字节开始为uid
						my_printf("\r\n");
#endif
					}
				}
			}
		}
	}

	if(!status)
	{
#ifdef DEBUG_INDICATOR
		my_printf("psr explore():fail.\r\n");
#endif
	}

	*return_new_nodes_num = num_node;
	return status;

//	if(status)
//	{
//		/******* Step 2. Diag New Node Link *************/
//		valid_link = 0;

//#ifdef DEBUG_INDICATOR
//		if(num_node)
//		{
//			my_printf("psr_explore():ACQUIRE %bd NODES UID.\r\n",num_node);
//			uart_send_asc(&psr_explore_new_node_uid_buffer[1],(psr_explore_new_node_uid_buffer[0]-1));
//			my_printf("\r\n");
//		}
//#endif

//		if(num_node != 0)
//		{
//			valid_link = 0;
//			for(diag_index=0;diag_index<num_node;diag_index++)
//			{
//				LINK_STRUCT link_info;
//				UID_HANDLE target_uid_handle;
//				u8 try_times;

//				SET_BREAK_THROUGH("quit psr_explore()\r\n");
//				target_uid_handle = &psr_explore_new_node_uid_buffer[diag_index*6+2];

////				if(search_uid_in_network(target_uid_handle))
////				{
////#ifdef DEBUG_INDICATOR
////					my_printf("psr_explore():uid [");
////					uart_send_asc(target_uid_handle,6);
////					my_printf("] duplicated, reset build id & abandon.\r\n");
////#endif					
////					mgnt_ebc_set_build_id(uid2pipeid(target_uid_handle), get_build_id(0), AUTOHEAL_LEVEL1);		//有的节点发scan容易复位, 不论成败,对其设置build id省的影响别的节点组网

////					continue;
////				}

//				for(try_times = 0; try_times<CFG_MAX_PSR_TRANSACTION_TRY; try_times++)
//				{
//					SET_BREAK_THROUGH("quit psr_explore()\r\n");
//					result = get_optimized_link_from_pipe_to_uid(pipe_id,target_uid_handle,pt_ebc_struct->xmode&0x03,&link_info,&err_code);
//					if(result)
//					{
//						break;
//					}
//					else
//					{
//						if(err_code==PSR_TIMEOUT)
//						{
//							repair_pipe(pipe_id);
//						}
//					}
//				}

//				if(result)
//				{
//					mem_cpy((u8*)(&new_links_buffer[valid_link]), (u8*)(&link_info), sizeof(LINK_STRUCT));
//#ifdef DEBUG_INDICATOR
//					my_printf("OK. Downlink Mode:%bX, Uplink Mode:%bX\r\n", new_links_buffer[valid_link].downlink_mode, new_links_buffer[valid_link].uplink_mode);
//#endif
//					valid_link++;
//				}
//				else
//				{
//					//经过修理pipe仍不能正确工作的,认为节点失去连接
//					if(err_code==PSR_TIMEOUT)
//					{
//#ifdef DEBUG_OPTIMIZATION
//						my_printf("path seems broken. set the node disconnect temporarily\r\n");
//#endif					
//						set_node_state(pipeid2node(pipe_id),NODE_STATE_DISCONNECT);
//					}
//					
//#ifdef DEBUG_INDICATOR
//					my_printf("psr diag fail, abandon target uid[");
//					uart_send_asc(target_uid_handle,6);
//					my_printf("].\r\n");
//#endif
//				}
//			}
//		}
//		*new_links_num = valid_link;
//	}
//	return status;
}

/*******************************************************************************
* Function Name  : add_pipe_for_node()
* Description    : add a new pipe for new node, return the pipe id from cc to the node.
					if the pipe exist in the pipe database, just return its pipe_id;
					if the pipe doesn't exist(e.g just add to the tree), create a new pipe and return its pipe_id;

					WARNING:
					if the pipe exist, this function only return its pipe id value, and doesn't garantee the pipe 
					is still valid;

					2013-02-06 psr setup fail, retry 3 times.
					
* Input          : node
* Output         : 
* Return         : pipe_id, if setup new pipe fail, return 0.
*******************************************************************************/
STATUS add_pipe_for_node(NODE_HANDLE node)
{
	STATUS status = FAIL;
	PIPE_REF pipe_ref;
	u8 phase;
	u16 pipe_id;
	u8 script_buffer[256];
	u8 script_len;

	phase = get_node_phase(node);
	pipe_ref = inquire_pipe_mnpl_db_by_uid(node->uid);

	if(!pipe_ref)
	{
		pipe_id = get_available_pipe_id();
	}
	else
	{
		pipe_id = get_pipe_id(pipe_ref);
	}
	
	if(pipe_id)
	{
		script_len = get_path_script_from_topology(node, script_buffer);
		if(script_len)
		{
#ifdef DEBUG_INDICATOR
			my_printf("PIPE SCRIPT: ");
			uart_send_asc(script_buffer, script_len);
			my_printf("\r\n");
#endif
			status = psr_setup(phase, pipe_id, script_buffer, script_len);	//psr_setup包括了重试
		}
		else
		{
#ifdef DEBUG_INDICATOR
			my_printf("FATAL: exe_node can't be found in the topology!\r\n");
#endif
		}
	}
	return status;
}



/*******************************************************************************
* Function Name  : add_pipe_under_node()
* Description    : 
					add(update) a pipe under assigned node, return the pipe id
					if pipe exist, resetup the pipe using the same pipe_id;
					if pipe not exist, create a new pipe and return its pipe_id;
					psr setup fail, retry 3 times.
					use for network optimizing;
					
					* difference from add_pipe_for_node():
					** add_pipe_for_node(): add node to topology first, then add pipe;
					** add_pipe_under_node(): add pipe first, then change topology;
					add_pipe_for_node() may cause dismatch between topology and pipe mnpl lib by the failure of pipe setup
					and add_pipe_under_node() can avoid such circumstance since it setup pipe first.

					** add_pipe_for_node(): if pipe exist, return the pipeid without setup pipe action;
					** add_pipe_under_node(): if pipe exist, resetup the pipe as new newlink, even if new pipe is just the same
						as the old one(just as refresh_pipe())

					
					
* Input          : node
* Output         : 
* Return         : pipe_id, if setup new pipe fail, return 0.
*******************************************************************************/
STATUS add_pipe_under_node(NODE_HANDLE parent_node, LINK_HANDLE link)
{
	STATUS status = FAIL;
	PIPE_REF pipe_ref;
	u8 phase;
	u16 pipe_id;
	u8 script_buffer[256];
	u8 script_len;

	phase = get_node_phase(parent_node);
	pipe_ref = inquire_pipe_mnpl_db_by_uid(link->uid);

	if(!pipe_ref)
	{
		pipe_id = get_available_pipe_id();
	}
	else
	{
		pipe_id = get_pipe_id(pipe_ref);
	}
	
	if(pipe_id)
	{
		script_len = get_path_script_from_topology(parent_node, script_buffer);
		mem_cpy(script_buffer+script_len, link->uid, 6);
		script_buffer[script_len+6] = link->downlink_mode;
		script_buffer[script_len+7] = link->uplink_mode;
		script_len += 8;

#ifdef DEBUG_INDICATOR
		my_printf("PIPE SCRIPT: ");
		uart_send_asc(script_buffer, script_len);
		my_printf("\r\n");
#endif
		status = psr_setup(phase, pipe_id, script_buffer, script_len);	//psr_setup包括了重试
	}
	return status;	
}


/*******************************************************************************
* Function Name  : build_network_complete()
* Description    : 连续组网直到返回complete为止
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
u16 build_network_complete(u16 max_nodes_to_search)
{
	u8 i;
	BUILD_RESULT build_result[3];
	u16 new_nodes_count;

	build_network_by_step(0, max_nodes_to_search, BUILD_RESTART);

	while(build_result[0]!=COMPLETED || build_result[1]!=COMPLETED || build_result[2]!=COMPLETED)
	{
		for(i=0;i<3;i++)
		{
			build_result[i] = build_network_by_step(i, max_nodes_to_search, BUILD_CONTINUE);
			SET_BREAK_THROUGH("quit_build_network()\r\n");
		}
		SET_BREAK_THROUGH("quit_build_network()\r\n");
	}

	
	new_nodes_count = 0;
	for(i=0;i<CFG_PHASE_CNT;i++)
	{
		new_nodes_count += inquire_network_new_nodes_count(i);
	}

	my_printf("build_network():new explored node %d\r\n",new_nodes_count);

	return new_nodes_count;
}




/*******************************************************************************
* Function Name  : build_network()
* Description    : 每相连续两次组网新增结果为0则结束组网
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
u16 build_network(u16 max_nodes_to_search)
{
	u8 i;
	BUILD_RESULT build_result[CFG_PHASE_CNT];
	u16 new_nodes_count[CFG_PHASE_CNT];
	u16 temp_nodes_count;
	u16 total_new_nodes_count;

	/* Reset topology */
	build_network_by_step(0, max_nodes_to_search, BUILD_RESTART);	//执行一次即三相均重置

	for(i=0;i<CFG_PHASE_CNT;i++)
	{
		build_result[i] = TO_BE_CONTINUE;
		new_nodes_count[i] = 0;
	}

	/* search */
	total_new_nodes_count = 0;
	do
	{
		SET_BREAK_THROUGH("quit_build_network()\r\n");
		for(i=0;i<CFG_PHASE_CNT;i++)
		{
			SET_BREAK_THROUGH("quit_build_network()\r\n");
			if(build_result[i] != COMPLETED)
			{
				build_network_by_step(i, max_nodes_to_search, BUILD_CONTINUE);
				temp_nodes_count = inquire_network_new_nodes_count(i);
				if(temp_nodes_count == new_nodes_count[i])
				{
					if(build_result[i] == ZERO_SEARCHED)		//连续两次搜索结果没有增加则搜索结束;
					{
						build_result[i] = COMPLETED;
					}
					else
					{
						build_result[i] = ZERO_SEARCHED;
					}
				}
				else
				{
					build_result[i] = TO_BE_CONTINUE;
					new_nodes_count[i] = temp_nodes_count;
				}
			}
		}
	}while(build_result[0]!=COMPLETED); // || build_result[1]!=COMPLETED || build_result[2]!=COMPLETED);

	/* report */
	total_new_nodes_count = 0;
	for(i=0;i<CFG_PHASE_CNT;i++)
	{
		total_new_nodes_count += new_nodes_count[i];
	}

	my_printf("build_network():new explored node %d\r\n",total_new_nodes_count);

	return total_new_nodes_count;
}



/*******************************************************************************
* Function Name  : get_exe_node()
* Description    : 分步组网, 每次调用得到当前应该执行操作的节点
其工作逻辑如下:
1. 首先得到"孤节点列表"和"非孤节点列表"
2. 找到"孤节点列表"里执行级别最低的节点, 设为A;
3. 找到"非孤节点列表"里执行级别最低的节点, 设为B;
4. 如B的执行级别比A小, 返回B;
5. 否则返回A;
					
* Input          : None
* Output         : None
* Return         : 应操作的节点地址, 如返回NULL, 则组网应结束
*******************************************************************************/
NODE_HANDLE get_exe_node(u8 phase)
{
	u16 solo_len;
	u16 non_solo_len;

	solo_len = get_list_from_branch(network_tree[phase], SOLO, ALL, NODE_STATE_DISCONNECT|NODE_STATE_TEMPORARY|NODE_STATE_EXCLUSIVE|NODE_STATE_ZX_BROKEN,solo_list, 0, CFG_MGNT_MAX_NEW_LINKS_BUFFER_CNT);
	non_solo_len = get_list_from_branch(network_tree[phase], NON_SOLO, ALL, NODE_STATE_DISCONNECT|NODE_STATE_TEMPORARY|NODE_STATE_EXCLUSIVE|NODE_STATE_ZX_BROKEN,non_solo_list, 0, CFG_MGNT_MAX_NEW_LINKS_BUFFER_CNT);

	solo_len = filter_list_by_build_depth(solo_list,solo_len,solo_list,get_max_build_depth());
	non_solo_len = filter_list_by_build_depth(non_solo_list,non_solo_len,non_solo_list,get_max_build_depth());

	if(solo_len==0 && non_solo_len==0)
	{
#ifdef DEBUG_INDICATOR
		my_printf("get_exe_node(): solo_len + non_solo_len couldn't be zero.\r\n");
#endif
		return NULL;
	}

	//2013-05-24 加入filt_out条件后, 不能用solo_len和non_solo_len是否为0判断是否根节点是唯一节点了
	//判断根节点是网络唯一节点的条件只是二者相加为1, 因为根节点永远不会被filt_out
	if(solo_len + non_solo_len == 1)		
	{
		if(network_tree[phase]->broad_class> get_max_broad_class())
		{
			return NULL;
		}
		else
		{
			return network_tree[phase];
		}
	}
	else
	{
		NODE_HANDLE solo_min_class_node;
		NODE_HANDLE non_solo_min_class_node;

		// find the min broad class node in all solo nodes and non-solo nodes. 
		// broad class: the datarate with which the node has found all possible neighbour node.
		// e.g. node A has use BPSK+SCAN to search all neighbour nodes. now it use BPSK+SCAN can only got 0 BSRFs.
		// so its broad class should be set as 1.
		solo_min_class_node = search_min_broad_class_node_in_node_list(solo_list, solo_len);
		non_solo_min_class_node = search_min_broad_class_node_in_node_list(non_solo_list, non_solo_len);
		if(solo_min_class_node==NULL && non_solo_min_class_node==NULL)
		{
#ifdef DEBUG_INDICATOR
			my_printf("get_exe_node(): solo_min_class_node and non_solo_min_class_node couldn't be null at the same time.\r\n");
#endif
			return NULL;
		}
		else if(solo_min_class_node==NULL)
		{
			if(non_solo_min_class_node->broad_class > get_max_broad_class())
			{
				return NULL;
			}
			else
			{
				return non_solo_min_class_node;
			}
		}
		else if(non_solo_min_class_node==NULL)
		{
			if(solo_min_class_node->broad_class > get_max_broad_class())
			{
				return NULL;
			}
			else
			{
				return solo_min_class_node;
			}
		}
		else
		{
			// if min search class is less or equal to the min search class of non solo nodes
			// operate these min-class solo nodes to execuate the EBC seach;
			// else, those min-class non-solo nodes is to execute the EBC search;
			if((non_solo_min_class_node->broad_class > get_max_broad_class())&&(solo_min_class_node->broad_class > get_max_broad_class()))
			{
				return NULL;
			}

			// 节点选择策略: 同等级情况下, 优先选择根节点
			// 如根节点等级高, 同等级情况下, 优先选择孤节点;
			if((network_tree[phase]->broad_class <= non_solo_min_class_node->broad_class) &&
			(network_tree[phase]->broad_class <= solo_min_class_node->broad_class))
			{
				return network_tree[phase];
			}
			else
			{
				if(non_solo_min_class_node->broad_class < solo_min_class_node->broad_class)
				{
					return non_solo_min_class_node;
				}
				else
				{
					return solo_min_class_node;
				}
			}
		}
	}
}

/*******************************************************************************
* Function Name  : reset_build_id()
* Description    : 2013-08-08 cc的三相使用同一个build_id, 另外为了处理集中器build_id为0的问题, 
					组网时检查build_id, 如果为0, 生成一个新的build_id
					
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void reset_build_id()
{
	u8 build_id;
	
	do
	{
		build_id = (u8)(rand());
	}while(build_id==0 || build_id == get_build_id(0));

	set_build_id(0,build_id);
}


/*******************************************************************************
* Function Name  : build_network_by_step()
* Description    : 2013-05-11
					为避免build_network执行时间过长的缺点, 采用分步形式
					每次完成一个节点的explore与拓展

					分步执行build network 时, 每次重新整理生成新的exe list表

					2015-01-13 
					zx invalid 返回Zero Searched(原来返回completed)
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
BUILD_RESULT build_network_by_step(u8 phase, u16 max_nodes_to_search, BUILD_CONTROL build_option)
{
	static u16 round[3];
	NODE_HANDLE exe_node;
	u8 collected_uid_cnt;
	u8 new_nodes_cnt;
	STATUS status;
	u8 i;
	u8 target_uid[6];
	
	if(phase >= CFG_PHASE_CNT)
	{
#ifdef DEBUG_INDICATOR
		my_printf("error build phase %d \r\n",phase);
#endif	
		return ZERO_SEARCHED;
	}

	if(build_option==BUILD_RESTART || get_build_id(0)==0)
	{
		u8 i;

#ifdef DEBUG_INDICATOR
		if(get_build_id(0)==0 && build_option!=BUILD_RESTART)
		{
			my_printf("WARNING: invalid build_id! reset network!!\r\n");
		}
#endif

		/* Initialize 网络RESTART执行一次即将三相都重置 */
		reset_network();

		for(i=0;i<CFG_PHASE_CNT;i++)
		{
			round[i] = 0;
		}
		
		/* randomize build id, build id is a 8-bit not-zero randomized integar 
		to avoid build_id repeated between 3 phases, the last 2 bits is the phase value
		*/
		//do
		//{
		//	build_id = (u8)(rand()&0xFC) + phase;
		//}while(build_id==0 || (build_id & 0x0F)==0 || build_id==get_build_id(phase));

		/* 2013-08-08 CC三相使用一个build_id, 看成一个网络, 以简化处理 */
		reset_build_id();
		
		return TO_BE_CONTINUE;
	}
	else if(build_option == BUILD_RENEW)
	{
#ifdef DEBUG_INDICATOR
		my_printf("build_network_by_step():reset all topology nodes in network phase %bd as non_searched status.\r\n",phase);
#endif
		set_branch_property(network_tree[phase], NODE_BROAD_CLASS , 0); //设置所有节点的搜索级别为0
		clr_branch_state(network_tree[phase], NODE_STATE_RECOMPLETED);	//设置所有节点为可重发现状态
		round[phase] = 0;

		return TO_BE_CONTINUE;
	}
	else if(build_option == BUILD_CONTINUE)
	{
		if(!is_zx_valid(phase))					//清理拓扑信息, renew都不需要判断过零点有效
		{
#ifdef DEBUG_INDICATOR
			my_printf("phase %d zx signal is absense, quit network building\r\n",phase);
#endif
			return ZERO_SEARCHED;
		}

		//2013-05-23 bug fix: exe_list should be updated every time, because different phase network building could be called
		exe_node = get_exe_node(phase);
		if(!exe_node)
		{
			u16 updated_nodes;

			updated_nodes = update_temporary_nodes_into_new_nodes(network_tree[phase]);
			if(updated_nodes)	//2015-10-15 当搜索周期结束时,没有成为完备节点的节点升级为完备节点标注为new arrival节点(认为其是build_id已经设置,但没有正确返回)
			{
#ifdef DEBUG_INDICATOR
				my_printf("phase %bu, %d temporary nodes are updated\r\n",phase,updated_nodes);
#endif
				return TO_BE_CONTINUE;
			}
			else
			{
#ifdef DEBUG_INDICATOR
				my_printf("build_network_by_step(): all nodes in phase %bx have reached max build class, build over.\r\n",phase);
#endif		
				print_build_network(phase);
				return COMPLETED;
			}
		}

#ifdef DEBUG_INDICATOR
		my_printf("\r\n/***PHASE%d,ROUND%d,EXE_NODE:[",phase,round[phase]);
		uart_send_asc(exe_node->uid,6);
		my_printf("],RATE:%bX***/\r\n",exe_node->broad_class);
#endif			
		round[phase]++;

		// 2.1 explore
		new_nodes_cnt = 0;
		collected_uid_cnt = node_explore(exe_node);			//node_explore返回的是exe_node周围的新节点UID的集合,存放于
														//neighbor_uid_db(根节点)或psr_explore_new_node_uid_buffer(非根节点)中;
#ifdef DEBUG_INDICATOR
		if(collected_uid_cnt)
		{
			my_printf("%bd uids were collected below node[", collected_uid_cnt);
			uart_send_asc(exe_node->uid, 6);
			my_printf("]\r\n");
		}
#endif

		// 2.2 对collect的uid处理
		for(i=0; i<collected_uid_cnt; i++)
		{
			SET_BREAK_THROUGH("BUILD ABORT.\r\n");

			// 2.2.1 get target uid
			if(is_root_node(exe_node))
			{
				status = inquire_neighbor_uid_db(i, target_uid);
			}
			else
			{
				if(((i+1)*6+2) <= sizeof(psr_explore_new_node_uid_buffer))
				{
					mem_cpy(target_uid,&psr_explore_new_node_uid_buffer[i*6+2],6);
					status = OKAY;
				}
				else
				{
					status = FAIL;
				}
			}

			// 2.2.2 处理target_uid
			if(status)
			{
				status = build_network_process_explored_uid(exe_node, target_uid);
				if(status)
				{
					new_nodes_cnt++;
				}
			}
		}

#ifdef DEBUG_INDICATOR
		if(collected_uid_cnt)			//2015-10-19 仅当collected uid cnt不为零时才报告加入新节点数
		{
			my_printf("%bd uids collected and %bd new nodes were added below node[", collected_uid_cnt, new_nodes_cnt);
			uart_send_asc(exe_node->uid, 6);
			my_printf("]\r\n");
		}
#endif


		// 2.3 统计处理
		if(max_nodes_to_search && (get_network_total_subnodes_count() >= max_nodes_to_search))	// 2013-02-19 check total node every time
		{
			u16 updated_nodes;

			updated_nodes = update_temporary_nodes_into_new_nodes(network_tree[phase]);	//最大节点数已经达到,不需要再继续搜索了
			if(updated_nodes)
			{
#ifdef DEBUG_INDICATOR
				my_printf("phase %bu, %d temporary nodes are updated\r\n",phase,updated_nodes);
#endif
				return TO_BE_CONTINUE;
			}
			else
			{
#ifdef DEBUG_INDICATOR
				u8 i;
				my_printf("total %d subnodes has been found, BUILD OVER.\r\n", get_network_total_subnodes_count());
				my_printf("BUILD OVER.\r\n");
				for(i=0;i<CFG_PHASE_CNT;i++)
				{
					print_build_network(i);
				}
#endif
				return COMPLETED;
			}
		}
		else if(new_nodes_cnt==0)
		{
#ifdef DEBUG_INDICATOR
			my_printf("NODE[");
			uart_send_asc(exe_node->uid, 6);
			my_printf("] BROAD CLASS INC TO %bX.\r\n", exe_node->broad_class+1);
#endif
			exe_node->broad_class++;
			return ZERO_SEARCHED;
		}			
		else
		{
#ifdef DEBUG_INDICATOR
			u8 i;

			my_printf("BUILD TO BE CONTINUED.\r\n");
			for(i=0;i<CFG_PHASE_CNT;i++)
			{
				print_build_network(i);
			}
#endif
			return TO_BE_CONTINUE;
		}
	}
	else
	{
#ifdef DEBUG_INDICATOR
		my_printf("WARNING: error build option: %bu\r\n",build_option);
#endif
		return ZERO_SEARCHED;
	}
}


/*******************************************************************************
* Function Name  : build_network_process_explored_uid()
* Description    : 2015-10-15
					处理逻辑:
					0. EXCLUSIVE节点直接跳过
					1. 成功完成了"链路训练 - Pipe建立 - Ping - Setup Build ID"的节点称为"完备"节点, 才会被加入拓扑成为新节点
					2. 完成了Ping, 没有完成build_id的, 也加入节点, 但称为"临时"节点
					3. 已经存在于拓扑中的完备节点,没有标注RECOMPLETED的,再次发现时,仍走一遍完备流程,如成功,迁移到新的节点位置下,并标注RECOMPLETED和REFOUND
						不成功则不变
					4. 对于已经标注RECOMPLETED的节点,不再走完备流程
					5. REFOUND的标志在应用层取走REFOUND节点集合时撤销, RECOMPLETED标志在一个新组网周期开始时撤销
					6. 一个组网周期结束时, 还没有变成完备节点的临时节点升级为完备节点, 这是预防有些节点收到了build_id, 而上行没有成功
* Input          : 执行搜索的节点, 搜索到的新节点的uid
* Output         : None
* Return         : STATUS
* Author		 : Lv Haifeng
* Ver			 : 1.0
*******************************************************************************/
STATUS build_network_process_explored_uid(NODE_HANDLE exe_node_handle, UID_HANDLE target_uid)
{
	NODE_HANDLE existed_node = NULL;
	NODE_HANDLE new_node = NULL;
	STATUS status = FAIL;
	LINK_STRUCT target_link;
	u8 err_code;
	u16 pipe_id;
	ARRAY_HANDLE return_buffer;

	//find uid in topology
	existed_node = uid2node(target_uid);
	if(existed_node)
	{
		// 对于已经标注EXCLUSIVE的节点,不再处理
		if(check_node_state(existed_node, NODE_STATE_EXCLUSIVE))
		{
#ifdef DEBUG_INDICATOR
			my_printf("node [");
			uart_send_asc(target_uid,6);
			my_printf("] is set exclusive, ignore it.\r\n");
#endif
			return FAIL;
		}

		// 对于已经标注RECOMPLETED的节点,不再处理
		if(check_node_state(existed_node, NODE_STATE_RECOMPLETED))
		{
#ifdef DEBUG_INDICATOR
			my_printf("node [");
			uart_send_asc(target_uid,6);
			my_printf("] has been re-found before, ignore it.\r\n");
#endif
			return FAIL;
		}

		// 检查处理uid是否是执行搜索节点的直系父辈节点, 如果是, 不能处理
		if(search_uid_in_branch(existed_node, exe_node_handle->uid))
		{
#ifdef DEBUG_INDICATOR
			my_printf("WARNING:node [");
			uart_send_asc(target_uid,6);
			my_printf("] is parent node of exe_node[");
			uart_send_asc(exe_node_handle->uid,6);
			my_printf("], ignore it.\r\n");
#endif
			return FAIL;
		}
	}

	// 完备节点流程
	// 2.1 训练链路
#ifdef DEBUG_INDICATOR
	my_printf("\r\n#2.1 train link to uid [");
	uart_send_asc(target_uid,6);
	my_printf("]\r\n");
#endif

	status = get_optimized_link_from_node_to_uid(exe_node_handle, target_uid, get_current_broad_class(), &target_link, &err_code);
	if(!status)
	{
#ifdef DEBUG_INDICATOR
		my_printf("build_network_process_explored_uid(): train new link from [");
		uart_send_asc(exe_node_handle->uid,6);
		my_printf("]to[");
		uart_send_asc(target_uid,6);
		my_printf("] fail. err_code:0x%bx\r\n",err_code);			//diag失败的不作为有效link,不加node
#endif
	}

	// 2.2 建立Pipe
	if(status)
	{
#ifdef DEBUG_INDICATOR
		my_printf("\r\n#2.2 setup pipe to uid [");
		uart_send_asc(target_uid,6);
		my_printf("]\r\n");
#endif
		status = add_pipe_under_node(exe_node_handle, &target_link);
	}

	// 2.3 Ping
	if(status)
	{
		return_buffer = OSMemGet(SUPERIOR);
		if(return_buffer)
		{
#ifdef DEBUG_INDICATOR
			my_printf("\r\n#2.3 ping uid [");
			uart_send_asc(target_uid,6);
			my_printf("]\r\n");
#endif
			pipe_id = uid2pipeid(target_uid);
			status = mgnt_ping(pipe_id,return_buffer);
			if(!status)
			{
				status = mgnt_ping(pipe_id,return_buffer);		//最多两次
			}
			
			if(status)
			{
				/* Ping成功的节点,建立起临时节点 */
				new_node = add_link_to_topology(exe_node_handle, &target_link);	//有子节点的节点被move_branch之后,可能存在拓扑与pipe不对应的问题

				/* 保存Ping信息 */
				update_node_ping_info(new_node,return_buffer);
				set_node_state(new_node, NODE_STATE_TEMPORARY);	//Ping成功的节点为临时节点,设置build_id成功的节点为完备节点
			}
			OSMemPut(SUPERIOR,return_buffer);
		}
		else
		{
			status = FAIL;
		}
	}

	//2.4 set build_id
	if(status)
	{
#ifdef DEBUG_INDICATOR
		my_printf("\r\n#2.4 set build id 0x%bx to uid [",get_build_id(0));
		uart_send_asc(target_uid,6);
		my_printf("]\r\n");
#endif

		pipe_id = uid2pipeid(target_uid);
		status = mgnt_ebc_set_build_id(pipe_id, get_build_id(0), AUTOHEAL_LEVEL1);
		
		if(status)
		{
			if(!existed_node || (existed_node && check_node_state(new_node,NODE_STATE_TEMPORARY)))
			{
				set_node_state(new_node,NODE_STATE_NEW_ARRIVAL);		//如果是新增节点, 或者从临时节点转来, 属于新节点
			}
			else
			{
				set_node_state(new_node,NODE_STATE_RECOMPLETED);		//已经存在于拓扑中的节点设置两个标志,RECOMPLETED在下一个组网周期撤销
				set_node_state(new_node,NODE_STATE_REFOUND);			//REFOUND在应用层读取后撤销
			}
			clr_node_state(new_node, NODE_STATE_TEMPORARY);				//去掉临时节点属性
			clr_node_state(new_node, NODE_STATE_DISCONNECT);			//去掉节点失联属性
		}
	}
	return status;
}



/*******************************************************************************
* Function Name  : optimization_process_explored_uid()
* Description    : 2015-10-29
					优化用的处理explore节点与组网的process_explored_uid区别在于:
					1. 被处理的uid肯定不是新节点
					2. 没有ping和没有setup_buildid,只有前两部训练信道和setup_pipe
					3. 且不处理节点标记
					
* Input          : 执行搜索的节点, 搜索到的新节点的uid
* Output         : None
* Return         : STATUS
* Author		 : Lv Haifeng
* Ver			 : 1.0
*******************************************************************************/
STATUS optimization_process_explored_uid(NODE_HANDLE exe_node_handle, UID_HANDLE target_uid)
{
	NODE_HANDLE existed_node = NULL;
	STATUS status = FAIL;
	LINK_STRUCT target_link;
	u8 err_code;

	//find uid in topology
	existed_node = uid2node(target_uid);
	if(existed_node)
	{
		// 对于已经标注EXCLUSIVE的节点,不再处理
		if(check_node_state(existed_node, NODE_STATE_EXCLUSIVE))
		{
#ifdef DEBUG_INDICATOR
			my_printf("node [");
			uart_send_asc(target_uid,6);
			my_printf("] is set exclusive, ignore it.\r\n");
#endif
			return FAIL;
		}

		// 检查处理uid是否是执行搜索节点的直系父辈节点, 如果是, 不能处理
		if(search_uid_in_branch(existed_node, exe_node_handle->uid))
		{
#ifdef DEBUG_INDICATOR
			my_printf("WARNING:node [");
			uart_send_asc(target_uid,6);
			my_printf("] is parent node of exe_node[");
			uart_send_asc(exe_node_handle->uid,6);
			my_printf("], ignore it.\r\n");
#endif
			return FAIL;
		}
	}

	// 2.1 训练链路
	status = get_optimized_link_from_node_to_uid(exe_node_handle, target_uid, get_current_broad_class(), &target_link, &err_code);

	// 2.2 建立Pipe
	if(status)
	{
		status = add_pipe_under_node(exe_node_handle, &target_link);
	}

	if(status)
	{
		add_link_to_topology(exe_node_handle, &target_link);	//建立链路成功即认为成功, 同步拓扑
	}

	return status;
}



/*******************************************************************************
* Function Name  : update_temporary_nodes_into_new_nodes()
* Description    : 2015-10-15, 设计新的组网策略, 完成所有"链路训练,设置pipe,ping,build_id_set"四步的节点称为"完备节点"
					只有完备节点才出现在拓扑中
					实际上完成了ping的节点就被加入拓扑中,此时为"临时节点",直到完成build_id设置才被撤销临时标志,并标为new_arrival节点
					为防止有的节点build_id设置不成功,有下行,无上行,导致该节点永远不能再响应explore广播
					因此在一个抄表周期结束后,将这样的节点升级为完备节点
* Input          : 相位
* Output         : None
* Return         : 更新了多少个节点
* Author		 : Lv Haifeng
* Ver			 : 2015-10-15 
*******************************************************************************/
u16 update_temporary_nodes_into_new_nodes(NODE_STRUCT * branch_root_node)
{
	NODE_STRUCT * sub_node;
	static int reentrant_depth = 0;
	u16 updated_nodes = 0;

	if(reentrant_depth++>MAX_REENTRANT_DEPTH)
	{
#ifdef DEBUG_DISP_INFO
		my_printf("set_branch_property():max_reentrant_depth reached\r\n");
#endif	
		reentrant_depth--;
		return 0;
	}

	if(!check_node_valid(branch_root_node))
	{
#ifdef DEBUG_DISP_INFO
		my_printf("update_temporary_nodes_into_new_nodes():error execute node\r\n");
#endif	
		reentrant_depth--;
		return 0;
	}

	/* 当前节点处理 */
	if(branch_root_node->state & NODE_STATE_TEMPORARY)
	{
		branch_root_node->state &= ~NODE_STATE_TEMPORARY;
		branch_root_node->state |= NODE_STATE_NEW_ARRIVAL;
#ifdef DEBUG_INDICATOR
		my_printf("node [");
		uart_send_asc(branch_root_node->uid, 6);
		my_printf("] was updated to completed node\r\n");
#endif
		updated_nodes++;
	}

	/* 迭代处理 */
	sub_node = branch_root_node->first_child_node;
	while(sub_node != NULL)
	{
		updated_nodes += update_temporary_nodes_into_new_nodes(sub_node);
		sub_node = sub_node->next_bro_node;
	}
	reentrant_depth--;
	return updated_nodes;
}

/*******************************************************************************
* Function Name  : update_node_ping_info()
* Description    : 2014-09-24 林洋要求在组网过程中可以由MT向CC传递数据, 故增加了PING携带用户数据功能,
				   用版本号区分新旧两种格式
				   新格式 return_buffer [返回总信息长度, ver_num[4], node_hardware_state, build_id, user_data[20]]
* Input          : 
* Output         : None
* Return         : STATUS
* Author		 : Lv Haifeng
* Ver			 : 2015-10-15 
*******************************************************************************/
STATUS update_node_ping_info(NODE_HANDLE target_node, ARRAY_HANDLE ping_return_buffer)
{
	u32 ver_num;
	ver_num = (ping_return_buffer[1]<<24);
	ver_num += (ping_return_buffer[2]<<16);
	ver_num += (ping_return_buffer[3]<<8);
	ver_num += ping_return_buffer[4];

	if(ver_num > 0x14092400)
	{
		u8 node_hardware_state;
#if BRING_USER_DATA==1
		u8 user_data_len;
		user_data_len = ping_return_buffer[0] - 6;
#endif
		
		node_hardware_state = ping_return_buffer[5];
		if(!(node_hardware_state & 0x01))
		{
#ifdef DEBUG_INDICATOR
			my_printf("target node zx broken!\r\n");
#endif						
			set_node_state(target_node,NODE_STATE_ZX_BROKEN);
		}

		if(node_hardware_state & 0x02)
		{
#ifdef DEBUG_INDICATOR
			my_printf("target node vhh inspection circuit broken!\r\n");
#endif						
			set_node_state(target_node,NODE_STATE_AMP_BROKEN);
		}

		
#ifdef CHECK_NODE_VERSION							
		if(!check_node_version_num(ver_num))
		{
#ifdef DEBUG_INDICATOR
			my_printf("target node firmware version expired!\r\n");
#endif						
			set_node_state(target_node,NODE_STATE_OLD_VERSION);
		}
#endif
		/************************************
		* 2015-06-04 林洋反应抄控器参与组网造成集中器复位, 经检查抄控器没有设置user_data
		* Ping携带过来的用户数据是一个很长的乱码, 导致集中器写溢出
		* 解决方法一个是mt端的user_data_len上电初始化为0,一个是CC端写入前检查user_data_len不能长度越界
		************************************/
#if BRING_USER_DATA==1
		if((ping_return_buffer[0] > user_data_len) && (user_data_len<=CFG_USER_DATA_BUFFER_SIZE))
		{
			mem_cpy(target_node->user_data,&ping_return_buffer[7],user_data_len);
			target_node->user_data_len = user_data_len;
		}
		else
		{
			mem_clr(target_node->user_data,CFG_USER_DATA_BUFFER_SIZE,1);
			target_node->user_data_len = 0;
#ifdef DEBUG_INDICATOR
			my_printf("bring user data error\r\n");
#endif
		}
#endif
	}
	else
	{
		// 旧格式 ping_return_buffer [zx_valid available_timer available_mem_minor, mem_superior, available queue]
		if(ping_return_buffer[1]==0)
		{
#ifdef DEBUG_INDICATOR
			my_printf("target node zx broken!\r\n");
#endif						
			set_node_state(target_node,NODE_STATE_ZX_BROKEN);
		}
		
#ifdef CHECK_NODE_VERSION							
		if(!check_node_version(ping_return_buffer))
		{
#ifdef DEBUG_INDICATOR
			my_printf("target node firmware version expired!\r\n");
#endif						
			set_node_state(target_node,NODE_STATE_OLD_VERSION);
		}
#endif

		if(ping_return_buffer[11]!=0)
		{
#ifdef DEBUG_INDICATOR
			my_printf("target node vhh inspection circuit broken!\r\n");
#endif						
			set_node_state(target_node,NODE_STATE_AMP_BROKEN);
		}
	}
	return OKAY;
}
