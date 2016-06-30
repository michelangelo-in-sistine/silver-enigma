/******************** (C) COPYRIGHT 2016 ***************************************
* File Name          : powermesh_config.h
* Author             : Lv Haifeng
* Version            : V 1.6.0
* Date               : 02/26/2013
* Description        : 协议栈占用资源参数化配置
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODECTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, BELLING INC. SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAINMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _POWERMESH_CONFIG_H
#define _POWERMESH_CONFIG_H


/* Includes ------------------------------------------------------------------*/
#include "compile_define.h"

/* Timing Config */
#ifdef CPU_STM32
	#define T_SCALE 1.0944				//freq base of arm is 1KHz, freq base of 6810 is 913.7427Hz
#else
	#define T_SCALE 1
#endif

#define BPSK_BIT_TIMING		 			0.1824
#define XMT_SCAN_INTERVAL_STICKS_BASE 	10			//scan类型数据包在发送时的包与包之间时间间隔, 单位: 定时器time stick,以下同
#define ACK_DELAY_STICKS_BASE			25			//收到REQ_ACK的数据包后延时发送响应
	
#define SOFTWARE_DELAY_STICKS_BASE		5  			//有响应数据包从接收完毕到响应时间间隔, 与架构及处理器处理速度相关
#define HARDWARE_DELAY_STICKS_BASE 		5  			//底层接收滤波延时, 发送滤波延时, 与处理速度无关;

#define RCV_SCAN_MARGIN_STICKS_BASE		20			//scan类型数据包接收结尾margin
#define RCV_SINGLE_MARGIN_STICKS_BASE 	2			//非scan类型包不同通道接收时间差margin
#define EBC_WINDOW_MARGIN_STICKS_BASE	10 			//EBC协议中, 每个时间窗口留的margin
#define DLL_SEND_DELAY_BASE				25			//SCAN类型数据包后不能紧跟一个普通类型数据包, 接收方如果收不到SCAN最后一个频率数据包, 会在RCV_SCAN_MARGIN_STICKS_BASE
													//之后对接收机复位, 因此设置一个连续发送的基本延时, 以后所有
#define RCV_RESP_MARGIN_STICKS_BASE		100			//有响应发起方的接收等待margin
													
#define PSR_STAGE_DELAY_STICKS_BASE		DLL_SEND_DELAY_BASE + 15
#define PSR_MARGIN_STICKS_BASE			1000  		//PSR 管道建立源端等待窗口margin

#define XMT_SCAN_INTERVAL_STICKS 		(XMT_SCAN_INTERVAL_STICKS_BASE 	* T_SCALE)
#define ACK_DELAY_STICKS				(ACK_DELAY_STICKS_BASE			* T_SCALE)
#define DLL_SEND_DELAY_STICKS			(DLL_SEND_DELAY_BASE			* T_SCALE)
#define SOFTWARE_DELAY_STICKS			(SOFTWARE_DELAY_STICKS_BASE  	* T_SCALE)
#define HARDWARE_DELAY_STICKS 			(HARDWARE_DELAY_STICKS_BASE  	* T_SCALE)

#define RCV_SCAN_MARGIN_STICKS			(RCV_SCAN_MARGIN_STICKS_BASE	* T_SCALE)
#define RCV_SINGLE_MARGIN_STICKS 		(RCV_SINGLE_MARGIN_STICKS_BASE	* T_SCALE)
#define RCV_RESP_MARGIN_STICKS			(RCV_RESP_MARGIN_STICKS_BASE 	* T_SCALE)
#define EBC_WINDOW_MARGIN_STICKS		(EBC_WINDOW_MARGIN_STICKS_BASE 	* T_SCALE)

#define PSR_STAGE_DELAY_STICKS			(PSR_STAGE_DELAY_STICKS_BASE	* T_SCALE)		//PSR 每级处理增加的延时, 目的: 使等待超出的时间与级数近似无关
#define PSR_MARGIN_STICKS				(PSR_MARGIN_STICKS_BASE 		* T_SCALE)

