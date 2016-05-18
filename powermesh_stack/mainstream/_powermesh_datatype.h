/******************** (C) COPYRIGHT 2016 ***************************************
* File Name          : powermesh_datatype.h
* Author             : Lv Haifeng
* Version            : V 1.6.0
* Date               : 02/26/2013
* Description        : powermesh协议中用到的数据类型
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODECTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, BELLING INC. SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAINMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _POWERMESH_DATATYPE_H
#define _POWERMESH_DATATYPE_H


/* Includes ------------------------------------------------------------------*/
#include "_powermesh_config.h"

/* Basic Exported types ------------------------------------------------------*/
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

typedef signed char s8;
typedef signed short s16;
typedef signed long s32;
typedef u8 xdata * ARRAY_HANDLE;
typedef u8 xdata * METER_ID_HANDLE;
//typedef unsigned short BASE_LEN_TYPE;
typedef unsigned char BASE_LEN_TYPE;


typedef enum
{
	FALSE = 0,
	TRUE = !FALSE
}BOOL;

typedef enum								// for apply operation, such as dynamic RAM, timer etc.
{
	FAIL = 0,
	OKAY = !FAIL
}STATUS;

typedef enum								// for check function
{
	WRONG = 0,
	CORRECT = !WRONG,
	BROADCAST,								// BROADCAST and UNKNOWN used in dll uid filter
	SOMEBODY
}RESULT;

typedef struct
{
	u8 year;		// e.g. 2013: 13
	u8 month;		// 1-12
	u8 day;			// 1-31
	u8 hour;		// 0-23
}TIMING_VERSION_STRUCT;

typedef TIMING_VERSION_STRUCT xdata TIMING_VERSION_TYPE;
typedef TIMING_VERSION_STRUCT xdata * TIMING_VERSION_HANDLE;


#ifndef NULL
#define NULL	0
#endif

#define INVALID_RESOURCE_ID		(-1)		//以地址返回的资源, 检查其是否为0, 以ID(s8)返回的资源, 检查其是否为-1;
#define INVALID_PTRP			(-1.0)

#ifdef CPU_STM32
	typedef float TIMING_CALCULATION_TYPE;
#else
	typedef unsigned long TIMING_CALCULATION_TYPE;
#endif

/************ ADDR DB **********************/
#ifdef CPU_STM32
	typedef s16 ADDR_CNT_TYPE;
	typedef s16 ADDR_REF_TYPE;
#else
	typedef s8 ADDR_CNT_TYPE;
	typedef s8 ADDR_REF_TYPE;
#endif
typedef u8 xdata * UID_HANDLE;

/************ TIMER ************************/
typedef s8 TIMER_ID_TYPE;

/************ PHY ************************/
typedef signed char SEND_ID_TYPE;	// if -1, fail;
typedef unsigned char COMM_MODE_TYPE; //[CH3 CH2 CH1 CH0 SCAN 0 RATE1 RATE0]
#define RATE_BPSK	0x00
#define RATE_DS15	0x01
#define RATE_DS63	0x02
#define CHNL_CH0	0x10
#define CHNL_CH1	0x20
#define CHNL_CH2	0x40
#define CHNL_CH3	0x80
#define CHNL_SCAN	0x08
#define CHNL_SALVO	0xF0

typedef struct
{
	u8 phase;
	ARRAY_HANDLE psdu;
	BASE_LEN_TYPE psdu_len;
	u8 xmode;
	u8 prop;				//[0 0 0 0 SCAN SRF ESF ACUPDATE]
	unsigned long delay;
}xdata PHY_SEND_STRUCT;

typedef enum
{
	SEND_STATUS_IDLE = 0,	// after initialization; after send and readed;
	SEND_STATUS_RESERVED,	// after req, make occupied to avoid over-using;
	SEND_STATUS_PENDING,	// when a package is push and not start to send; (when pending, timer count down);
	SEND_STATUS_SENDING,	// when a package is sending;
	SEND_STATUS_OKAY,		// after a package is sent successfully and not read;
	SEND_STATUS_FAIL		// after a package is sent fail and not read;
}SEND_STATUS;

