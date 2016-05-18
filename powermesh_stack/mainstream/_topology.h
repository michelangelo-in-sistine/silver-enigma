#ifndef _TOPOLOGY_H
#define _TOPOLOGY_H

/* Compile Control ------------------------------------------------------------------*/

/* Constant Define ------------------------------------------------------------------*/
#define MAX_NODES 					GBL_MAX_NODES
#define MAX_NODE_CHILDREN			GBL_MAX_CHILDREN_PER_NODE
#define MAX_PIPE_DEPTH				15
#define MAX_REENTRANT_DEPTH			CFG_PIPE_STAGE_CNT * 2

///* Data Type ------------------------------------------------------------------------*/
//typedef enum
//{
//	OFF_LINE = 0,
//	ON_LINE
//}NODE_STATE;

//typedef struct NODE_STRUCT
//{
//	u8 uid[6];
//	u16 cnt_child_nodes;					//下一级子节点数, 为方便加入, 

//	struct NODE_STRUCT * parent_node;				//指向父节点;
//	struct NODE_STRUCT * first_child_node;			//指向下一级子节点;
//	struct NODE_STRUCT * next_bro_node;				//指向下一个同级节点;
//	struct NODE_STRUCT * previous_bro_node;			//指向下一个同级节点, 为方便而加入

//	u8 uplink_plan;							//与parent_node通信的上行方案
//	u8 uplink_ss;							//与parent_node通信的上行ss
//	u8 uplink_snr;							//与parent_node通信的上行ebn0;
//	u8 downlink_plan;						//与parent_node通信的下行方案
//	u8 downlink_ss;							//与parent_node通信的下行ss
//	u8 downlink_snr;						//与parent_node通信的下行enb0;

//	u8 broad_class;							// 当前节点的广播级别
//	NODE_STATE state;						// 当前节点状态
//}NODE_STRUCT;



//typedef NODE_STRUCT * NODE_HANDLE;

//typedef enum
//{
//	ALL = 0				// 遍历拓扑树的条件
//	,SOLO				// 独节点
//	,NON_SOLO			// 非独节点
//}TOPOLOGY_COND_ENUM;


/* Exported functions ------------------------------------------------------- */
void init_topology(void);
u16 get_node_usage(void);
BOOL check_node_valid(NODE_STRUCT * node);

NODE_STRUCT * creat_path_tree_root(void);
NODE_STRUCT * add_node(NODE_STRUCT * parent_node);
STATUS delete_node(NODE_STRUCT * node_to_delete);
STATUS delete_branch(NODE_STRUCT * node_to_delete);
BOOL is_child_node(NODE_HANDLE target_node, NODE_HANDLE parent_node);
BOOL is_node_in_branch(NODE_STRUCT * node_to_search, NODE_STRUCT * branch_root_node);
u8 get_node_depth(NODE_HANDLE target_node);

NODE_STRUCT * search_uid_in_branch(NODE_STRUCT * branch_root_node, u8 * uid);
u16 get_list_from_branch(NODE_STRUCT * branch_root_node, TOPOLOGY_COND_ENUM topology_cond, u16 state_filt_in_cond, u16 state_filt_out_cond, NODE_HANDLE * list_buffer_to_write, u16 list_buffer_usage, u16 list_buffer_depth);

void randomize_list(NODE_HANDLE * list, u8 list_len);
STATUS move_node(NODE_HANDLE node_to_move, NODE_HANDLE new_parent);
STATUS move_branch(NODE_STRUCT * branch_node_to_move, NODE_STRUCT * new_parent_node);
STATUS set_node_uid(NODE_STRUCT * node, u8 * target_uid);

STATUS set_branch_property(NODE_HANDLE branch_root, NODE_PROPERTY_ENUM property, u8 set_value);
STATUS clr_branch_state(NODE_HANDLE branch_root_node, u8 clr_value);

u8 get_path_script_from_topology(NODE_STRUCT * node, u8 * script_buffer);

void print_branch(NODE_STRUCT * node);
void print_list(NODE_STRUCT ** node_list, u8 list_len);

NODE_HANDLE back_trace(NODE_HANDLE back_parent_node, NODE_HANDLE current_node, int back_steps);
RESULT check_topology(void);
#endif