#define APP_MC_MARGIN_STICKS			20
#define APP_MC_INITIAL_STICKS			20			//every 




/* Phase Config */
#if DEVICE_TYPE==DEVICE_CC
	#define CFG_PHASE_CNT							3
#else
	#define CFG_PHASE_CNT							1
#endif

/* Mem Config */
#define CFG_MEM_MINOR_BLOCK_SIZE					32				// MINOR型动态内存每次分配字节数
#if CPU_TYPE==CPU_STM32F030C8
#define CFG_MEM_SUPERIOR_BLOCK_SIZE					100				// M0 CPU内存访问必须是aligned, 
#else
#define CFG_MEM_SUPERIOR_BLOCK_SIZE					255				// SUPERIOR型动态内存每次分配字节数
#endif
#define CFG_USER_DATA_BUFFER_SIZE					20				// PING携带的用户数据长度

#if DEVICE_TYPE==DEVICE_CC
	#define CFG_MEM_MINOR_BLOCKS_CNT				16				// MINOR类型内存最大分配块数
	#define CFG_MEM_SUPERIOR_BLOCKS_CNT				6				// SUPERIOR类型内存最大分配块数
#elif DEVICE_TYPE==DEVICE_MT || DEVICE_TYPE==DEVICE_SS || DEVICE_TYPE==DEVICE_SE
	#define CFG_MEM_MINOR_BLOCKS_CNT				4
	#define CFG_MEM_SUPERIOR_BLOCKS_CNT				3				// DST考虑转发加回复, 最大需要三块SUPERIOR内存
#elif DEVICE_TYPE==DEVICE_DC || DEVICE_TYPE==DEVICE_CV
	#define CFG_MEM_MINOR_BLOCKS_CNT				4
	#define CFG_MEM_SUPERIOR_BLOCKS_CNT				3
#endif

/* Rscodec Config */
#define CFG_RS_DECODE_LENGTH 						CFG_MEM_SUPERIOR_BLOCK_SIZE
#define CFG_RS_ENCODE_LENGTH 						((CFG_MEM_SUPERIOR_BLOCK_SIZE%20>10)\
														?(CFG_MEM_SUPERIOR_BLOCK_SIZE/20*10 + CFG_MEM_SUPERIOR_BLOCK_SIZE%20 - 10)\
														:(CFG_MEM_SUPERIOR_BLOCK_SIZE/20*10))

/* ADDR Database Config */
#if DEVICE_TYPE==DEVICE_CC
	#define CFG_ADDR_ITEM_CNT						2047			// 数据类型支持最大32767个
	//#define CFG_ADDR_ITEM_CNT						16
#elif DEVICE_TYPE==DEVICE_MT
	#define CFG_ADDR_ITEM_CNT						16				// 最大127个, (2^N-1用于描述非法资源索引)
#elif DEVICE_TYPE==DEVICE_DC	
	#define CFG_ADDR_ITEM_CNT						255				// 
#elif DEVICE_TYPE==DEVICE_CV
	#define CFG_ADDR_ITEM_CNT						16				
#elif DEVICE_TYPE==DEVICE_SE
	#define CFG_ADDR_ITEM_CNT						16				
#elif DEVICE_TYPE==DEVICE_SS
	#define CFG_ADDR_ITEM_CNT						16				
#endif
#define CST_MAX_EXPIRE_CNT							255				// 地址信息管理的超时机制最大计数

/* Timer Config */
#if DEVICE_TYPE==DEVICE_CC
	#define CFG_TIMER_CNT							12				// 定时器最大个数
#elif DEVICE_TYPE==DEVICE_MT	
	#define CFG_TIMER_CNT							4
#elif DEVICE_TYPE==DEVICE_DC	
	#define CFG_TIMER_CNT							4
#elif DEVICE_TYPE==DEVICE_CV	
	#define CFG_TIMER_CNT							8
#elif DEVICE_TYPE==DEVICE_SE	
	#define CFG_TIMER_CNT							4
#elif DEVICE_TYPE==DEVICE_SS	
	#define CFG_TIMER_CNT							4
	

#endif