typedef struct
{
	u8  phase;
	
	// the variables related to pure PLC receiving.
	u8	plc_rcv_valid;								//[CH3 CH2 CH1 CH0 0 0 DR1 DR0]
	u8	plc_rcv_buffer[4][CFG_PHY_RCV_BUFFER_SIZE];
	BASE_LEN_TYPE	plc_rcv_len[4];

	// the variables related to assisting.
	u8	plc_rcv_we;
	u8	plc_rcv_parity_err[4];

	// information from rcv registers.
	u16 plc_rcv_pos[4]; 		
	u16 plc_rcv_pon[4];
	u8	plc_rcv_acps[4];		// sync phase
	u8	plc_rcv_agc_word;

	// Belows are the variables related to specific PHY standard
	u8	phy_rcv_valid;			//[CH3 CH2 CH1 CH0 SCAN SRF DR1 DR0]
	u8	phy_rcv_supposed_ch;	//[CH3 CH2 CH1 CH0 0 0 0 0] 	assigned in PHPR
	u8 * phy_rcv_data;
	BASE_LEN_TYPE	phy_rcv_len;
	s8	phy_rcv_ss[4];
	s8	phy_rcv_ebn0[4];

	u8	phy_rcv_info;			//assistant variables
	u8	phy_rcv_protect;
	BASE_LEN_TYPE	phy_rcv_pkg_len[4];
	TIMER_ID_TYPE phy_tid;
}xdata PHY_RCV_STRUCT;

typedef PHY_SEND_STRUCT xdata * PHY_SEND_HANDLE;
typedef PHY_RCV_STRUCT xdata * PHY_RCV_HANDLE;

/************ DLL ************************/
typedef struct
{
	u8			phase;
	ARRAY_HANDLE	target_uid_handle;
	ARRAY_HANDLE	lsdu;
	BASE_LEN_TYPE	lsdu_len;
	u8			prop;			//[DIAG ACK REQ_ACK EDP_PROTOCAL SCAN SRF 0 ACUPDATE]

	u8			xmode;
	u8		  	rmode;
	unsigned long			delay;
	//u8		backlog;
	//u8		max_retry_times;
} xdata DLL_SEND_STRUCT;

typedef struct{
	u8 phase;
	u8 dll_rcv_indication;	//需要主线程处理的接收数据
	u8 dll_rcv_valid;		//[Diag ACK ACK_REQ	 1 SCAN SRF	IDX1 IDX0]
	ARRAY_HANDLE lpdu;
	BASE_LEN_TYPE lpdu_len;
} xdata DLL_RCV_STRUCT;

typedef DLL_SEND_STRUCT xdata * DLL_SEND_HANDLE;
typedef DLL_RCV_STRUCT xdata * DLL_RCV_HANDLE;

/************ EBC *************************/
typedef struct
{
	u8 phase;
	u8 bid;					// broadcast_id
	u8 xmode;
	u8 rmode;
	u8 scan;
	u8 window;

	u8 mask;				// [X X buildid mid uid snr ss acPhase]
	s8 ss;					// ss
	s8 snr;					// snr
	u8 uid[6];				// node UID
	u8 mid[6];				// meter ID
	u8 build_id;			// build network ID
	
	u8 max_identify_try;	// identify 使用
}xdata EBC_BROADCAST_STRUCT;
typedef EBC_BROADCAST_STRUCT xdata * EBC_BROADCAST_HANDLE;

typedef struct
{
	u8 phase;
	u8 bid;
	u8 xmode;
	u8 rmode;
	u8 scan;
	u8 max_identify_try;
	u8 num_bsrf;
}xdata EBC_IDENTIFY_STRUCT;
typedef EBC_IDENTIFY_STRUCT xdata * EBC_IDENTIFY_HANDLE;

