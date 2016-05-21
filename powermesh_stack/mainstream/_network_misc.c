/******************** (C) COPYRIGHT 2012 Belling Inc. ********************
* File Name          : mgnt_misc.c
* Author             : Lv Haifeng
* Version            : V2.0
* Date               : 2013-03-28
* Description        : MGNT层联系高层抽象与实现的杂类工具函数
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
#include "ver_info.h"
#include "math.h"
/* Private define ------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
#define DEBUG_INDICATOR DEBUG_OPTIMIZATION

/* Extern variables ---------------------------------------------------------*/
extern NODE_HANDLE network_tree[];
extern LINK_STRUCT new_nodes_info_buffer[];

/* Private variables ---------------------------------------------------------*/
NODE_HANDLE temp_node_list[CFG_MGNT_MAX_NODES_CNT];

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : uid2pipeid
* Description	 : 
* Input 		 : None
* Output		 : None
* Return		 : None
*******************************************************************************/
u16 uid2pipeid(UID_HANDLE uid)
{
	PIPE_REF pipe;
	
	if(!uid)
	{
#ifdef DEBUG_DISP_INFO
		my_printf("FATAL: UID INVALID.\r\n");
#endif
		return 0;
	}

	pipe = inquire_pipe_mnpl_db_by_uid(uid);
	if(!pipe)
	{
		return 0;
	}

	return pipe->pipe_info & 0x0FFF;
}

/*******************************************************************************
* Function Name  : pipeid2uid
* Description	 : 
* Input 		 : None
* Output		 : None
* Return		 : None
*******************************************************************************/
UID_HANDLE pipeid2uid(u16 pipe_id)
{
	PIPE_REF pipe;
	ADDR_REF_TYPE addr;
	
	if(!pipe_id)
	{
#ifdef DEBUG_DISP_INFO
		my_printf("FATAL: PIPE_ID INVALID.\r\n");
#endif
		return NULL;
	}

	pipe = inquire_pipe_mnpl_db(pipe_id);
	if(pipe)
	{
		addr = pipe->way_uid[pipe->pipe_stages-1];
		return inquire_addr_db(addr);
	}
	else
	{
		return NULL;
	}
}

/*******************************************************************************
* Function Name  : uid2node
* Description	 : 
* Input 		 : None
* Output		 : None
* Return		 : None
*******************************************************************************/
NODE_HANDLE uid2node(UID_HANDLE uid)
{
	NODE_HANDLE node_handle;
	u8 i;
	
	if(!uid)
	{
#ifdef DEBUG_DISP_INFO
		my_printf("FATAL: UID INVALID.\r\n");
#endif
		return NULL;
	}

	for(i=0;i<CFG_PHASE_CNT;i++)
	{
		node_handle = search_uid_in_branch(network_tree[i], uid);
		if(node_handle)
		{
			return node_handle;
		}
	}
	return NULL;
}

/*******************************************************************************
* Function Name  : node2uid
* Description	 : 
* Input 		 : None
* Output		 : None
* Return		 : None
*******************************************************************************/
UID_HANDLE node2uid(NODE_HANDLE node_handle)
{
	if(!check_node_valid(node_handle))
	{
#ifdef DEBUG_DISP_INFO
		my_printf("FATAL: NODE_HANDLE INVALID.\r\n");
#endif
		return NULL;
	}

	return node_handle->uid;
}


/*******************************************************************************
* Function Name  : node2pipeid
* Description	 : 
* Input 		 : None
* Output		 : None
* Return		 : None
*******************************************************************************/
u16 node2pipeid(NODE_HANDLE node_handle)
{
	if(!check_node_valid(node_handle))
	{
#ifdef DEBUG_DISP_INFO
		my_printf("FATAL: NODE_HANDLE INVALID.\r\n");
#endif
		return 0;
	}

	return uid2pipeid(node2uid(node_handle));
}

/*******************************************************************************
* Function Name  : node2pipeid
* Description	 : 
* Input 		 : None
* Output		 : None
* Return		 : None
*******************************************************************************/
NODE_HANDLE pipeid2node(u16 pipe_id)
{
	if(!pipe_id)
	{
#ifdef DEBUG_DISP_INFO
		my_printf("FATAL: PIPE_ID INVALID.\r\n");
#endif
		return NULL;
	}

	return uid2node(pipeid2uid(pipe_id));
}

/*******************************************************************************
* Function Name  : get_node_phase
* Description    : 得到节点相位
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
u8 get_node_phase(NODE_HANDLE node_handle)
{
	u8 i;

	if(!check_node_valid(node_handle))
	{
#ifdef DEBUG_DISP_INFO
		my_printf("FATAL: NODE_HANDLE INVALID.\r\n");
#endif
		return 0;
	}

	for(i=0; i<=2; i++)
	{
		if(is_node_in_branch(node_handle, network_tree[i]))
		{
			return i;
		}
	}

#ifdef DEBUG_DISP_INFO
	my_printf("FATAL: NODE BELONG TO NO TREE.\r\n");
#endif
	return (u8)INVALID_RESOURCE_ID;
}

/*******************************************************************************
* Function Name  : choose_optimized_ch_by_scan_diag_data()
* Description    : 通过scan的diag结果选出上下行最佳信道. 选择策略是信噪比优先, 在信噪比一致的条件下选择ss较大的
					接收buffer数据格式: downlink ch0 ss, downlink ch0 snr, downlink ch1 ss, ...
									uplink ch0 ss, uplink ch1 snr, ...
					共16字节
* Input          : scan diag result
* Output         : index of chosen uplink ch and downlink ch, 0:ch0, 1:ch1...
* Return         : OKAY
*******************************************************************************/
STATUS choose_optimized_ch_by_scan_diag_data(u8 * scan_diag_data, u8 * ch_downlink, u8 * ch_uplink)
{
	s8 snr;
	s8 ss;
	u8 i;
	u8 * pt;

	*ch_downlink = 0;				// give a initial default value
	*ch_uplink = 0;

	pt = &scan_diag_data[1];
	snr = -128;
	ss = -128;
	
	for(i=0;i<=3;i++)
	{
		if(((s8)(*pt)>snr) ||
			((s8)(*pt) == snr) && ((s8)(*(pt-1))>ss))
		{
			snr = (s8)(*pt);
			ss = (s8)(*(pt-1));
			*ch_downlink = i;
		}
		pt += 2;
	}

	snr = -128;
	ss = -128;
	for(i=0;i<=3;i++)
	{
		if(((s8)(*pt)>snr) ||
			((s8)(*pt) == snr) && ((s8)(*(pt-1))>ss))
		{
			snr = (s8)(*pt);
			ss = (s8)(*(pt-1));
			*ch_uplink = i;
		}
		pt += 2;
	}
	
	return OKAY;
}