/* Sending Queue Config */
#if DEVICE_TYPE==DEVICE_CC
	#define CFG_QUEUE_CNT							CFG_MEM_SUPERIOR_BLOCKS_CNT
#elif DEVICE_TYPE==DEVICE_MT
	#define CFG_QUEUE_CNT							2
#elif DEVICE_TYPE==DEVICE_DC						
	#define CFG_QUEUE_CNT							CFG_MEM_SUPERIOR_BLOCKS_CNT
#elif DEVICE_TYPE==DEVICE_CV
	#define CFG_QUEUE_CNT							CFG_MEM_SUPERIOR_BLOCKS_CNT
#elif DEVICE_TYPE==DEVICE_SE
	#define CFG_QUEUE_CNT							CFG_MEM_SUPERIOR_BLOCKS_CNT
#elif DEVICE_TYPE==DEVICE_SS
	#define CFG_QUEUE_CNT							2
#endif
#define MAX_QUEUE_DELAY								80000UL			// 按16个ds63+scan计算得到, pm_calc_bsrf_delay_timing(2,1,16,'stm')

/* PHY Config */
#define CFG_PHY_RCV_BUFFER_SIZE						CFG_MEM_SUPERIOR_BLOCK_SIZE
#define CFG_CC_ACPS_DELAY							94				// 为使CC发送的相位信息差与模块间相位差一致, CC发送需延时, 靠读相位信息实现
#define CFG_XMT_AMP_DEFAULT_VALUE					0x80
#define CFG_XMT_AMP_MIN_VALUE						0x08

/* MAC Config */
#define CFG_DEFAULT_MAC_STICK_BASE							10				// bpsk (pilot+sync_word)=6.4ms, 但处理需要时间, 8ms比较勉强
#define CFG_DEFAULT_MAC_STICK_ANNEAL						200				// 信道检测落后于发送控制, 因此发送后增加一个退火状态, 避免一个节点连续发送持续占用信道
#define CFG_MAX_NEARBY_NODES						16				//附近主节点数目
#define CFG_MAX_NAV_TIMING							0xfff0			//最大NAV时间, 65520ms, 即一分钟
#define CFG_MAX_NAV_VALUE							0x0fff			//最大NAV值, 最小单位16ms
#define CFG_MAC_MONOPOLY_COVER_TIM					5000			//对于连续的过程前调用, 如组网,使得整个过程不容易被打断;

/* EBC Config */
#if DEVICE_TYPE==DEVICE_CC
	#define EBC_NEIGHBOR_CNT						64				// EBC Caller 支持一次寻找最大临近节点数
#elif DEVICE_TYPE==DEVICE_MT
	#define EBC_NEIGHBOR_CNT						CFG_ADDR_ITEM_CNT
#elif DEVICE_TYPE==DEVICE_DC
	#define EBC_NEIGHBOR_CNT						64
#elif DEVICE_TYPE==DEVICE_CV
	#define EBC_NEIGHBOR_CNT						CFG_ADDR_ITEM_CNT
#elif DEVICE_TYPE==DEVICE_SE
	#define EBC_NEIGHBOR_CNT						CFG_ADDR_ITEM_CNT
#elif DEVICE_TYPE==DEVICE_SS
	#define EBC_NEIGHBOR_CNT						CFG_ADDR_ITEM_CNT
#endif
//#define CFG_EBC_INDENTIFY_TRY						3
#define CFG_EBC_MAX_BPSK_WINDOW						7				// BPSK最大窗口128
#define CFG_EBC_MAX_DS15_WINDOW						4				// DS15最大窗口16
#define CFG_EBC_MAX_DS63_WINDOW						3				// DS63最大窗口8
#define CFG_EBC_ACQUIRE_MAX_NODES					16				// 一次最大获得节点号

/* Psr Config */
#define CFG_PIPE_STAGE_CNT							8				// PIPE最大支持级数
#if DEVICE_TYPE==DEVICE_CC
	#define CFG_PSR_PIPE_RECORD_CNT					2047						// 节点支持最大PIPE记录
	//#define CFG_PSR_PIPE_RECORD_CNT					16
	#define CFG_PSR_PIPE_MNPL_CNT					CFG_PSR_PIPE_RECORD_CNT		// 最大管理PIPE记录