typedef struct
{
	u8 phase;
	u8 bid;
	u8 xmode;
	u8 rmode;
	u8 scan;
	u8 node_index;				//节点序号
	u8 node_num;				//节点数量
}xdata EBC_ACQUIRE_STRUCT;
typedef EBC_ACQUIRE_STRUCT xdata * EBC_ACQUIRE_HANDLE;


/************ NETWORK *************************/
typedef unsigned char PROTOCOL_TYPE;
#define PROTOCOL_PSR 0x60
#define PROTOCOL_DST 0x10

typedef enum
{
	DOWNLINK = 0,
	UPLINK = !DOWNLINK
}PSR_DIRECTION;

typedef enum
{
	NO_ERROR = 0x00,			//没有错误
	TARGET_EXECUTE_ERR = 0xF0,	//目标执行错误, e.g. psr_inquire指定取回节点范围越界(严重错误)
	NO_PIPE_INFO = 0xF1,		//节点没有对应的PIPE信息
	NO_XPLAN = 0xF2,			//节点没有相应的发送模式信息
	ACK_TIMEOUT = 0xF3,			//节点向下传输ACK超时, 包括psr_send的错误和pipe中间节点返回的ack错误
	PSR_TIMEOUT = 0xF4,			//需要答复的PSR请求等待答复超时: psr_send成功, 但超过定时没有psr reply返回
	DIAG_TIMEOUT = 0xF5,		//psr_diag的接收节点对指定节点diag超时无返回时返回的错误代码, 与ACK_TIMEOUT有所区别, 此种错误不需要rebuild pipe
	UID_NOT_MATCH = 0xF6,		//接收的UID和PIPE不相符(严重错误)
	MEM_OUT = 0xF7,				//节点无法分配到内存(严重错误)
	TIMER_RUN_OUT = 0xF8,		//节点无法分配到定时器(严重错误)
	SENDING_FAIL = 0xF9,		//发送错误(严重错误)
	UNKNOWN_ERR = 0xFA			//其他未定义的错误(严重错误)
}PSR_ERROR_TYPE;

typedef struct
{
	u8 phase;
	u16 pipe_id;
	u8 prop;					//[SETUP PATROL 0 0 0 ERROR 0 0] //todo://[SETUP PATROL BIWAY 0 DIRECTION ERROR 0 0]
	u8 uplink;					//1:uplink 0:downlink
	u8 index;
	ARRAY_HANDLE nsdu;			// data struct is LPDU , not NPDU, IMPORTANT!
	BASE_LEN_TYPE nsdu_len;
//	PSR_ERROR_TYPE err_code;
} xdata PSR_SEND_STRUCT;

typedef struct
{
	u8 phase;
	u16 pipe_id;
	u8 psr_rcv_indication;		//需要主线程处理的接收数据
	u8 psr_rcv_valid;			//psr_rcv_valid: [valid patrol biway uplink error index1 index0]
	ARRAY_HANDLE npdu;
	BASE_LEN_TYPE npdu_len;
} xdata PSR_RCV_STRUCT;

typedef PSR_SEND_STRUCT xdata * PSR_SEND_HANDLE;
typedef PSR_RCV_STRUCT xdata * PSR_RCV_HANDLE;

/* Topology ------------------------------------------------------------------------*/
#define NODE_STATE_DISCONNECT				0x0001	//节点暂时无法连接
#define NODE_STATE_EXCLUSIVE				0x0002	//节点禁止进一步explore(台区外节点)
#define NODE_STATE_NEW_ARRIVAL				0x0004	//新发现的节点,用取出一次后即撤销标志
#define NODE_STATE_REFOUND					0x0008	//已经存在于拓扑中的完备节点,又一次响应了相同build_id的广播,可能意味着其复位过,或被其他网络加过,
													//标记为REFOUND通知应用层处理, 取出后撤销标志
#define NODE_STATE_RECOMPLETED				0x0010	//与REFOUND意义相同,只不过REFOUND被应用层取出后标记撤销,而RECOMPLETED标志仅在一个新组网周期启动时才撤销,防止节点反复加入,影响效率
#define NODE_STATE_TEMPORARY				0x0020	//发现的新节点,如果PING成功,则被加入拓扑,并保存PING回来的信息
													//该位在set_build_id后撤销
													//或在组网状态达到COMPLETED后撤销(认为是set_build_id成功下行,无成功上行)
