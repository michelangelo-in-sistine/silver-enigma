

/* Includes ------------------------------------------------------------------*/
#include "powermesh_include.h"
#include "stdlib.h"

/* Constant Define ------------------------------------------------------------------*/
#define NODES_STORAGE_START_ADDR	((u32)(nodes_storage_entities))
#define NODES_STORAGE_END_ADDR		((u32)(nodes_storage_entities) + (u32)((CFG_MGNT_MAX_NODES_CNT-1) * sizeof(NODE_STRUCT)))


/* Private Variables ------------------------------------------------------------------*/
u16 nodes_usage;
u8 nodes_storage_entities_flag[CFG_MGNT_MAX_NODES_CNT/8+1];
NODE_STRUCT nodes_storage_entities[CFG_MGNT_MAX_NODES_CNT];

/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : u16 get_node_usage(void)
* Description    : ���������ܽڵ���
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
u16 get_node_usage(void)
{
	return nodes_usage;
}

/*******************************************************************************
* Function Name  : check_node_valid()
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
BOOL check_node_valid(NODE_STRUCT * node)
{
	u16	index;
	u16 i;
	u8  j;
	BOOL value = FALSE;

	if((u32)node < NODES_STORAGE_START_ADDR)
	{
#ifdef DEBUG_TOPOLOGY
		my_printf("node addr is beyond storage bottom bound\r\n");
#endif
	}
	else if((u32)node > NODES_STORAGE_END_ADDR)
	{
#ifdef DEBUG_TOPOLOGY
		my_printf("node addr is beyond storage upper bound\r\n");
#endif
	}
	else
	{
		index = ((u32)(node) - (u32)(nodes_storage_entities)) / sizeof(NODE_STRUCT);
		i = index / 8;
		j = index % 8;
		if(nodes_storage_entities_flag[i] & (0x01<<j))
		{
			value = TRUE;
		}
		else
		{
		}
	}
	
	if(value == FALSE)
	{
		my_printf("error node:\r\n");
		print_error_node(node);
	}
	
	return value;
}

/*******************************************************************************
* Function Name  : init_node()
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void init_node(NODE_STRUCT * root_node)
{
	int i;
	u8 * pt;
	
	if(!check_node_valid(root_node))
	{
		return;
	}

	pt = (u8 *)(root_node);
	for(i=0;i<sizeof(NODE_STRUCT);i++)
	{
		*(pt++) = 0;
	}
}

/*******************************************************************************
* Function Name  : get_node_storage()
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
NODE_STRUCT * get_node_storage()
{
	u16 i;
	u8  j;
	u8  flag;

	if(nodes_usage>=CFG_MGNT_MAX_NODES_CNT)
	{
#ifdef DEBUG_TOPOLOGY
		my_printf("node storage is full!\r\n");
#endif
		return NULL;
	}
	else
	{
		NODE_STRUCT * new_node;

		for(i=0;i<CFG_MGNT_MAX_NODES_CNT/8+1;i++)
		{
			flag = nodes_storage_entities_flag[i];
			if(flag != 0xFF)
			{
				break;
			}
		}

		for(j=0;j<8;j++)
		{
			if((0x01<<j) & (~flag))
			{
				break;
			}
		}

		// i*8 + j is the available storage index

		nodes_storage_entities_flag[i] |= (0x01<<j);
		new_node = &nodes_storage_entities[i*8+j];
		init_node(new_node);
		nodes_usage++;
		return new_node;
	}	
}

/*******************************************************************************
* Function Name  : delete_node_storage()
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS delete_node_storage(NODE_STRUCT * node_to_delete)
{
	u16	index;
	u16 i;
	u8  j;
	
	if(!check_node_valid(node_to_delete))
	{
#ifdef DEBUG_TOPOLOGY
		my_printf("can't delete a empty storage!\r\n");
#endif
		return FAIL;
	}
	
	index = ((u32)(node_to_delete) - (u32)(nodes_storage_entities)) / sizeof(NODE_STRUCT);
	i = index / 8;
	j = index % 8;
	
	init_node(node_to_delete);

	nodes_storage_entities_flag[i] &= (~(0x01<<j));
	nodes_usage--;
	return OKAY;
}


/* Public functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : init_topology()
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void init_topology(void)
{
	u16 i;

	nodes_usage = 0;
	for(i=0;i<(CFG_MGNT_MAX_NODES_CNT/8+1);i++)
	{
		nodes_storage_entities_flag[i] = 0;
	}
}


/*******************************************************************************
* Function Name  : creat_path_tree_root()
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
NODE_STRUCT * creat_path_tree_root()
{
	NODE_STRUCT * root_node;

	root_node = get_node_storage();
	return root_node;
}

/*******************************************************************************
* Function Name  : add_node()
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
NODE_STRUCT * add_node(NODE_STRUCT * parent_node)
{
	NODE_STRUCT * new_node;
	NODE_STRUCT * last_child_node;

	if(!check_node_valid(parent_node))
	{
#ifdef DEBUG_TOPOLOGY
		my_printf("new node can't add to a invalid node!\r\n");
#endif
		return NULL;
	}
	
	if(parent_node->cnt_child_nodes >= CFG_TOPOLOGY_MAX_NODE_CHILDREN)		// if node has too many children, return null 
	{
		return NULL;
	}
	else
	{
		new_node = get_node_storage();
		if(new_node != NULL)
		{
			init_node(new_node);

			if(parent_node->first_child_node == NULL)
			{
				parent_node->first_child_node = new_node;		//up relation 
				parent_node->cnt_child_nodes = 1;
				new_node->parent_node = parent_node;
			}
			else
			{
				last_child_node = parent_node->first_child_node;
				while(last_child_node->next_bro_node != NULL)
				{
					last_child_node = last_child_node->next_bro_node;
				}
				parent_node->cnt_child_nodes++;					//up relation 
				new_node->parent_node = parent_node;

				last_child_node->next_bro_node = new_node;		//left relation
				new_node->previous_bro_node = last_child_node;
			}
			
		}
		return new_node;
	}
}

/*******************************************************************************
* Function Name  : delete_node()
* Description    : ɾ���ڵ�, �豣֤���ӽڵ���ĿΪ0
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS delete_node(NODE_STRUCT * node_to_delete)
{
	NODE_STRUCT * previous_node;
	NODE_STRUCT * next_node;
	NODE_STRUCT * parent_node;
	static int reentrant_depth = 0;
	
	if(reentrant_depth++>MAX_REENTRANT_DEPTH)
	{
#ifdef DEBUG_DISP_INFO
		my_printf("max_reentrant_depth reached\r\n");	
#endif	
		reentrant_depth--;				//�����ݹ�㼶����,˵��������˴�����,��һֱ���˵����
		return FAIL;
	}
		
	if(!check_node_valid(node_to_delete))
	{
#ifdef DEBUG_TOPOLOGY
		my_printf("node_to_delete not valid!\r\n");
#endif
		reentrant_depth--;
		return FAIL;
	}

	if(node_to_delete->cnt_child_nodes != 0)
	{
#ifdef DEBUG_TOPOLOGY
		my_printf("sub nodes of the node to delete is not empty!\r\n");
#endif
		reentrant_depth--;
		return FAIL;
	}

	previous_node = node_to_delete->previous_bro_node;
	parent_node = node_to_delete->parent_node;
	next_node = node_to_delete->next_bro_node;
	
	if(parent_node != NULL)
	{
		if(previous_node == NULL)						// if this node is the first child
		{
			parent_node->first_child_node = node_to_delete->next_bro_node;
			parent_node->cnt_child_nodes--;
			if(next_node != NULL)
			{
				next_node->previous_bro_node = NULL;
			}
		}
		else
		{
			parent_node->cnt_child_nodes--;
			previous_node->next_bro_node = node_to_delete->next_bro_node;
			if(next_node != NULL)
			{
				next_node->previous_bro_node = previous_node;
			}
		}
	}		// if parent_node is NULL, node is the root node, and root node is always single;

	delete_node_storage(node_to_delete);
	reentrant_depth--;
	return OKAY;
}

/*******************************************************************************
* Function Name  : move_node()
* Author		 : Lv Haifeng
* Date			 : 2014-10-11
* Description    : ���ƶ��ڵ�
					* ����ڵ��ƶ����Ǳ�����������������ȥ, ����ڵ���ӽڵ�
					* �����ӽڵ�ĸ��ڵ�, �����ƶ�; ���ӽڵ�ĸ��ڵ�, �������ƶ�;
					* ����µ�parent�뵱ǰparentһ��, ����Ҫ�䶯;
					* �Ǹ��ڵ�, ������ӽڵ�, ���Ƚ��ӽڵ��и������ڵ�, Ȼ���ƶ�;
					* ���new_parentΪNULL, ��ɾ���ڵ�;
					
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS move_node(NODE_HANDLE node_to_move, NODE_HANDLE new_parent)
{
	NODE_HANDLE parent_node;
	STATUS status = OKAY;

	if(!check_node_valid(node_to_move))
	{
#ifdef DEBUG_TOPOLOGY
		my_printf("node to move not valid!\r\n");
#endif
		return FAIL;
	}

	if(new_parent && !check_node_valid(new_parent))			//���Ŀ��ڵ㲻Ϊ��, ����ַ��Ч��
	{
#ifdef DEBUG_TOPOLOGY
		my_printf("new parent not valid!\r\n");
#endif
		return FAIL;
	}

	if(node_to_move->parent_node == NULL)
	{
		if(node_to_move->cnt_child_nodes != 0)
		{
#ifdef DEBUG_INDICATOR
			my_printf("root node with children can't be moved.\r\n");
#endif
			return FAIL;
		}
	}

	parent_node = node_to_move->parent_node;

	if(parent_node != new_parent)
	{
		//�й�: ���Ƚ������ӽڵ��������ڵ�
		while(node_to_move->first_child_node)
		{
			status = move_branch(node_to_move->first_child_node,node_to_move->parent_node);
			if(!status)
			{
				break;
			}
		}

		//�ƶ��ڵ�, ��Ŀ�Ľڵ��ַΪNULL, ɾ���ڵ�
		if(status)
		{

			if(new_parent)
			{
				status = move_branch(node_to_move, new_parent);
			}
			else
			{
				status = delete_node(node_to_move);
			}
		}
	}
	return status;
}


/*******************************************************************************
* Function Name  : delete_branch()
* Description    : �ݹ�ɾ���ڵ㼰�������������ӽڵ���ɵķ�֧
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS delete_branch(NODE_STRUCT * node_to_delete)
{
	NODE_STRUCT * sub_node;
	static int reentrant_depth = 0;
	STATUS status;
	
	if(reentrant_depth++>MAX_REENTRANT_DEPTH)
	{
#ifdef DEBUG_DISP_INFO
		my_printf("max_reentrant_depth reached\r\n");
#endif	
		reentrant_depth--;
		return FAIL;
	}
	
	if(!check_node_valid(node_to_delete))
	{
#ifdef DEBUG_TOPOLOGY
		my_printf("can't delete a empty node!\r\n");
#endif
		reentrant_depth--;
		return FAIL;
	}

	sub_node = node_to_delete->first_child_node;
	while(sub_node != NULL)
	{
		status = delete_branch(sub_node);
		if(status == FAIL)
		{
			reentrant_depth--;
			return FAIL;								//2013-09-23 ��ɾ��ʧ��ʱ, ����ѭ��, ������ѭ��
		}
		sub_node = node_to_delete->first_child_node;
	}

	status = delete_node(node_to_delete);
	reentrant_depth--;
	return status;
}

/*******************************************************************************
* Function Name  : is_child_node()
* Description    : �жϽڵ��Ƿ���ָ���ڵ���ӽڵ�
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
BOOL is_child_node(NODE_HANDLE target_node, NODE_HANDLE parent_node)
{
	NODE_HANDLE sub_node;

	if(!check_node_valid(parent_node) || !check_node_valid(target_node))
	{
#ifdef DEBUG_DISP_INFO
		my_printf("is_node_sub_node(): node invalid.\r\n");
#endif	
		return FALSE;
	}

	sub_node = parent_node->first_child_node;
	while(sub_node != NULL)
	{
		if(target_node == sub_node)
		{
			return TRUE;
		}
		sub_node = sub_node->next_bro_node;
	}
	return FALSE;
}

/*******************************************************************************
* Function Name  : is_node_in_branch()
* Description    : �����ڵ��Ƿ����ڵ�ǰ��֧, ������Χ�������ӽڵ�
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
BOOL is_node_in_branch(NODE_STRUCT * node_to_search, NODE_STRUCT * branch_root_node)
{
	BOOL search_result;
	NODE_STRUCT * sub_node;
	static int reentrant_depth = 0;
	
	if(reentrant_depth++>MAX_REENTRANT_DEPTH)
	{
#ifdef DEBUG_DISP_INFO
		my_printf("max_reentrant_depth reached\r\n");
#endif	
		reentrant_depth--;
		return FALSE;
	}
	
	if(branch_root_node == node_to_search)
	{
		reentrant_depth--;
		return TRUE;
	}
	else
	{
		sub_node = branch_root_node->first_child_node;
		while(sub_node != NULL)
		{
			search_result = is_node_in_branch(node_to_search, sub_node);
			if(search_result == FALSE)
			{
				sub_node = sub_node->next_bro_node;
			}
			else
			{
				reentrant_depth--;
				return TRUE;
			}
		}
		reentrant_depth--;
		return FALSE;
	}
}

/*******************************************************************************
* Function Name  : get_node_depth()
* Description    : �õ��ڵ����������е����, 0Ϊ���ڵ�, 1Ϊ1��, 2Ϊ2��...
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u8 get_node_depth(NODE_HANDLE target_node)
{
	static int reentrant_depth = 0;
	
	if(reentrant_depth++>MAX_REENTRANT_DEPTH)
	{
#ifdef DEBUG_DISP_INFO
		my_printf("max_reentrant_depth reached\r\n");
#endif	
		reentrant_depth--;
		return 0;
	}

	if(target_node->parent_node == NULL)
	{
		reentrant_depth--;
		return 0;
	}
	else
	{
		u8 parent_node_depth;
		parent_node_depth = get_node_depth(target_node->parent_node);
		reentrant_depth--;
		
		return  parent_node_depth + 1;
	}
}


/*******************************************************************************
* Function Name  : search_uid_in_branch()
* Description    : ����ָ��UID�Ľڵ�, ���������ַ
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
NODE_STRUCT * search_uid_in_branch(NODE_STRUCT * branch_root_node, u8 * uid)
{
	NODE_STRUCT * search_result;
	NODE_STRUCT * sub_node;
	static int reentrant_depth = 0;
	
	if(reentrant_depth++>MAX_REENTRANT_DEPTH)
	{
#ifdef DEBUG_DISP_INFO
		my_printf("max_reentrant_depth reached\r\n");
#endif	
		reentrant_depth--;
		return 0;
	}

	// 2014-11-20 �㲥��ַ��ͨ���ַ����Ҫ��������
	if((check_uid(0, uid)==SOMEBODY) || (check_uid(0, uid)==BROADCAST))
	{
		reentrant_depth--;
		return 0;
	}
	
	if(mem_cmp(branch_root_node->uid, uid,6) == TRUE)
	{
		reentrant_depth--;
		return branch_root_node;
	}
	else
	{
		sub_node = branch_root_node->first_child_node;
		while(sub_node != NULL)
		{
			search_result = search_uid_in_branch(sub_node, uid);
			if(search_result == NULL)
			{
				sub_node = sub_node->next_bro_node;
			}
			else
			{
				reentrant_depth--;
				return search_result;
			}
		}
		reentrant_depth--;
		return NULL;
	}
}

/*******************************************************************************
* Function Name  : get_list()
* Description    : �ݹ��������������, �õ�ָ���ڵ��֧�µ��������������Ľڵ��ַ�б�
					Ҫ��ֹ��list_bufferд��Խ��
					��list_buffer_to_write��������NULL,��������ΪALL,���Եõ��������е�Ԫ����Ŀ
					2013-02-19 �����Խ�紦��

					2013-05-09 ����ڵ�״̬����
					state_filt_in_cond:		������һ�����Ľڵ㽫����¼��list��, Ϊ0��don't care;
					state_filt_out_cond:	������һ�����Ľڵ㽫���ų�����, Ϊ0��don't care;
					������ͬʱ����ʱ, state_filt_out_condǿ��state_filt_in_cond
* Input          : 
* Output         : 
* Return         : ����list����, 
*******************************************************************************/
u16 get_list_from_branch(NODE_STRUCT * branch_root_node, TOPOLOGY_COND_ENUM topology_cond, u16 state_filt_in_cond, u16 state_filt_out_cond, NODE_HANDLE * list_buffer_to_write, u16 list_buffer_usage, u16 list_buffer_depth)
{
	NODE_STRUCT * sub_node;
	u16	list_increment;
	static int reentrant_depth = 0;
	
	if(reentrant_depth++>MAX_REENTRANT_DEPTH)
	{
#ifdef DEBUG_DISP_INFO
		my_printf("max_reentrant_depth reached\r\n");
#endif
		reentrant_depth--;
		return 0;
	}
	
	list_increment = 0;

	if(!state_filt_in_cond || (branch_root_node->state & state_filt_in_cond))	//�ж�filter_in����, ��filter_inΪ0����������¼, �粻Ϊ0��������һһ������¼
	{
		if(!(branch_root_node->state & state_filt_out_cond))			//�ڵ�״̬λ������һλ��filter����һ������¼��list��
		{
			switch(topology_cond)
			{
				case(ALL):
					if(list_buffer_to_write)
					{
						if(list_buffer_usage<list_buffer_depth)
						{
							//*(list_buffer_to_write++) = branch_root_node;
							list_buffer_to_write[list_buffer_usage] = branch_root_node;		//�޸ĳ�ͷ�̶�, ÿ��index����, ��������֧��һ��bufferѭ�����ε���
							list_increment++;				//list_increment is the list length increment in this procedure.
							list_buffer_usage++;			//list_buffer_usage is the total list increment in total reentrant procudure, to avoid write-out-of-range failure.
						}
						else
						{
#ifdef DEBUG_DISP_INFO
							my_printf("list buffer out of range.\r\n");
#endif
						}
					}
					else
					{
						list_increment++;
					}
				break;
				case(SOLO):
					if(branch_root_node->cnt_child_nodes == 0)
					{
						if(list_buffer_to_write)
						{
							if(list_buffer_usage<list_buffer_depth)
							{
								//*(list_buffer_to_write++) = branch_root_node;
								list_buffer_to_write[list_buffer_usage] = branch_root_node;
								list_increment++;
								list_buffer_usage++;
							}
							else
							{
#ifdef DEBUG_DISP_INFO
								my_printf("list buffer out of range.\r\n");
#endif
							}					
						}
						else
						{
							list_increment++;
						}
					}
				break;
				case(NON_SOLO):
					if(branch_root_node->cnt_child_nodes != 0)
					{
						if(list_buffer_to_write)
						{
							if(list_buffer_usage<list_buffer_depth)
							{
								//*(list_buffer_to_write++) = branch_root_node;
								list_buffer_to_write[list_buffer_usage] = branch_root_node;
								list_increment++;
								list_buffer_usage++;
							}
							else
							{
#ifdef DEBUG_DISP_INFO
								my_printf("list buffer out of range.\r\n");
#endif
							}					
						}
						else
						{
							list_increment++;
						}
					}
				break;
				default:
#ifdef DEBUG_TOPOLOGY
				my_printf("unknown search condition\r\n");
#endif
				break;
			}
		}
		else
		{
//#ifdef DEBUG_TOPOLOGY
//			my_printf("node[");
//			uart_send_asc(branch_root_node->uid,6);
//			my_printf("] is filtered out.\r\n");
//#endif
		}
	}

	sub_node = branch_root_node->first_child_node;
	while(sub_node != NULL)
	{
		u16 list_sub_increment;
		
		list_sub_increment = get_list_from_branch(sub_node,topology_cond,state_filt_in_cond,state_filt_out_cond,list_buffer_to_write,list_buffer_usage,list_buffer_depth);
		list_increment += list_sub_increment;
		list_buffer_usage += list_sub_increment;
//		if(list_buffer_to_write)
//		{
//			list_buffer_to_write += list_sub_increment;
//		}

		sub_node = sub_node->next_bro_node;
	}
	reentrant_depth--;
	return list_increment;
}