/*******************************************************************************
* Function Name  : search_min_broad_class_in_node_list()
* Description    : 
					
* Input          : None
* Output         : None
* Return         : min broad class in the node list, if node list is zero, return MAX broad class
*******************************************************************************/
u8 search_min_broad_class_in_node_list(NODE_HANDLE * node_list, u8 node_list_len)
{
	NODE_HANDLE node;
	u8 min_broad_class;
	u8 i;
	
	min_broad_class = 3;
	for(i=0; i<node_list_len; i++)
	{
		node = node_list[i];
		if(node->broad_class < min_broad_class)
		{
			min_broad_class = node->broad_class;
		}
	}
	return min_broad_class;
}

/*******************************************************************************
* Function Name  : search_min_broad_class_node_in_node_list()
* Description    : 
					
* Input          : None
* Output         : None
* Return         : min broad class node in the node list, if node list is zero, return NULL
*******************************************************************************/
NODE_HANDLE search_min_broad_class_node_in_node_list(NODE_HANDLE * node_list, u8 node_list_len)
{
	NODE_HANDLE node;
	NODE_HANDLE min_broad_class_node;
	u8 min_broad_class;
	u8 i;
	
	min_broad_class = 3;
	min_broad_class_node = NULL;
	for(i=0; i<node_list_len; i++)
	{
		node = node_list[i];
		if(node->broad_class < min_broad_class)
		{
			min_broad_class = node->broad_class;
			min_broad_class_node = node;
		}
	}
	return min_broad_class_node;
}


/*******************************************************************************
* Function Name  : get_list_from_list_by_broad_class()
* Description    : 从一个node list中提取出所有broad class等于指定broad class的节点, 存入target_list中
					
* Input          : None
* Output         : None
* Return         : target_list的长度
*******************************************************************************/
u8 get_list_from_list_by_broad_class(NODE_HANDLE * node_list, u8 node_list_len, NODE_HANDLE * target_list, u8 broad_class)
{
	u8 target_list_len;
	u8 i;
	NODE_HANDLE node;
	
	target_list_len = 0;
	for(i=0; i<node_list_len; i++)
	{
		node = node_list[i];
		if(node->broad_class == broad_class)
		{
			target_list[target_list_len] = node;
			target_list_len++;
		}
	}
	return target_list_len;
}

/*******************************************************************************
* Function Name  : filter_list_by_build_depth()
* Description    : 从一个node list中删除超过指定深度的节点, 存入target_list中
					target_list与node_list可以是同一个
					
* Input          : None
* Output         : None
* Return         : target_list的长度
*******************************************************************************/
u8 filter_list_by_build_depth(NODE_HANDLE * node_list, u8 node_list_len, NODE_HANDLE * target_list, u8 max_build_depth)
{
	u8 target_list_len;
	u8 i;
	NODE_HANDLE node;

	target_list_len = 0;
	for(i=0; i<node_list_len; i++)
	{
		node = node_list[i];
		if((max_build_depth == 0) || (get_node_depth(node)<max_build_depth))
		{
			target_list[target_list_len] = node;
			target_list_len++;
		}
	}
	return target_list_len;

	
}


/*******************************************************************************
* Function Name  : set_broadcast_struct()
* Description    : 设置build network需要的broadcast的结构体
					
* Input          : None
* Output         : None
* Return         : target_list的长度
*******************************************************************************/
void set_broadcast_struct(u8 phase, EBC_BROADCAST_STRUCT * pes, u8 build_id, u8 broad_class)
{
	pes->phase = phase;
	pes->build_id = build_id;
	pes->scan = 0x01;
#ifdef SEARCH_BY_PHASE	
	pes->mask = BIT_NBF_MASK_BUILDID | BIT_NBF_MASK_ACPHASE;
#else
	pes->mask = BIT_NBF_MASK_BUILDID /*| BIT_NBF_MASK_ACPHASE*/;
#endif
	pes->bid = build_id & 0x0F;		// broad_id 4 bits
	pes->max_identify_try = 0x02;
	if(!pes->bid)
	{
		pes->bid = 0x01;
	}

	set_current_broad_class(broad_class);
	switch(broad_class)			//0:BPSK; 1:DS15; 2:DS63 3:OVER
	{
		case(0):
		{
			pes->xmode = 0x10;
			pes->rmode = 0x10;
			pes->window = get_default_broad_window();		// 2014-04-21 送检版本主动注册设置窗口小一点
			break;
		}
		case(1):
		{
			pes->xmode = 0x11;
			pes->rmode = 0x11;
			pes->window = ((get_default_broad_window()-1)>0) ? (get_default_broad_window()-1):2;
			break;
		}
		case(2):
		{
			pes->xmode = 0x12;
			pes->rmode = 0x12;
			pes->window = ((get_default_broad_window()-2)>0) ? (get_default_broad_window()-2):1;
			break;
		}
		default:
		{
#ifdef DEBUG_MGNT
			my_printf("ERROR: build has over\r\n");
#endif
			return;
		}
	}
}