#define NODE_STATE_RECONNECT				0x0040	//节点被重新修复
#define NODE_STATE_UPGRADE					0x0080	//节点被优化升级

// 高位是节点软硬件相关的状态
#define NODE_STATE_OLD_VERSION				0x8000	//节点的固件版本是过期的
#define NODE_STATE_ZX_BROKEN				0x4000	//节点过零电路失效
#define NODE_STATE_AMP_BROKEN				0x2000	//节点VHH检测电路失效



typedef struct NODE_STRUCT
{
	u8 uid[6];
	u16 cnt_child_nodes;							//下一级子节点数, 为方便加入, 

	struct NODE_STRUCT * parent_node;				//指向父节点;
	struct NODE_STRUCT * first_child_node;			//指向下一级子节点;
	struct NODE_STRUCT * next_bro_node;				//指向下一个同级节点;
	struct NODE_STRUCT * previous_bro_node;			//指向下一个同级节点, 为方便而加入

	u8 uplink_plan;									//与parent_node通信的上行方案
	u8 uplink_ss;									//与parent_node通信的上行ss
	u8 uplink_snr;									//与parent_node通信的上行ebn0;
	u8 downlink_plan;								//与parent_node通信的下行方案
	u8 downlink_ss;									//与parent_node通信的下行ss
	u8 downlink_snr;								//与parent_node通信的下行enb0;

	u8 broad_class:2;								// 低2位:当前节点的广播级别(broad_class); arm为小端对齐, 先写的为低2位

	u8 repair_fail_cnt:3;							// 修理pipe计数器, 超过一定次数则换路径
													// 修理成功后清零
	u8 set_disconnect_cnt:3;						// 当前节点PIPE被标注为disconnect次数
													// 超过一定次数要换PIPE, 即使修理成功也按失败处理, 即重选pipe
													// 仅新建pipe时清零

	u16 alternative_progress_phase:2;				// 处理相位
	u16 alternative_progress_fail:1;				// 所有相位,所有节点下重建路径失败,该节点可能已经彻底坏掉或被替换,将其标注防止无尽的修复下去
	u16 alternative_progress_cnt:10;				// back_trace重找节点计数
	u16 grade_drift_cnt:3;							// 3bit有符号数
	

	u16 state;										// bit0: NODE_STATE_DISCONNECT	0x01	//节点暂时无法连接
													// bit1: NODE_STATE_EXCLUSIVE	0x02	//节点禁止进一步explore(台区外节点)
													// bit2: NODE_STATE_ZX_BROKEN	0x04	//节点过零电路失效
													// bit3: NODE_STATE_NEW_ARRIVAL	0x08	//新发现的节点,用取出一次后即撤销标志
													// bit4: NODE_STATE_OLD_VERSION	0x10	//节点的固件版本是过期的
													// bit5: NODE_STATE_REFOUND		0x20	//对一次explore过程中被发现已存在于拓扑中的节点(可能有复位问题)进行标记,只重设置一次,防止其不断干扰组网
													// bit6: NODE_STATE_PIPE_FAIL	0x40	//设置节点pipe fail,即只有拓扑中有该节点,pipe库中无对应pipe.
													// bit7: NODE_STATE_AMP_BROKEN	0x80	//节点VHH检测电路失效
	

	u8 user_data_len;								// 2014-09-24 节点组网时向CC传递的用户层信息
	u8 user_data[CFG_USER_DATA_BUFFER_SIZE];
	
}NODE_STRUCT;



typedef NODE_STRUCT * NODE_HANDLE;

typedef enum
{
	ALL = 0				// 遍历拓扑树的条件
	,SOLO				// 独节点
	,NON_SOLO			// 非独节点
}TOPOLOGY_COND_ENUM;