/*******************************************************************************
* Function Name  : randomize_list()
* Description    : ���ڵ��б����������
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void randomize_list(NODE_HANDLE* list, u8 list_len)
{
	u8 i;
	u8 dice;
	NODE_HANDLE temp;

	for(i=0;i<list_len;i++)
	{
		dice = (rand() % list_len);
		temp = list[i];
		list[i] = list[dice];
		list[dice] = temp;
	}
	
}

/*******************************************************************************
* Function Name  : move_branch()
* Description    : ֧��
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS move_branch(NODE_STRUCT * branch_node_to_move, NODE_STRUCT * new_parent_node)
{
	NODE_STRUCT * old_parent;
	NODE_STRUCT * old_previous_bro;
	NODE_STRUCT * old_next_bro;
	NODE_STRUCT * new_previous_bro;
	static int reentrant_depth = 0;
	
	if(reentrant_depth++>MAX_REENTRANT_DEPTH)
	{
#ifdef DEBUG_DISP_INFO
		my_printf("max_reentrant_depth reached\r\n");
#endif	
		reentrant_depth--;
		return FAIL;
	}
	
	if(!check_node_valid(branch_node_to_move) || !check_node_valid(new_parent_node))
	{
		reentrant_depth--;
		return FAIL;
	}
	
	
	// Check to make sure the new parent is not the sub node of the branch.
	// Otherwise, a loop branch will generate and big problem will be generated.
	if(is_node_in_branch(new_parent_node, branch_node_to_move))
	{
#ifdef DEBUG_TOPOLOGY
		my_printf("new parent can't be the sub node of the branch to move.\r\n");
#endif
		reentrant_depth--;
		return FAIL;
	}

	if(branch_node_to_move->parent_node == NULL)
	{
#ifdef DEBUG_TOPOLOGY
		my_printf("root node can't be moved.\r\n");
#endif
		reentrant_depth--;
		return FAIL;
	}

	if(is_child_node(branch_node_to_move, new_parent_node))
	{
//#ifdef DEBUG_TOPOLOGY
//		my_printf("node in branch already.\r\n");
//#endif
		reentrant_depth--;
		return OKAY;
	}

	old_parent = branch_node_to_move->parent_node;
	old_previous_bro = branch_node_to_move->previous_bro_node;
	old_next_bro = branch_node_to_move->next_bro_node;

	if(old_previous_bro == NULL)				//first child
	{
		old_parent->cnt_child_nodes--;
		old_parent->first_child_node = branch_node_to_move->next_bro_node;
		if(old_next_bro != NULL)
		{
			old_next_bro->previous_bro_node = NULL;		// right bro will be new first child node;
		}
	}
	else
	{
		old_parent->cnt_child_nodes--;
		old_previous_bro->next_bro_node = branch_node_to_move->next_bro_node;
		if(old_next_bro != NULL)
		{
			old_next_bro->previous_bro_node = old_previous_bro;
		}
	}
	
	if(new_parent_node->first_child_node == NULL)
	{
		new_parent_node->first_child_node = branch_node_to_move;
		new_parent_node->cnt_child_nodes = 1;
		branch_node_to_move->parent_node = new_parent_node;
		branch_node_to_move->previous_bro_node = NULL;
		branch_node_to_move->next_bro_node = NULL;
	}
	else
	{
		new_previous_bro = new_parent_node->first_child_node;
		while(new_previous_bro->next_bro_node != NULL)
		{
			new_previous_bro = new_previous_bro->next_bro_node;
		}
		new_previous_bro->next_bro_node = branch_node_to_move;
		branch_node_to_move->previous_bro_node = new_previous_bro;
		branch_node_to_move->parent_node = new_parent_node;
		branch_node_to_move->next_bro_node = NULL;
		new_parent_node->cnt_child_nodes++;
	}
	reentrant_depth--;
	return OKAY;
}

/*******************************************************************************
* Function Name  : set_node_uid()
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS set_node_uid(NODE_STRUCT * node, u8 * target_uid)
{
	if(!check_node_valid(node))
	{
		return FAIL;
	}

	mem_cpy(node->uid, target_uid, 6);
	return OKAY;
}

/*******************************************************************************
* Function Name  : get_path_script_from_topology()
* Description    : ���������еõ���CC��node��·������, Ҫ�����еĸ��ڵ㶼����ON_LINE
					
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u8 get_path_script_from_topology(NODE_STRUCT * node, u8 * script_buffer)
{
	NODE_STRUCT * node_path[MAX_PIPE_DEPTH];
	u8 pipe_stage;
	u8 pipe_script_len;
	u8 i;
	

	// ���ݵ�root, ͬʱ��¼�����ݵ�·��, ѭ��������, ���һ��node��ַ��һ���ڵ��ַ
	pipe_stage = 0;
	pipe_script_len = 0;
	
	while(node->parent_node != NULL)
	{
		//if(node->parent_node->state == ON_LINE)
		{
			node_path[pipe_stage] = node;
			node = node->parent_node;
			pipe_stage++;
		}
//		else
//		{
//			return 0;
//		}
	}

	
	while(pipe_stage != 0)
	{
		node = node_path[pipe_stage-1];
		for(i=0;i<6;i++)
		{
			*(script_buffer++) = node->uid[i];
		}
		*(script_buffer++) = node->downlink_plan;
		*(script_buffer++) = node->uplink_plan;
		pipe_script_len += 8;
		pipe_stage--;
	}

	return pipe_script_len;
}

/*******************************************************************************
* Function Name  : set_branch()
* Description    : �Է�֧�ϵ����нڵ����Խ�������
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS set_branch_property(NODE_HANDLE branch_root_node, NODE_PROPERTY_ENUM property, u8 set_value)
{
	NODE_STRUCT * sub_node;
	static int reentrant_depth = 0;
	
	if(reentrant_depth++>MAX_REENTRANT_DEPTH)
	{
#ifdef DEBUG_DISP_INFO
		my_printf("set_branch_property():max_reentrant_depth reached\r\n");
#endif	
		reentrant_depth--;
		return FAIL;
	}

	if(!check_node_valid(branch_root_node))
	{
#ifdef DEBUG_DISP_INFO
		my_printf("set_branch_property():error execute node\r\n");
#endif	
		reentrant_depth--;
		return FAIL;
	}

	switch(property)
	{
		case(NODE_BROAD_CLASS):
		{
			branch_root_node->broad_class = set_value;
			break;
		}
		case(NODE_DISCONNECT):
		{
			branch_root_node->state |= NODE_STATE_DISCONNECT;
			break;
		}
		// �����Խڵ���������ô���
	}

	/* �������� */
	sub_node = branch_root_node->first_child_node;
	while(sub_node != NULL)
	{
		STATUS status;
		
		status = set_branch_property(sub_node, property, set_value);	// д����Ҫ����, ������Ч�Լ��
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
* Function Name  : clr_branch_state()
* Description    : �Է�֧�ϵ����нڵ����Խ�������
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS clr_branch_state(NODE_HANDLE branch_root_node, u8 clr_value)
{
	NODE_STRUCT * sub_node;
	static int reentrant_depth = 0;
	
	if(reentrant_depth++>MAX_REENTRANT_DEPTH)
	{
#ifdef DEBUG_DISP_INFO
		my_printf("set_branch_property():max_reentrant_depth reached\r\n");
#endif	
		reentrant_depth--;
		return FAIL;
	}

	if(!check_node_valid(branch_root_node))
	{
#ifdef DEBUG_DISP_INFO
		my_printf("set_branch_property():error execute node\r\n");
#endif	
		reentrant_depth--;
		return FAIL;
	}

	branch_root_node->state &= (~clr_value);

	/* �������� */
	sub_node = branch_root_node->first_child_node;
	while(sub_node != NULL)
	{
		STATUS status;
		
		status = clr_branch_state(sub_node, clr_value);	// д����Ҫ����, ������Ч�Լ��
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
* Function Name  : print_branch()
* Description    : 2013-02-05 ����ݹ�����������, ��Ҫ��ÿ��������ڶ��Ծ�̬��������һ
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void print_branch(NODE_STRUCT * node)
{
	NODE_STRUCT * sub_node;
	static int reentrant_depth = 0;
	
	if(reentrant_depth++>MAX_REENTRANT_DEPTH)
	{
#ifdef DEBUG_DISP_INFO
		my_printf("max_reentrant_depth reached\r\n");
#endif
		reentrant_depth--;
		return;
	}
	
	if(!check_node_valid(node))
	{
		reentrant_depth--;
		return;
	}

	//my_printf("[BRANCH:");

	sub_node = node->first_child_node;			// only print the non-solo branch 
	if(sub_node != NULL)
	{
		my_printf("[");
		uart_send_asc(node->uid,6);
		while(sub_node != NULL)
		{
			my_printf(" ");
			uart_send_asc(sub_node->uid,6);
			//my_printf("%bX%bX ",sub_node->uplink_plan,sub_node->downlink_plan);
			sub_node = sub_node->next_bro_node;
		}
		my_printf("]\r\n");
		
		sub_node = node->first_child_node;
		while(sub_node != NULL)
		{
			print_branch(sub_node);
			sub_node = sub_node->next_bro_node;
		}
	}



	reentrant_depth--;
	return;
}

/*******************************************************************************
* Function Name  : print_list()
* Description    : 2013-02-05 ����ݹ�����������, ��Ҫ��ÿ��������ڶ��Ծ�̬��������һ
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void print_list(NODE_HANDLE * node_list, u8 list_len)
{
	u8 i;
	NODE_HANDLE node;
	static int reentrant_depth = 0;
	
	if(reentrant_depth++>MAX_REENTRANT_DEPTH)
	{
#ifdef DEBUG_DISP_INFO
		my_printf("max_reentrant_depth reached\r\n");
#endif
		reentrant_depth--;
		return;
	}

	my_printf("{");
	for(i=0;i<list_len;i++)
	{
		node = node_list[i];
		uart_send_asc(node->uid,6);
		if(i != (list_len-1))
		{
			my_printf(" ");
		}
	}
	my_printf("}\r\n");
	reentrant_depth--;
	return;
}

/*******************************************************************************
* Function Name  : back_trace()
* Author		 : Lvhaifeng
* Date		     : 2014-10-15
* Description	 : ���ݵݹ��㷨, �����ڲ��������˵�ǰ����, �������е�ĳ���ڵ㿪ʼ, 
*					�Ե�����ʽ����������, ����Ѱ��ʧ�ٵĽڵ�
*					���ֵ��ݱ����ȼ������Ƚ��õ���Ϊ���ڵ㽫���������������γɵ���������, 
*					(������Ĺ���������ɸ��ڵ�ǣ��, �򲻱�; �����ӽڵ�ǣ��, 
*					��ԭ�и��ڵ������һ���ӽڵ�)
*					���Եݹ�ķ�ʽ�����µ�������
*					�������ֵ��ݱ����㷨�Ĵ����ǹ̶���, ��˿���ָ��һ������, ������õ�ʱ��
*					ÿ�ν����������һ, �Ϳ���һ��һ����������������, ����������ؽ��µ�������;
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
NODE_HANDLE back_trace(NODE_HANDLE back_parent_node, NODE_HANDLE current_node, int back_steps)
{
	static int reentrant_depth = 0;
	static int back_steps_cnt;
	NODE_HANDLE sub_node;
	NODE_HANDLE parrent_node;
	NODE_HANDLE target_node;

	if(reentrant_depth == 0)
	{
		back_steps_cnt = 0;
	}
	
	if(reentrant_depth++>MAX_REENTRANT_DEPTH)
	{
#ifdef DEBUG_DISP_INFO
		my_printf("max_reentrant_depth reached\r\n");
#endif	
		reentrant_depth--;
		return NULL;
	}	



	sub_node = current_node->first_child_node;
	parrent_node = current_node->parent_node;

	if(back_steps_cnt == back_steps)
	{
		reentrant_depth--;
		return current_node;
	}
	else
	{
		back_steps_cnt++;
		while(sub_node != NULL)
		{
			if(back_parent_node != sub_node)
			{
				target_node = back_trace(current_node, sub_node, back_steps);
				if(target_node)
				{
					reentrant_depth--;
					return target_node;
				}
			}
			sub_node = sub_node->next_bro_node;
		}

		if(back_parent_node != parrent_node)
		{
			if(parrent_node)
			{
				target_node = back_trace(current_node, parrent_node, back_steps);
				if(target_node)
				{
					reentrant_depth--;
					return target_node;
				}
			}
		}
		reentrant_depth--;
		return NULL;
	}
}


/*******************************************************************************
* Function Name  : print_error_node()
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void print_error_node(NODE_HANDLE error_node)
{
	my_printf("/*-------------------------\r\n");
	my_printf("ERROR NODE ADDR:0x%lx",(u32)(error_node));
	my_printf("\r\n");
	my_printf("NODE DATA:");
	uart_send_asc((ARRAY_HANDLE)error_node,sizeof(NODE_STRUCT));
	my_printf("\r\n");

	my_printf("UID:[");
	uart_send_asc(error_node->uid,6);
	my_printf("]\r\n");
	my_printf("cnt_child_nodes:0x%lx\r\n",error_node->cnt_child_nodes);
	my_printf("parent_node:0x%lx\r\n",error_node->parent_node);
	my_printf("first_child_node:0x%lx\r\n",error_node->first_child_node);
	my_printf("next_bro_node:0x%lx\r\n",error_node->next_bro_node);
	my_printf("previous_bro_node:0x%lx\r\n",error_node->previous_bro_node);
	
	my_printf("uplink:0x%bx,ss:0x%bx,snr:0x%bx\r\n",error_node->uplink_plan,error_node->uplink_ss,error_node->uplink_snr);
	my_printf("downlk:0x%bx,ss:0x%bx,snr:0x%bx\r\n",error_node->downlink_plan,error_node->downlink_ss,error_node->downlink_snr);
	my_printf("class:0x%bx\r\n",error_node->broad_class);
	my_printf("state([X|X|X|VER|NEW|ZX|FORBID|DISCONNECT]):0x%bx\r\n",error_node->state);
}


/*******************************************************************************
* Function Name  : check_node_reentrance()
* Description    : ����������ݽṹ�Ƿ���ȷ
					����: ���������ڵ�����������еĽڵ�
						a. ÿ����תǰ���Ŀ���ַ�Ƿ���ȷ?
						b. ��鸸-����, ��-��֮��������ַ�Ƿ���ȷ?
						c. ���ڵ����������Ƿ���ȷTBD
						d. ���������Ƿ񲻳����ܽڵ�����?
						e. ������ɺ����Ľڵ������Ƿ���usage��flag��ʾ��ʹ����һ��?
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
u16 _total_check_node_cnt = 0;

RESULT check_node_reentrance(NODE_HANDLE current_node, NODE_HANDLE parent_node)
{
	RESULT result;
	NODE_HANDLE check_sub_node;

	//��ֹ���ӳɻ�·������ѭ��
	_total_check_node_cnt++;
	if(_total_check_node_cnt>CFG_MGNT_MAX_NODES_CNT)
	{
		my_printf("total node check out of max nodes cnt\r\n");
		return WRONG;
	}

	//�������
	result = CORRECT;
	if(!check_node_valid(current_node))
	{
		result = WRONG;
	}

	if(current_node->parent_node)
	{
		if(!check_node_valid(current_node->parent_node) || (current_node->parent_node != parent_node))
		{
			my_printf("parent_node reference error\r\n");
			result = WRONG;
		}
	}

	if(current_node->first_child_node)
	{
		if(!check_node_valid(current_node->first_child_node) || (current_node != current_node->first_child_node->parent_node))
		{
			my_printf("first_child_node reference error\r\n");
			result = WRONG;
		}
	}

	if(current_node->next_bro_node)
	{
		if(!check_node_valid(current_node->next_bro_node) || (current_node != current_node->next_bro_node->previous_bro_node))
		{
			my_printf("next_bro_node reference error\r\n");
			result = WRONG;
		}
	}

	if(current_node->previous_bro_node)
	{
		if(!check_node_valid(current_node->previous_bro_node) || (current_node != current_node->previous_bro_node->next_bro_node))
		{
			my_printf("previous_bro_node reference error\r\n");
			result = WRONG;
		}
	}

	if(result == WRONG)
	{
		print_error_node(current_node);
		return WRONG;
	}

	// �ݹ��������ӽڵ�
	if(current_node->first_child_node)
	{
		check_sub_node = current_node->first_child_node;
		do
		{
			result = check_node_reentrance(check_sub_node, current_node);
			if(result == WRONG)
			{
				break;
			}
			check_sub_node = check_sub_node->next_bro_node;
		}while(check_sub_node);
	}

	return result;
}

extern NODE_HANDLE network_tree[];

RESULT check_topology(void)
{
	u16 i,j;
	RESULT result;
	u16 total_node_flag_cnt;

	_total_check_node_cnt = 0;
	result = CORRECT;
	
	for(i=0; i<CFG_PHASE_CNT; i++)
	{
		result = check_node_reentrance(network_tree[i],NULL);
		if(result == WRONG)
		{
			break;
		}
	}

	if(result == CORRECT)
	{
		if(_total_check_node_cnt != nodes_usage)
		{
			my_printf("_total_check_node_cnt=%d,nodes_usage=%d\r\n",_total_check_node_cnt,nodes_usage);
			result = WRONG;
		}
	}

	total_node_flag_cnt = 0;
	for(i=0;i<CFG_MGNT_MAX_NODES_CNT/8+1;i++)
	{
		for(j=0;j<8;j++)
		{
			if(nodes_storage_entities_flag[i] & (0x01<<j))
			{
				total_node_flag_cnt++;
			}
		}
	}

	if(total_node_flag_cnt != _total_check_node_cnt)
	{
		my_printf("total_node_flag_cnt:%d, _total_check_node_cnt:%d\r\n",total_node_flag_cnt,_total_check_node_cnt);
		result = WRONG;
	}
	return result;
}