/*******************************************************************************
* Function Name  : is_self_node()
* Description    : Judge if the node is the local node
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
BOOL is_self_node(u8 phase, NODE_HANDLE node)
{
	u8 self_uid[6];

	get_uid(phase, self_uid);
	if(mem_cmp(node->uid, self_uid,6))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
	
}

/*******************************************************************************
* Function Name  : is_root_node()
* Description    : Judge if the node is the root node
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
BOOL is_root_node(NODE_HANDLE node)
{
	if(node->parent_node)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}



/*******************************************************************************
* Function Name  : add_link_to_topology()
* Description    : 指定节点上增加一个新子节点, 设置其UID, 上行plan, ss, snr, 下行plan, ss, snr
					//2015-09-28 如果拓扑中已经存在该节点,移动此节点
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
NODE_HANDLE add_link_to_topology(NODE_HANDLE parent_node, LINK_HANDLE new_link)
{
	NODE_HANDLE exist_node;
	NODE_HANDLE new_node;

	if((check_uid(0,new_link->uid)==BROADCAST) || (check_uid(0,new_link->uid)==SOMEBODY))
	{
#ifdef DEBUG_INDICATOR
		my_printf("try to add error uid[");
		uart_send_asc(new_link->uid,6);
		my_printf("] to topology! \r\n");
		
#endif
		return NULL;
	}

	exist_node = search_uid_in_network(new_link->uid);
	if(exist_node)											//node 可能是原有网络中的节点换了父节点, 通信方式也要变化
	{
		move_branch(exist_node,parent_node);				//2015-09-28 如果拓扑中已经存在该节点,移动此节点. 如需移动节点本来就是parent_node子节点,move_branch()直接返回

		update_topology_link(exist_node, new_link);

		return exist_node;
	}
	else
	{
		new_node = add_node(parent_node);
		if(new_node == NULL)
		{
			return NULL;
		}
		else
		{
			mem_cpy(new_node->uid,new_link->uid,6);
			update_topology_link(new_node, new_link);

			return new_node;
		}
	}
}


/*******************************************************************************
* Function Name  : inquire_network_new_nodes_count()
* Description    : 查询网络指定相位新探索得到的节点数
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u16 inquire_network_new_nodes_count(u8 phase)
{
	if(phase<CFG_PHASE_CNT)
	{
		return	get_list_from_branch(network_tree[phase], ALL, NODE_STATE_NEW_ARRIVAL, NODE_STATE_EXCLUSIVE, NULL,0,0);
	}
	else
	{
		return 0;
	}
}

/*******************************************************************************
* Function Name  : inquire_network_nodes_count()
* Description    : 查询网络指定相位所有节点数
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u16 inquire_network_nodes_count(u8 phase)
{
	if(phase<CFG_PHASE_CNT)
	{
		return	get_list_from_branch(network_tree[phase], ALL, ALL, NODE_STATE_EXCLUSIVE, NULL,0,0)-1;
	}
	else
	{
		return 0;
	}
}


/*******************************************************************************
* Function Name  : inquire_network_all_nodes_count()
* Description    : 查询网络当前所有节点数
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u16 inquire_network_all_nodes_count()
{
	u16 total_nodes;
	u8 i;

	total_nodes = 0;

	for(i=0;i<CFG_PHASE_CNT;i++)
	{
		total_nodes += inquire_network_nodes_count(i);
	}

	return total_nodes;
}

/*******************************************************************************
* Function Name  : acquire_network_nodes()
* Description    : 获得指定相位网络节点uid
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u16 acquire_network_nodes(u8 phase, u8 * nodes_buffer, u16 nodes_buffer_size)
{
	u16 node_cnt;
	u16 i;
	ARRAY_HANDLE ptw;

	if(!nodes_buffer)
	{
		return 0;
	}
	
	node_cnt = get_list_from_branch(network_tree[phase], ALL, ALL, NODE_STATE_EXCLUSIVE, temp_node_list, 0, CFG_MGNT_MAX_NODES_CNT);
	node_cnt--;
	
	if((node_cnt)*6 > nodes_buffer_size)
	{
		node_cnt = nodes_buffer_size/6;
	}

	ptw = nodes_buffer;
	for(i=0;i<node_cnt;i++)
	{
		mem_cpy(ptw,temp_node_list[i+1]->uid,6);						//第一个节点是根节点,要去掉
		clr_node_state(temp_node_list[i+1],NODE_STATE_NEW_ARRIVAL);		//清除NEW_ARRIVAL信息
		ptw += 6;
	}

	return node_cnt;
}

/*******************************************************************************
* Function Name  : get_network_total_subnodes_count()
* Description    : 获得网络全部子节点数
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u16 get_network_total_subnodes_count(void)
{
	u16 count;
	u8 i;

	count = 0;
	for(i=0;i<CFG_PHASE_CNT;i++)
	{
		count += get_list_from_branch(network_tree[i],ALL,ALL,NODE_STATE_EXCLUSIVE,NULL,0,0)-1;
	}
	return count;
}

/*******************************************************************************
* Function Name  : acquire_network_all_nodes()
* Description    : 获得网络全部节点uid
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u16 acquire_network_all_nodes(u8 * nodes_buffer, u16 nodes_buffer_size)
{
	u16 node_cnt_phase,node_cnt;
	u16 i;
	u16 used_nodes_buffer_size;
	ARRAY_HANDLE ptw;

	node_cnt = 0;
	used_nodes_buffer_size = 0;
	ptw = nodes_buffer;
	for(i=0;i<CFG_PHASE_CNT;i++)
	{
		node_cnt_phase = acquire_network_nodes(i, ptw, nodes_buffer_size - used_nodes_buffer_size);
		ptw += node_cnt_phase * 6;
		used_nodes_buffer_size = ptw - nodes_buffer;
		node_cnt += node_cnt_phase;
	}
	
	return node_cnt;
}

/*******************************************************************************
* Function Name  : inquire_phase_of_uid()
* Description    : 查询uid所在相位
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
//u8 inquire_phase_of_uid(UID_HANDLE uid)
//{
//	NODE_HANDLE node;

//	node = uid2node(uid);
//	if(node)
//	{
//		return get_node_phase(node);
//	}
//	else
//	{
//		return (u8)INVALID_RESOURCE_ID;
//	}
//}

u8 inquire_phase_of_uid(UID_HANDLE uid)
{
	PIPE_REF pipe_ref;
	
	pipe_ref = inquire_pipe_mnpl_db_by_uid(uid);
	if(!pipe_ref)
	{
		return (u8)INVALID_RESOURCE_ID;
	}
	else
	{
		return pipe_ref->phase;
	}
	
}


/*******************************************************************************
* Function Name  : inquire_pipe_of_uid()
* Description    : 查询uid所在相位
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
//u16 inquire_pipe_of_uid(UID_HANDLE uid)
//{
//	return uid2pipeid(uid);
//}

u16 inquire_pipe_of_uid(UID_HANDLE uid)
{
	PIPE_REF pipe_ref;
	pipe_ref = inquire_pipe_mnpl_db_by_uid(uid);
	if(!pipe_ref)
	{
		return 0;
	}
	else
	{
		return (pipe_ref->pipe_info & 0x0FFF);
	}
}


/*******************************************************************************
* Function Name  : acquire_target_uid_of_pipe()
* Description    : 获得pipe对应的目标节点uid
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS acquire_target_uid_of_pipe(u16 pipe_id, u8 * target_uid_buffer)
{
	ARRAY_HANDLE node_uid;

	node_uid = pipeid2uid(pipe_id);
	if(node_uid)
	{
		if(target_uid_buffer)
		{
			mem_cpy(target_uid_buffer, node_uid, 6);
		}
		return OKAY;
	}
	else
	{
		return FAIL;
	}
}


/*******************************************************************************
* Function Name  : set_node_state()
* Description    : 设置节点状态, 
					e.g. set_node_state(node, NODE_STATE_EXCLUSIVE|NODE_STATE_ACPS_BROKEN);
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS set_node_state(NODE_HANDLE node, u16 set_bits)
{
	if(!check_node_valid(node))
	{
		return FAIL;
	}
	
	node->state |= set_bits;
	return OKAY;
}

/*******************************************************************************
* Function Name  : clr_node_state()
* Description    : 清除节点状态, 
					e.g. clr_node_state(node, NODE_STATE_DISCONNECT);
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS clr_node_state(NODE_HANDLE node, u16 clr_bits)
{
	if(!check_node_valid(node))
	{
		return FAIL;
	}
	
	node->state &= ~clr_bits;
	return OKAY;
}

/*******************************************************************************
* Function Name  : check_node_state()
* Description    : 检查节点状态, 
					
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
BOOL check_node_state(NODE_HANDLE node, u16 state_bit)
{
	if(!check_node_valid(node))
	{
		return FALSE;
	}

	if(node->state & state_bit)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}

}

/*******************************************************************************
* Function Name  : check_uid_state()
* Description    : 检查UID节点状态, 
					
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
BOOL check_uid_state(UID_HANDLE target_uid, u16 state_bit)
{
	NODE_HANDLE target_node;

	target_node = uid2node(target_uid);
	if(target_node)
	{
		return check_node_state(target_node, state_bit);
	}
	else
	{
		return FALSE;
	}
}


/*******************************************************************************
* Function Name  : set_uid_exclusive()
* Description    : 设置节点为其他台区的节点, 不在其上做进一步拓展
					
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS set_uid_exclusive(UID_HANDLE uid_handle)
{
	NODE_HANDLE node;

	node = uid2node(uid_handle);
	if(!node)
	{
		return FAIL;
	}
	else
	{
		return set_node_state(node, NODE_STATE_EXCLUSIVE);
	}
}

/*******************************************************************************
* Function Name  : set_uid_disconnect()
* Description    : 设置节点disconnect
					
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS set_uid_disconnect(UID_HANDLE uid_handle)
{
	NODE_HANDLE node;

	node = uid2node(uid_handle);
	if(!node)
	{
		return FAIL;
	}
	else
	{
		node->set_disconnect_cnt++;				//2014-10-16 每次set_node_state都对计数器加1
		clr_node_state(node, NODE_STATE_RECONNECT);				//2015-10-29 set_disconnect的同时不能有reconnect,逻辑不通
		return set_node_state(node, NODE_STATE_DISCONNECT);
	}
}

/*******************************************************************************
* Function Name  : clr_uid_disconnect()
* Description    : 撤销节点disconnect状态
					
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS clr_uid_disconnect(UID_HANDLE uid_handle)
{
	NODE_HANDLE node;

	node = uid2node(uid_handle);
	if(!node)
	{
		return FAIL;
	}
	else
	{
		return clr_node_state(node, NODE_STATE_DISCONNECT);
	}
}


/*******************************************************************************
* Function Name  : check_node_version()
* Description    : 检查节点固件版本是否正确
					PING返回的版本号为return_buffer的2,3,4,5
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
BOOL check_node_version(ARRAY_HANDLE ping_return_buffer)
{
	if(ping_return_buffer[2]!=VER_YEAR)
	{
		return FALSE;
	}
	else if(ping_return_buffer[3]!=VER_MONTH)
	{
		return FALSE;
	}
	else if(ping_return_buffer[4]!=VER_DAY)
	{
		return FALSE;
	}
	else if(ping_return_buffer[5]!=VER_HOUR)
	{
		return FALSE;
	}
	return TRUE;
}

/*******************************************************************************
* Function Name  : check_node_version_num()
* Description    : 检查节点固件版本是否正确
					应对新版本
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
BOOL check_node_version_num(u32 ver_num)
{
	u32 correct_ver_num;

	correct_ver_num = (VER_YEAR<<24) + (VER_MONTH<<16) + (VER_DAY<<8) + VER_HOUR;

	return (ver_num == correct_ver_num)?TRUE:FALSE;
}


/*******************************************************************************
* Function Name  : search_uid_in_network()
* Author		 : Lv Haifeng
* Date			 : 2014-10-11
* Description    : 在全网搜索指定UID, 找到则返回节点句柄
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
NODE_HANDLE search_uid_in_network(UID_HANDLE uid_handle)
{
	u8 i;
	NODE_HANDLE duplicated_node;
	
	for(i=0;i<CFG_PHASE_CNT;i++)
	{
		duplicated_node = search_uid_in_branch(network_tree[i], uid_handle);
		if(duplicated_node)
		{
			return duplicated_node;
		}
	}
	return NULL;
}


/*******************************************************************************
* Function Name  : calc_ptrp()
* Description    : 
					
* Input          : path_description, num_link
					path_description format:[comm_type snr comm_type snr ...]
					ptrp of path = 
* Output         : 
* Return         : 
*******************************************************************************/
#define PTRP_ALPHA	0.5		//ebn0因子
#define PTRP_B		256		//从be
float calc_ptrp(u8 * path_rate_snr_description, u8 num_link, float * pt_ppcr)
{
	u8 i;
	u8 * pt;
	float num;
	float den;
	float ebn0;
	float erfx;
	float ptrp;			//path-roughput

	pt = path_rate_snr_description;

	num = 1;
	den = 0;
	
	for(i=0;i<num_link;i++)
	{
		// den = sum(1/v1 + 1/v2 + 1/v3 ...) = sum(1 + 15 + 63 + ...)/v1, and v1 = 5e6/912
		switch((*(pt++)) & 0x03)					
		{
			case(0x00):
			{
				den = den + 1;
				break;
			}
			case(0x01):
			{
				den = den + 15;
				break;
			}
			case(0x02):
			{
				den = den + 63;
				break;
			}
			default:
			{
#ifdef DEBUG_TOPOLOGY
				my_printf("error comm type\r\n");
#endif
 				return 0;				//此处若使用return 语句, watch窗口关于ebn0的显示会出错
			}
		}

		ebn0 = ((float)((s8)(*pt++)));
		ebn0 = ebn0/10;
		ebn0 = pow(10,ebn0);			//db2pow
		ebn0 = sqrt(ebn0*PTRP_ALPHA);
		erfx = tanh(1.12838*ebn0+0.10277*pow(ebn0,3));
		num = num * (pow(erfx,PTRP_B));
	}
	if(pt_ppcr)
	{
		*pt_ppcr = num;					//ppcr用于递推计算ptpr
	}
	ptrp = num / den * 5482.45614;
	return ptrp;
}