typedef enum
{
	NODE_BROAD_CLASS = 0	// 节点的广播级别
	,NODE_DISCONNECT		// 节点状态
}NODE_PROPERTY_ENUM;



/************ Management & APP *************************/
typedef struct
{
	u8		mgnt_rcv_indication;	//需要主线程处理的接收数据
	u8		mgnt_rcv_valid;
	u8		phase;
	u16		pipe_id;
	ARRAY_HANDLE mpdu;
	BASE_LEN_TYPE		mpdu_len;
}MGNT_RCV_STRUCT;
typedef MGNT_RCV_STRUCT xdata * MGNT_RCV_HANDLE;

typedef struct
{
	PROTOCOL_TYPE 	protocol;		//PROTOCOL_PSR

	/* use in psr */
	u16				pipe_id;
	u8 				target_uid[6];	// in dst, uid is all 0xFF

	/* use in psr */

	/* use in all protocol */
	u8				phase;
	ARRAY_HANDLE 	apdu;
	BASE_LEN_TYPE	apdu_len;
}xdata APP_SEND_STRUCT;				//用户使用的发送数据结构

typedef struct
{
	PROTOCOL_TYPE	protocol;

	/* use in psr */
	u16		pipe_id;

	/* use in dst */
	u8 		src_uid[6];
	COMM_MODE_TYPE		comm_mode;
//#if APP_RCV_SS_SNR == 1			//2015-03-30 固定增加此数据位
	s8		ss;						//信号强度
	s8		snr;					//信噪比
//#endif

	/* use in all protocol */
	u8 		phase;
	ARRAY_HANDLE 	apdu;
	BASE_LEN_TYPE	apdu_len;
}xdata APP_RCV_STRUCT;				//用户使用的接收数据结构

//typedef struct
//{
//	u8		phase;
//	u8 		target_mid[6];
//	ARRAY_HANDLE apdu;
//	u8		apdu_len;
//}xdata APP_UID_SEND_STRUCT;	//用户使用的以uid为索引的发送数据结构

typedef struct
{
	u8			phase;
	u8			search;
	u8			forward;			//1:转发; 2:主动
#if NODE_TYPE==NODE_MASTER
	u8			search_mid;			//valid only in meter_id search
#endif
	ARRAY_HANDLE 	nsdu;
	BASE_LEN_TYPE	nsdu_len;
}DST_SEND_STRUCT;		//DST通信发送数据结构, 其他发送配置在全局数据体中

//typedef struct
//{
//	u8			phase;
//	RATE_TYPE	rate;
//	u8			src_uid[6];
//	ARRAY_HANDLE apdu;
//	u8			apdu_len;
//}DST_RCV_STRUCT;		//DST通信接收数据结构

typedef struct
{
	u8			dst_rcv_indication;
	u8			phase;
	u8			comm_mode;
//	u8 			flooding;
	u8			src_uid[6];
	u8			uplink;
	ARRAY_HANDLE 	apdu;
	BASE_LEN_TYPE	apdu_len;
}xdata DST_STACK_RCV_STRUCT;		//DST通信接收数据结构

typedef struct
{
	COMM_MODE_TYPE comm_mode;
	u8 jumps;
	u8 forward_prop;	// [search_mid config_lock network_ena acps_ena 0 window_index[2:0]]	2014-11-17 高位表示主动发起者

	//	回复,转发使用
	u8 conf;			// 交给app层之后, 保存信息, 用于返回时调用
	u8 stamps;
	BASE_LEN_TYPE ppdu_len;		// 本次数据包长
}DST_CONFIG_STRUCT;		// CC用这个数据体配置发送模式, MT用这个数据体设置回复模式

typedef struct
{
	u8		app_rcv_indication;
	u8		phase;
	u16		pipe_id;
	ARRAY_HANDLE apdu;
	u8			uplink;
	BASE_LEN_TYPE	apdu_len;
}xdata APP_STACK_RCV_STRUCT;//协议栈使用的接收数据结构