#elif DEVICE_TYPE==DEVICE_MT 
	#define CFG_PSR_PIPE_RECORD_CNT					16
#elif DEVICE_TYPE==DEVICE_DC
	#define CFG_PSR_PIPE_RECORD_CNT					CFG_ADDR_ITEM_CNT
#elif DEVICE_TYPE==DEVICE_CV
	#define CFG_PSR_PIPE_RECORD_CNT 				16
	#define CFG_PSR_PIPE_MNPL_CNT					CFG_PSR_PIPE_RECORD_CNT		// 最大管理PIPE记录
#elif DEVICE_TYPE==DEVICE_SE
	#define CFG_PSR_PIPE_RECORD_CNT 				16
#elif DEVICE_TYPE==DEVICE_SS
	#define CFG_PSR_PIPE_RECORD_CNT 				16
#endif

/* Dst Config */
#define CFG_DST_MAX_JUMPS							4						// 洪泛最大四级
#define CFG_DST_MAX_WINDOW_INDEX					5						// 洪泛最大窗口2^5 32

/* App Config */
#ifdef USE_RSCODEC
#if CPU_TYPE==CPU_STM32F030C8
	#define CFG_APDU_MAX_LENGTH						25						//(50-18-6-1)
#else
	#define CFG_APDU_MAX_LENGTH						100						
#endif
#else
	#define CFG_APDU_MAX_LENGTH						200						//林洋使用长包
#endif

/* Network Management Config, Only CC */
#if DEVICE_TYPE==DEVICE_CC
	#define CFG_MGNT_MAX_NODES_CNT					2047					// CC管理全网最大管理节点数
	//#define CFG_MGNT_MAX_NODES_CNT					16
#elif DEVICE_TYPE==DEVICE_CV
	#define CFG_MGNT_MAX_NODES_CNT					16						// CV管理全网最大管理节点数, 调试阶段先少一点
#endif

#if NODE_TYPE==NODE_MASTER
	#define CFG_MGNT_MAX_PIPES_CNT					CFG_MGNT_MAX_NODES_CNT	// CC管理全网最大PIPE数目
	#define CFG_TOPOLOGY_MAX_NODE_CHILDREN			65535

#if CPU_TYPE==CPU_STM32F103ZE
	#define CFG_MGNT_MAX_NEW_LINKS_BUFFER_CNT		64
#else
	#define CFG_MGNT_MAX_NEW_LINKS_BUFFER_CNT		16
#endif
	
	#define CFG_MAX_BUILD_CLASS						01						// build network中广播用到的最低通信速率 0:bpsk; 1:ds15
	#define CFG_MAX_BUILD_WINDOW					06
	#define CFG_MAX_LINK_CLASS						1						// 优化链路方案用到的最低通信速率, 可能存在build network时使用较高的速率广播得到了子节点, 但优化时选择了更低速率的链路方案的情况
	#define CFG_MAX_DIAG_TRY						1
	#define CFG_MAX_SETUP_PIPE_TRY 					2
	#define CFG_MAX_PSR_TRANSACTION_TRY				2
	#define CFG_MAX_DST_TRANSACTION_TRY				1
	#define CFG_MAX_DISCONNECT_REPAIR				3						// 对一条经常被标注为disconnect的pipe, 超过指定次数则将其降速或搜寻新的路径
	#define CFG_MAX_OPTIMAL_PROC_REPAIR				2						// 一条pipe,最多容忍修复失败次数,超过次数则重建pipe

	#define PTRP_BPSK 								5482.5
	#define PTRP_DS15 								365.4971
	#define PTRP_DS63 								87.0231
	#define CFG_BPSK_PTRP_GATE						(PTRP_BPSK/2-100)
	#define CFG_DS15_PTRP_GATE						(PTRP_DS63/2)
	#define CFG_BPSK_SNR_GATE						10
	#define CFG_DS15_SNR_GATE						10
#endif

#endif