/*******************************************************************************
* Function Name  : calc_link_ptrp
* Description    : 计算一段链路的PTRP
* Input          : None
* Output         : None
* Return         : PTRP of the link
*******************************************************************************/
float calc_link_ptrp(u8 downlink_mode, u8 downlink_snr, u8 uplink_mode, u8 uplink_snr, float * pt_ppcr)
{
	u8 buffer[4];
	float ptrp;

	buffer[0]=downlink_mode;
	buffer[1]=downlink_snr;
	buffer[2]=uplink_mode;
	buffer[3]=uplink_snr;

	ptrp = calc_ptrp(buffer,2,pt_ppcr);
	
	return ptrp;
}

/*******************************************************************************
* Function Name  : add_ptrp
* Description    : 计算两条path合并后的新路径的ptpr
*					ptrp = r1*r2/(r1/p1 + r2/p2)
* Input          : None
* Output         : None
* Return         : PTRP of the new path
*******************************************************************************/
float add_ptrp(float ptrp1, float ppcr1, float ptrp2, float ppcr2, float * pt_add_ppcr)
{
	float ptrp;
	float num,den;

	if(pt_add_ppcr)
	{
		*pt_add_ppcr = ppcr1 * ppcr2;
	}

	num = ptrp1 * ppcr1 * ptrp2 * ppcr2;
	den = ptrp1 * ppcr2 + ptrp2 * ppcr1;
	ptrp = num/den;
	return ptrp;
}