typedef APP_SEND_STRUCT xdata * APP_SEND_HANDLE;
typedef APP_RCV_STRUCT xdata * APP_RCV_HANDLE;
typedef APP_STACK_RCV_STRUCT xdata * APP_STACK_RCV_HANDLE;

typedef DST_SEND_STRUCT xdata * DST_SEND_HANDLE;
typedef DST_STACK_RCV_STRUCT xdata * DST_STACK_RCV_HANDLE;

typedef struct				//集中器存储管理的PIPE的数据格式
{
	u8	phase;
	u16 pipe_info;								// pipe_info: [biway 0 0 0 pipe_id(12 bits)]
	u8  pipe_stages;							// pipe级数
	ADDR_REF_TYPE way_uid[CFG_PIPE_STAGE_CNT];	// 每一级节点的uid
	u8	xmode_rmode[CFG_PIPE_STAGE_CNT];		// 每一级的下行,上行通信方式
}PIPE_MNPL_STRUCT;

typedef PIPE_MNPL_STRUCT xdata * PIPE_REF;

typedef struct
{
	u16 pipe_id;
	u8 uid[6];
	PSR_ERROR_TYPE err_code;
}xdata ERROR_STRUCT;
typedef ERROR_STRUCT xdata * ERR_HANDLE;
#define TEST_PIPE_ID	0x0FFF					// 测试用pipe_id, 不能被查询


/************ Network Management *************************/
typedef struct
{
	u8 uid[6];
	u8 uplink_mode;
	u8 uplink_ss;
	u8 uplink_snr;
	u8 downlink_mode;
	u8 downlink_ss;
	u8 downlink_snr;
	float ptrp;					// PTRP = PATH THROUPUT
}LINK_STRUCT;
typedef LINK_STRUCT * LINK_HANDLE;

typedef enum
{
	BUILD_RESTART = 0,			// 删除所有拓扑节点, 用以重新启动搜索
	BUILD_RENEW = 1,			// 保留当前的搜索拓扑, 只是将所有节点的搜索级别设置为0, 即未执行搜索状态, 用于在网络中新加入节点后重新启动搜索
	BUILD_CONTINUE
}BUILD_CONTROL;

typedef enum
{
	TO_BE_CONTINUE = 0,
	COMPLETED,
	ZERO_SEARCHED				// 2014-04-21 为以前结束增加一个状态
}BUILD_RESULT;

typedef struct
{
	u8 max_broad_class; 	// 建网使用的最大速率级别 0: bpsk; 1: ds15
	u8 max_build_depth; 	// 建网最大深度 1: 主节点带一级节点; 0: 不限制
	u8 default_broad_windows;
	u8 current_broad_class;	// 当前使用的速率级别
}BUILD_CONFIG_STRUCT;


/************ Network Optimization *************************/
typedef struct
{
	u8 num_nodes;
	u8 uid[CFG_PIPE_STAGE_CNT][6];
}PATH_STRUCT;

typedef enum
{
	AUTOHEAL_LEVEL0 = 0,	//LEVEL0: 无重试, 无自修复
	AUTOHEAL_LEVEL1,		//LEVEL1: 重试, 当明确收到NO_PIPE_INFO时重刷PIPE
	AUTOHEAL_LEVEL2,		//LEVEL2: 重试 + 重刷PIPE + 修复路径链路
	AUTOHEAL_LEVEL3,		//LEVEL3: 重试 + 重刷PIPE + 修复路径链路 + 重建路径
}AUTOHEAL_LEVEL;

	/****** 路径重建的选项参数 ******/
#define REBUILD_PATH_CONFIG_TRY_OTHER_PHASE	0x80	//当原相位搜索失败时, 试探其他相位
#define REBUILD_PATH_CONFIG_LONGER_JUMPS	0x40	//用更长的路径搜索
#define REBUILD_PATH_CONFIG_SLOWER_RATE		0x20	//用更慢的速度搜索
#define REBUILD_PATH_CONFIG_SYNC_TOPOLOGY 	0x10	//将PIPE改变的结果同步到拓扑库中


#endif