/*******************************************************************************
* Function Name  : calc_node_ptrp
* Description    : 计算拓扑的一个节点到主节点的ptrp
* Input          : None
* Output         : None
* Return         : PTRP of the path
*******************************************************************************/
float calc_node_ptrp(NODE_HANDLE node, float * pt_ppcr)
{
	float node_ptrp, node_ppcr;
	NODE_HANDLE next_node;
	float next_ptrp, next_ppcr;
	u8 cnt = 0;
	
	if(!check_node_valid(node))
	{
		return INVALID_PTRP;
	}

	// #1. calc node_ptrp of the target node link
	node_ptrp = calc_link_ptrp(node->downlink_plan,node->downlink_snr,node->uplink_plan,node->uplink_snr,&node_ppcr);
	next_node = node->parent_node;

	// #2. uptrace to the top node and accumulate the node_ptrp
	while(next_node->parent_node)		//根节点不计算ptrp
	{
		next_ptrp = calc_link_ptrp(next_node->downlink_plan,next_node->downlink_snr,next_node->uplink_plan,next_node->uplink_snr,&next_ppcr);
		node_ptrp = add_ptrp(node_ptrp, node_ppcr, next_ptrp, next_ppcr, &node_ppcr);

		next_node = next_node->parent_node;

		if(cnt++>CFG_PIPE_STAGE_CNT)								//防止死循环
		{
#ifdef DEBUG_DISP_INFO
			my_printf("too deep node stage, error\r\n");
#endif
			return INVALID_PTRP;
		}
	}

	if(pt_ppcr)
	{
		*pt_ppcr = node_ppcr;
	}
	return node_ptrp;
}

/*******************************************************************************
* Function Name  : calc_node_add_link_ptrp
* Description    : 计算拓扑的一个节点增加一个link的ptrp
* Input          : None
* Output         : None
* Return         : PTRP of the path+link
*******************************************************************************/
float calc_node_add_link_ptrp(NODE_HANDLE node, LINK_HANDLE link, float * pt_ppcr)
{
	float node_ptrp, node_ppcr;
	float link_ptrp, link_ppcr;

	node_ptrp = calc_node_ptrp(node, &node_ppcr);
	link_ptrp = calc_link_ptrp(link->downlink_mode, link->downlink_snr, link->uplink_mode, link->uplink_snr, &link_ppcr);
	if(!node_ptrp || !link_ptrp)
	{
		return INVALID_PTRP;
	}
	else
	{
		return add_ptrp(node_ptrp,node_ppcr,link_ptrp,link_ppcr,pt_ppcr);
	}
}

/*******************************************************************************
* Function Name  : calc_path_add_link_ptrp
* Description    : 计算一个路径的ptrp基础上增加一个link的ptrp
* Input          : None
* Output         : None
* Return         : PTRP of the path+link
*******************************************************************************/
float calc_path_add_link_ptrp(float path_ptrp, float path_ppcr, LINK_HANDLE link, float * pt_ppcr)
{
	float link_ptrp, link_ppcr;

	link_ptrp = calc_link_ptrp(link->downlink_mode, link->downlink_snr, link->uplink_mode, link->uplink_snr, &link_ppcr);

	return add_ptrp(path_ptrp,path_ppcr,link_ptrp,link_ppcr,pt_ppcr);
}




/*******************************************************************************
* Function Name  : sync_pipe_to_topology
* Description    : 根据pipe修改topology
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
STATUS sync_pipe_to_topology(u16 pipe_id)
{
	u8 script_buffer[CFG_PIPE_STAGE_CNT * 8];
	u8 script_length;
	PIPE_REF pipe_ref;

	pipe_ref = inquire_pipe_mnpl_db(pipe_id);

	if(!pipe_ref)
	{
#ifdef DEBUG_INDICATOR
		my_printf("sync_pipe_to_topology(): no corresponding pipe script for pipeid:%X\r\n",pipe_id);
#endif
		return FAIL;
	}

	script_length = get_path_script_from_pipe_mnpl_db(pipe_id, script_buffer);

	return sync_path_script_to_topology(pipe_ref->phase, script_buffer, script_length);
}


/*******************************************************************************
* Function Name  : sync_path_script_to_topology
* Description    : 将路径信息同步到现有的拓扑库中, 用于将路径修改信息加入现有的拓扑
			2014-10-11
				1. 允许script中有现有网络中不存在的节点;
				2. 允许script中有其他相位的非根节点;
				3. 允许script相互矛盾, 相互矛盾的结果就是一条一条的script叠加上去, 一次一次的调整
					得到一个最终的结果(可能与第一条script矛盾)
* Input          : None
* Output         : None
* Return         : STATUS 
*******************************************************************************/
STATUS sync_path_script_to_topology(u8 phase, ARRAY_HANDLE script, u8 script_len)
{
	UID_HANDLE pt_uid;
	NODE_HANDLE parent_node;
	NODE_HANDLE target_node;
	STATUS status = FAIL;

#ifdef DEBUG_INDICATOR
	my_printf("sync_pipe_to_topology():pipe script:");
	uart_send_asc(script,script_len);
	my_printf("\r\n");
#endif

	
	/* check sript format */
	if(script_len<8)
	{
#ifdef DEBUG_INDICATOR
		my_printf("sync_pipe_to_topology():pipe script length not enough.\r\n");
#endif
		return FAIL;
	}

	if(((script_len>>3)<<3) != script_len)		//script_len必须是8的整倍数
	{
#ifdef DEBUG_INDICATOR
		my_printf("sync_pipe_to_topology():pipe script format error.\r\n");
#endif
		return FAIL;
	}


	parent_node = network_tree[phase];
	for(pt_uid=script;(pt_uid-script)<script_len;pt_uid += 8)
	{
		target_node = search_uid_in_network(pt_uid);

		// 如果uid不存在于网络内(包含三相), 则创立一个该节点
		if(!target_node)
		{
#ifdef DEBUG_INDICATOR
			my_printf("sync_pipe_to_topology(): add new uid[");
			uart_send_asc(pt_uid,6);
			my_printf("] to network.\r\n");
#endif
			target_node = add_node(parent_node); 	//add_node已经包含了节点信息初始化
			if(!target_node)
			{
#ifdef DEBUG_INDICATOR
				my_printf("sync_pipe_to_topology(): add node fail.\r\n");
#endif
				status = FAIL;
			}
			else
			{
				mem_cpy(target_node->uid,pt_uid,6);
				target_node->downlink_plan = *(pt_uid+6);
				target_node->uplink_plan = *(pt_uid+7);
				status = OKAY;
			}
		}

		/* 如果uid已经存在于网络内:
			+如果可以移动的, 移动分支;
			+如果不能移动的, 移动单个节点;
		*/
		else
		{
			status = move_branch(target_node,parent_node);
			if(!status)
			{
#ifdef DEBUG_INDICATOR
				my_printf("move_branch fail, move node only\r\n");	
#endif
				status = move_node(target_node,parent_node);
			}
		}

		if(status)
		{
			target_node->downlink_plan = *(pt_uid+6);
			target_node->uplink_plan = *(pt_uid+7);
			target_node->downlink_snr = 0;
			target_node->downlink_ss = 0;
			target_node->uplink_snr = 0;
			target_node->uplink_ss = 0;
			parent_node = target_node;
		}
		else
		{
			break;
		}
	}

#ifdef DEBUG_INDICATOR
	my_printf("sync_path_script_to_topology():return %bu\r\n",status);	
#endif
	return status;
}

/*******************************************************************************
* Function Name  : is_path_concordant
* Description    : 判断路径描述是否与当前拓扑的节点拓扑关系完全一致
* 
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
BOOL is_path_concordant_with_topology(u8 phase, ARRAY_HANDLE script, u8 script_len)
{
	u8 script_buffer[CFG_PIPE_STAGE_CNT * 8];
	u8 script_buffer_length;
	UID_HANDLE target_uid;
	NODE_HANDLE target_node;

	/* check sript format */
	if(script_len<8)
	{
#ifdef DEBUG_INDICATOR
		my_printf("is_path_concordant_with_topology():pipe script length not enough.\r\n");
#endif
		return FALSE;
	}

	if(((script_len>>3)<<3) != script_len)		//script_len必须是8的整倍数
	{
#ifdef DEBUG_INDICATOR
		my_printf("is_path_concordant_with_topology():pipe script format error.\r\n");
#endif
		return FALSE;
	}

	target_uid = script + script_len - 8;
	target_node = uid2node(target_uid);
	if(!target_node)
	{
#ifdef DEBUG_INDICATOR
		my_printf("is_path_concordant_with_topology():uid node not existed.\r\n");
#endif

		return FALSE;
	}
	script_buffer_length = get_path_script_from_topology(target_node, script_buffer);
	if(script_buffer_length==script_len)
	{
		UID_HANDLE uid1, uid2;
		uid1 = script_buffer;
		uid2 = script;
		while(uid1-script_buffer < script_buffer_length)
		{
			if(mem_cmp(uid1,uid2,6))
			{
				uid1 += 8;
				uid2 += 8;
				continue;
			}
			else
			{
				return FALSE;
			}
		}
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/*******************************************************************************
* Function Name  : update_pipe_db_in_branch
* Description    : 更新了拓扑中的一个link后, 会连带影响该link以下的所有pipe
					由于分布式存储中, 每个节点对于pipe的存储是一个节点固定一种通信方式
					因此只要一段link发生变化, 通过该link的所有pipe都会自动变化, 即该段link的branch都要变化
					CC需要将此种变化及时更新反应在pipe mnpl库中,以准确判断定时
* 
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
STATUS update_pipe_db_in_branch(NODE_HANDLE update_branch_node)
{
	static int reentrant_depth = 0;
	NODE_HANDLE sub_node;

	if(reentrant_depth++>MAX_REENTRANT_DEPTH)
	{
#ifdef DEBUG_DISP_INFO
		my_printf("max_reentrant_depth reached\r\n");
#endif	
		reentrant_depth--;
		return FAIL;
	}
	
	if(!check_node_valid(update_branch_node))
	{
		reentrant_depth--;
		return FAIL;
	}

	/* 当前节点处理 */
	{
		// 更新addr db
		{
			ADDR_REF_TYPE addr_ref;

			addr_ref = inquire_addr_db_by_uid(update_branch_node->uid);		//CC的mnpl库的uid索引也是基于addr db,只是上下行不依赖addr db.因此严格来说非一级节点可以不用做这一步
			if(addr_ref != INVALID_RESOURCE_ID)
			{
				update_addr_db(addr_ref,update_branch_node->downlink_plan,update_branch_node->uplink_plan);
			}
		}
	
		// 更新pipe mnpl db
		{
			u8 phase;
			u8 script_buffer[CFG_PIPE_STAGE_CNT * 8];
			u8 script_len;
			u16 pipe_id;


			pipe_id = node2pipeid(update_branch_node);
			if(pipe_id)
			{
				phase = get_node_phase(update_branch_node);
				script_len = get_path_script_from_topology(update_branch_node, script_buffer);
				update_pipe_mnpl_db(phase, pipe_id, script_buffer, script_len);
			}
		}
	}
	
	

	/* 迭代处理 */
	sub_node = update_branch_node->first_child_node;
	while(sub_node != NULL)
	{
		STATUS status;
		
		status = update_pipe_db_in_branch(sub_node);
		if(status == FAIL)
		{
			reentrant_depth--;
			return FAIL;
		}
		sub_node = sub_node->next_bro_node;
	}
	
	reentrant_depth--;
	return OKAY;	

	
}


/*******************************************************************************
* Function Name  : phase_diag
* Description    : 使用EBC判断模块相位差
* 
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
RESULT phase_diag(u8 phase, UID_HANDLE target_uid_handle, u8 diag_rate)
{
	EBC_BROADCAST_STRUCT es;
	u8 resp_num;
	
	mem_clr(&es, sizeof(es), 1);
	es.phase = phase;
	es.bid = 0;
	es.xmode = diag_rate|0x10;
	es.rmode = diag_rate|0x10;
	es.scan = 1;
	es.window = 0;
	es.mask = BIT_NBF_MASK_ACPHASE | BIT_NBF_MASK_UID;
	es.max_identify_try = 2;
	mem_cpy(es.uid,target_uid_handle,6);

	resp_num = root_explore(&es);
	if(resp_num)
	{
#ifdef DEBUG_INDICATOR
		my_printf("same phase\r\n");
#endif
		return CORRECT;

	}	
	else
	{
#ifdef DEBUG_INDICATOR
		my_printf("no response\r\n");
#endif
		return WRONG;
	}
}



/*******************************************************************************
* Function Name  : get_acps_relation2
* Description    : 为研究相位差问题而做
* 
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
#define 	CST_BPSK_DIFF	80			// BPSK下同相标准相位差
#define 	CST_DS15_DIFF	170			// DS15下同相标准相位差
#define 	CST_DS63_DIFF	35			// DS63下同相标准相位差

u8 get_acps_relation2(u8 mode, u8 local_ph, u8 remote_ph)
{
	unsigned char std_diff;
	unsigned char delta;
	
	// 根据数据速率选择相应实验测量相位差为标准相位差
	switch(mode & 0x03)
	{
		case(0x00):
			std_diff = CST_BPSK_DIFF;
			break;
		case(0x01):
			std_diff = CST_DS15_DIFF;
			break;
		default:
			std_diff = CST_DS63_DIFF;
			break;
	}
	
	// 计算相位差
	if(local_ph>=remote_ph)
	{
		delta = local_ph - remote_ph;
	}
	else
	{
		delta = local_ph - remote_ph + 200;
	}
	
	// 与标准相位差比较
	if(delta>=std_diff)
	{
		delta = delta - std_diff;
	}
	else
	{
		delta = delta - std_diff + 200;
	}

	return delta;
}


/*******************************************************************************
* Function Name  : phy_acps_compare2
* Description    : 为研究相位差问题而做
* 
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
u8 phy_acps_compare2(PHY_RCV_STRUCT * phy)
{
    u8 i;
	u8 phy_rcv_valid = phy->phy_rcv_valid;
	u8 delta;
	
	if((phy_rcv_valid & 0xF0) != 0x00)
	{
		for(i=0;i<4;i++)
		{	
			if(phy_rcv_valid & (0x10<<i))
			{
				delta = get_acps_relation2(phy_rcv_valid&0x03, phy->plc_rcv_acps[i], phy->plc_rcv_buffer[i][SEC_NBF_COND]);	//2012-12-10 correct ACPS should got from plc_rcv_buffer, not phy_rcv_data;
				my_printf("CH%bx: ACPS DELTA %bu\r\n",i,delta);
			}
		}
	}
	return 0;
}



/*******************************************************************************
* Function Name  : acps_diag
* Description    : 使用DST判断模块相位差, 原理是指定节点, 发一个无意义的DST转发包, 源端监听对方发出的转发包, 判断彼此相位差
					与phase_diag相比, 前者只能判断是否是一个相位, 而具体的相位差不知道, 而phase_diag_dst能够得到彼此相位差到底多少
* 
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
RESULT acps_diag(u8 phase, UID_HANDLE target_uid_handle, u8 diag_rate)
{
	DLL_SEND_STRUCT dss;
	SEND_ID_TYPE sid;
	ARRAY_HANDLE lsdu;
	u32 sticks;
	TIMER_ID_TYPE tid;
	PHY_RCV_HANDLE pp;
	u8 i;
	RESULT result = WRONG;

	lsdu = OSMemGet(SUPERIOR);
	if(!lsdu)
	{
		return WRONG;
	}

	tid = req_timer();
	if(tid == INVALID_RESOURCE_ID)
	{
		OSMemPut(SUPERIOR,lsdu);
		return WRONG;
	}

	lsdu[0] = CST_DST_PROTOCOL | get_dst_index(phase);
	lsdu[1] = 0x11;			//jumps
	lsdu[2] = 0x00;			//forward_prop
	lsdu[3] = 0x00;			//acps
	lsdu[4] = 0x00;			//networkid
	lsdu[5] = 0x00;			//timing_stamp
	lsdu[6] = 0x01;			//mghd
	lsdu[7] = 0x75;			//msdu
	
	dss.phase = phase;
	dss.target_uid_handle = target_uid_handle;
	dss.lsdu = lsdu;
	dss.lsdu_len = 8;
	dss.prop = BIT_DLL_SEND_PROP_SCAN|BIT_DLL_SEND_PROP_ACUPDATE;				//一律按acps发送
	dss.xmode = 0x10 | diag_rate;
	dss.delay = 0;
	sid = dll_send(&dss);

	if(sid==INVALID_RESOURCE_ID)
	{
		result =  WRONG;
	}

	wait_until_send_over(sid);

	sticks = phy_trans_sticks(dss.lsdu_len + LEN_LPCI + LEN_PPCI, diag_rate, 1);
	sticks = sticks + 1000;
	set_timer(tid, sticks);
#ifdef DEBUG_INDICATOR
	my_printf("waiting %ld ms\r\n",sticks);
#endif
	do
	{
		for(i=0;i<CFG_PHASE_CNT;i++)
		{
			DST_STACK_RCV_HANDLE pdst;
		
			FEED_DOG();
			pdst = &_dst_rcv_obj[i];
			if(pdst->dst_rcv_indication)
			{
				{
					if(mem_cmp(pdst->src_uid, target_uid_handle, 6))
					{
						pp = GET_PHY_HANDLE(pdst);
						my_printf("PHASE %bu:\r\n",pdst->phase);
						phy_acps_compare2(pp);
					}
				}
				dst_rcv_resume(pdst);
			}
		}
	}while(!is_timer_over(tid));

	delete_timer(tid);
	OSMemPut(SUPERIOR, lsdu);
	return result;
}

#if BRING_USER_DATA==1
/*******************************************************************************
* Function Name  : acquire_node_user_data
* Description    : 获得节点的用户携带信息
* 
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
s8 acquire_node_user_data(NODE_HANDLE target_node, ARRAY_HANDLE user_data_buffer)
{
	if(!check_node_valid(target_node) || !user_data_buffer)
	{
		return INVALID_RESOURCE_ID;
	}

	mem_cpy(user_data_buffer, target_node->user_data, target_node->user_data_len);
	return target_node->user_data_len;
}

/*******************************************************************************
* Function Name  : acquire_uid_user_data
* Description    : 指定UID获得用户携带信息
* 
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
s8 acquire_uid_user_data(UID_HANDLE target_uid, ARRAY_HANDLE user_data_buffer)
{
	NODE_HANDLE target_node;

	target_node = uid2node(target_uid);
	if(target_node)
	{
		return acquire_node_user_data(target_node, user_data_buffer);
	}
	else
	{
		return INVALID_RESOURCE_ID;
	}
}

/*******************************************************************************
* Function Name  : acquire_pipe_user_data
* Description    : 指定pipe对应的节点的用户携带信息
* 
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
s8 acquire_pipe_user_data(u16 pipe_id, ARRAY_HANDLE user_data_buffer)
{
	NODE_HANDLE target_node;

	target_node = pipeid2node(pipe_id);
	if(target_node)
	{
		return acquire_node_user_data(target_node, user_data_buffer);
	}
	else
	{
		return INVALID_RESOURCE_ID;
	}
}

/*******************************************************************************
* Function Name  : update_pipe_user_data
* Description    : 刷新用户信息
* 
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
STATUS update_pipe_user_data(u16 pipe_id)
{
	u8 return_buffer[64];
	STATUS status;
	NODE_HANDLE node;

	node = pipeid2node(pipe_id);
	if(!node)
	{
		return FAIL;
	}
	
	status = mgnt_ping(pipe_id, return_buffer);

	if(status)
	{
		/* 2014-09-24 林洋要求在组网过程中可以由MT向CC传递数据, 故增加了PING携带用户数据功能,
			用版本号区分新旧两种格式

		*/
		u32 ver_num;

		ver_num = (return_buffer[1]<<24);
		ver_num += (return_buffer[2]<<16);
		ver_num += (return_buffer[3]<<8);
		ver_num += return_buffer[4];

		if(ver_num > 0x14092400)
		{
			u8 user_data_len;

			user_data_len = return_buffer[0] - 6;
			mem_cpy(node->user_data,&return_buffer[7],user_data_len);
			node->user_data_len = user_data_len;
		}
	}

	return status;
}

/*******************************************************************************
* Function Name  : update_uid_user_data
* Description    : 传入UID首地址, 
* 
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
STATUS update_uid_user_data(UID_HANDLE target_uid)
{
	u16 pipe_id;

	pipe_id = uid2pipeid(target_uid);
	return update_pipe_user_data(pipe_id);
}
#endif

/*******************************************************************************
* Function Name  : acquire_network_appointed_stated_nodes()
* Description    : 供应用层使用的获得在build_network_by_step过程中的指定状态的节点uid集合, 可能是 refound, new_arrival, reconnect, upgrade
					取出后将状态清掉
					2014-10-16
					若传入的buffer为NULL,仅清空状态
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u16 acquire_network_appointed_stated_nodes(u8 phase, u16 node_state, u8 * nodes_buffer, u16 nodes_buffer_size)
{
	u16 node_cnt;
	u16 i;
	ARRAY_HANDLE ptw;

	if(phase > CFG_PHASE_CNT)
	{
		return 0;
	}
	
	node_cnt = get_list_from_branch(network_tree[phase], ALL, node_state, NODE_STATE_EXCLUSIVE, temp_node_list, 0, CFG_MGNT_MAX_NODES_CNT);
	
	if(nodes_buffer)
	{
		if((node_cnt)*6 > nodes_buffer_size)
		{
			node_cnt = nodes_buffer_size/6;
		}
	}

	ptw = nodes_buffer;
	for(i=0;i<node_cnt;i++)
	{
		if(nodes_buffer)
		{
			mem_cpy(ptw,temp_node_list[i]->uid,6);
			ptw += 6;
		}
		clr_node_state(temp_node_list[i],node_state);		//清除状态信息
	}
	return node_cnt;
}

/*******************************************************************************
* Function Name  : acquire_network_new_nodes()
* Description    : 获得在build_network_by_step过程中新获取的节点
					根据节点的NODE_STATE_NEW_ARRIVAL状态判断, 获取后即将其状态清除
					若传入的buffer为NULL,仅清空状态
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u16 acquire_network_new_nodes(u8 phase, u8 * nodes_buffer, u16 nodes_buffer_size)
{
	return acquire_network_appointed_stated_nodes(phase, NODE_STATE_NEW_ARRIVAL, nodes_buffer, nodes_buffer_size);
}

/*******************************************************************************
* Function Name  : acquire_network_refound_nodes()
* Description    : 获得在build_network_by_step过程中的refound节点(重新响应explore广播并处理为完备节点的节点)
					获取后状态清零, 若传入的buffer为NULL,仅清空状态
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u16 acquire_network_refound_nodes(u8 phase, u8 * nodes_buffer, u16 nodes_buffer_size)
{
	return acquire_network_appointed_stated_nodes(phase, NODE_STATE_REFOUND, nodes_buffer, nodes_buffer_size);
}

/*******************************************************************************
* Function Name  : acquire_network_reconnect_nodes()
* Description    : 获得在优化过程中的修复的节点(RECONNECT)
					获取后状态清零, 若传入的buffer为NULL,仅清空状态
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u16 acquire_network_reconnect_nodes(u8 phase, u8 * nodes_buffer, u16 nodes_buffer_size)
{
	return acquire_network_appointed_stated_nodes(phase, NODE_STATE_RECONNECT, nodes_buffer, nodes_buffer_size);
}

/*******************************************************************************
* Function Name  : acquire_network_upgrade_nodes()
* Description    : 获得在优化过程中链路优化过的节点(UPGRADE)
					获取后状态清零, 若传入的buffer为NULL,仅清空状态
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u16 acquire_network_upgrade_nodes(u8 phase, u8 * nodes_buffer, u16 nodes_buffer_size)
{
	return acquire_network_appointed_stated_nodes(phase, NODE_STATE_UPGRADE, nodes_buffer, nodes_buffer_size);
}


/*******************************************************************************
* Function Name  : update_topology_link()
* Description    : 更新拓扑链路的信息
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS update_topology_link(NODE_HANDLE target_node, LINK_HANDLE new_link_info)
{
	if(!check_node_valid(target_node))
	{
		return FAIL;
	}
	else
	{
		target_node->uplink_plan = new_link_info->uplink_mode;
		target_node->uplink_ss = new_link_info->uplink_ss;
		target_node->uplink_snr = new_link_info->uplink_snr;
		target_node->downlink_plan = new_link_info->downlink_mode;
		target_node->downlink_ss = new_link_info->downlink_ss;
		target_node->downlink_snr = new_link_info->downlink_snr;
		return OKAY;
	}
}


/*******************************************************************************
* Function Name  : get_topology_subnodes_cnt()
* Description    : 获得相位的子节点总数
* Author		 : Lvhaifeng
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u16 get_topology_subnodes_cnt(u8 phase)
{
	if(phase < CFG_PHASE_CNT)
	{
		return get_list_from_branch(network_tree[phase], ALL, ALL, ALL, NULL,0,0)-1;
	}
	else
	{
		return 0;
	}
}

/*******************************************************************************
* Function Name  : get_dst_optimal_windows()
* Description    : 根据节点数规模得到洪泛窗口数
* Author		 : Lvhaifeng
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u8 get_dst_optimal_windows(u16 num_nodes)
{
	u8 windows;

	if(num_nodes<8)
	{
		windows = 2;
	}
	else if(num_nodes<16)
	{
		windows = 3;
	}
	else
	{
		windows = 4;
	}
	return windows;
}

#define RESTORE_BITS	3
/*******************************************************************************
* Function Name  : s2u3b
* Description    : 3bit有符号数转无符号数
* Author		 : Lvhaifeng
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u16 s2u3b(s16 s)
{
	u16 u;
	
	u = (s>=0) ? s : ((1<<RESTORE_BITS) + s);

	return u;
}

/*******************************************************************************
* Function Name  : u2s3b
* Description    : 3bit无符号数转有符号数
* Author		 : Lvhaifeng
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
s16 u2s3b(u16 u)
{
	s16 s;

	s = (u>=(1<<(RESTORE_BITS-1))) ? (u - (1<<RESTORE_BITS)):(u);
	
	return s;
}


/******************* (C) COPYRIGHT 2012 Belling *****END OF FILE****/
