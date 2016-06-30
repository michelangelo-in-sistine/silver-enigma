/******************** (C) COPYRIGHT 2016 ***************************************
* File Name          : powermesh_config.h
* Author             : Lv Haifeng
* Version            : V 1.6.0
* Date               : 02/26/2013
* Description        : Э��ջռ����Դ����������
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
#define XMT_SCAN_INTERVAL_STICKS_BASE 	10			//scan�������ݰ��ڷ���ʱ�İ����֮��ʱ����, ��λ: ��ʱ��time stick,����ͬ
#define ACK_DELAY_STICKS_BASE			25			//�յ�REQ_ACK�����ݰ�����ʱ������Ӧ
	
#define SOFTWARE_DELAY_STICKS_BASE		5  			//����Ӧ���ݰ��ӽ�����ϵ���Ӧʱ����, ��ܹ��������������ٶ����
#define HARDWARE_DELAY_STICKS_BASE 		5  			//�ײ�����˲���ʱ, �����˲���ʱ, �봦���ٶ��޹�;

#define RCV_SCAN_MARGIN_STICKS_BASE		20			//scan�������ݰ����ս�βmargin
#define RCV_SINGLE_MARGIN_STICKS_BASE 	2			//��scan���Ͱ���ͬͨ������ʱ���margin
#define EBC_WINDOW_MARGIN_STICKS_BASE	10 			//EBCЭ����, ÿ��ʱ�䴰������margin
#define DLL_SEND_DELAY_BASE				25			//SCAN�������ݰ����ܽ���һ����ͨ�������ݰ�, ���շ�����ղ���SCAN���һ��Ƶ�����ݰ�, ����RCV_SCAN_MARGIN_STICKS_BASE
													//֮��Խ��ջ���λ, �������һ���������͵Ļ�����ʱ, �Ժ�����
#define RCV_RESP_MARGIN_STICKS_BASE		100			//����Ӧ���𷽵Ľ��յȴ�margin
													
#define PSR_STAGE_DELAY_STICKS_BASE		DLL_SEND_DELAY_BASE + 15
#define PSR_MARGIN_STICKS_BASE			1000  		//PSR �ܵ�����Դ�˵ȴ�����margin

#define XMT_SCAN_INTERVAL_STICKS 		(XMT_SCAN_INTERVAL_STICKS_BASE 	* T_SCALE)
#define ACK_DELAY_STICKS				(ACK_DELAY_STICKS_BASE			* T_SCALE)
#define DLL_SEND_DELAY_STICKS			(DLL_SEND_DELAY_BASE			* T_SCALE)
#define SOFTWARE_DELAY_STICKS			(SOFTWARE_DELAY_STICKS_BASE  	* T_SCALE)
#define HARDWARE_DELAY_STICKS 			(HARDWARE_DELAY_STICKS_BASE  	* T_SCALE)

#define RCV_SCAN_MARGIN_STICKS			(RCV_SCAN_MARGIN_STICKS_BASE	* T_SCALE)
#define RCV_SINGLE_MARGIN_STICKS 		(RCV_SINGLE_MARGIN_STICKS_BASE	* T_SCALE)
#define RCV_RESP_MARGIN_STICKS			(RCV_RESP_MARGIN_STICKS_BASE 	* T_SCALE)
#define EBC_WINDOW_MARGIN_STICKS		(EBC_WINDOW_MARGIN_STICKS_BASE 	* T_SCALE)

#define PSR_STAGE_DELAY_STICKS			(PSR_STAGE_DELAY_STICKS_BASE	* T_SCALE)		//PSR ÿ���������ӵ���ʱ, Ŀ��: ʹ�ȴ�������ʱ���뼶�������޹�
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
#define CFG_MEM_MINOR_BLOCK_SIZE					32				// MINOR�Ͷ�̬�ڴ�ÿ�η����ֽ���
#if CPU_TYPE==CPU_STM32F030C8
#define CFG_MEM_SUPERIOR_BLOCK_SIZE					100				// M0 CPU�ڴ���ʱ�����aligned, 
#else
#define CFG_MEM_SUPERIOR_BLOCK_SIZE					255				// SUPERIOR�Ͷ�̬�ڴ�ÿ�η����ֽ���
#endif
#define CFG_USER_DATA_BUFFER_SIZE					20				// PINGЯ�����û����ݳ���

#if DEVICE_TYPE==DEVICE_CC
	#define CFG_MEM_MINOR_BLOCKS_CNT				16				// MINOR�����ڴ����������
	#define CFG_MEM_SUPERIOR_BLOCKS_CNT				6				// SUPERIOR�����ڴ����������
#elif DEVICE_TYPE==DEVICE_MT || DEVICE_TYPE==DEVICE_SS || DEVICE_TYPE==DEVICE_SE
	#define CFG_MEM_MINOR_BLOCKS_CNT				4
	#define CFG_MEM_SUPERIOR_BLOCKS_CNT				3				// DST����ת���ӻظ�, �����Ҫ����SUPERIOR�ڴ�
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
	#define CFG_ADDR_ITEM_CNT						2047			// ��������֧�����32767��
	//#define CFG_ADDR_ITEM_CNT						16
#elif DEVICE_TYPE==DEVICE_MT
	#define CFG_ADDR_ITEM_CNT						16				// ���127��, (2^N-1���������Ƿ���Դ����)
#elif DEVICE_TYPE==DEVICE_DC	
	#define CFG_ADDR_ITEM_CNT						255				// 
#elif DEVICE_TYPE==DEVICE_CV
	#define CFG_ADDR_ITEM_CNT						16				
#elif DEVICE_TYPE==DEVICE_SE
	#define CFG_ADDR_ITEM_CNT						16				
#elif DEVICE_TYPE==DEVICE_SS
	#define CFG_ADDR_ITEM_CNT						16				
#endif
#define CST_MAX_EXPIRE_CNT							255				// ��ַ��Ϣ����ĳ�ʱ����������

/* Timer Config */
#if DEVICE_TYPE==DEVICE_CC
	#define CFG_TIMER_CNT							12				// ��ʱ��������
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
#define MAX_QUEUE_DELAY								80000UL			// ��16��ds63+scan����õ�, pm_calc_bsrf_delay_timing(2,1,16,'stm')

/* PHY Config */
#define CFG_PHY_RCV_BUFFER_SIZE						CFG_MEM_SUPERIOR_BLOCK_SIZE
#define CFG_CC_ACPS_DELAY							94				// ΪʹCC���͵���λ��Ϣ����ģ�����λ��һ��, CC��������ʱ, ������λ��Ϣʵ��
#define CFG_XMT_AMP_DEFAULT_VALUE					0x80
#define CFG_XMT_AMP_MIN_VALUE						0x08

/* MAC Config */
#define CFG_DEFAULT_MAC_STICK_BASE							10				// bpsk (pilot+sync_word)=6.4ms, ��������Ҫʱ��, 8ms�Ƚ���ǿ
#define CFG_DEFAULT_MAC_STICK_ANNEAL						200				// �ŵ��������ڷ��Ϳ���, ��˷��ͺ�����һ���˻�״̬, ����һ���ڵ��������ͳ���ռ���ŵ�
#define CFG_MAX_NEARBY_NODES						16				//�������ڵ���Ŀ
#define CFG_MAX_NAV_TIMING							0xfff0			//���NAVʱ��, 65520ms, ��һ����
#define CFG_MAX_NAV_VALUE							0x0fff			//���NAVֵ, ��С��λ16ms
#define CFG_MAC_MONOPOLY_COVER_TIM					5000			//���������Ĺ���ǰ����, ������,ʹ���������̲����ױ����;

/* EBC Config */
#if DEVICE_TYPE==DEVICE_CC
	#define EBC_NEIGHBOR_CNT						64				// EBC Caller ֧��һ��Ѱ������ٽ��ڵ���
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
#define CFG_EBC_MAX_BPSK_WINDOW						7				// BPSK��󴰿�128
#define CFG_EBC_MAX_DS15_WINDOW						4				// DS15��󴰿�16
#define CFG_EBC_MAX_DS63_WINDOW						3				// DS63��󴰿�8
#define CFG_EBC_ACQUIRE_MAX_NODES					16				// һ������ýڵ��

/* Psr Config */
#define CFG_PIPE_STAGE_CNT							8				// PIPE���֧�ּ���
#if DEVICE_TYPE==DEVICE_CC
	#define CFG_PSR_PIPE_RECORD_CNT					2047						// �ڵ�֧�����PIPE��¼
	//#define CFG_PSR_PIPE_RECORD_CNT					16
	#define CFG_PSR_PIPE_MNPL_CNT					CFG_PSR_PIPE_RECORD_CNT		// ������PIPE��¼
#elif DEVICE_TYPE==DEVICE_MT 
	#define CFG_PSR_PIPE_RECORD_CNT					16
#elif DEVICE_TYPE==DEVICE_DC
	#define CFG_PSR_PIPE_RECORD_CNT					CFG_ADDR_ITEM_CNT
#elif DEVICE_TYPE==DEVICE_CV
	#define CFG_PSR_PIPE_RECORD_CNT 				16
	#define CFG_PSR_PIPE_MNPL_CNT					CFG_PSR_PIPE_RECORD_CNT		// ������PIPE��¼
#elif DEVICE_TYPE==DEVICE_SE
	#define CFG_PSR_PIPE_RECORD_CNT 				16
#elif DEVICE_TYPE==DEVICE_SS
	#define CFG_PSR_PIPE_RECORD_CNT 				16
#endif

/* Dst Config */
#define CFG_DST_MAX_JUMPS							4						// �鷺����ļ�
#define CFG_DST_MAX_WINDOW_INDEX					5						// �鷺��󴰿�2^5 32

/* App Config */
#ifdef USE_RSCODEC
#if CPU_TYPE==CPU_STM32F030C8
	#define CFG_APDU_MAX_LENGTH						25						//(50-18-6-1)
#else
	#define CFG_APDU_MAX_LENGTH						100						
#endif
#else
	#define CFG_APDU_MAX_LENGTH						200						//����ʹ�ó���
#endif

/* Network Management Config, Only CC */
#if DEVICE_TYPE==DEVICE_CC
	#define CFG_MGNT_MAX_NODES_CNT					2047					// CC����ȫ��������ڵ���
	//#define CFG_MGNT_MAX_NODES_CNT					16
#elif DEVICE_TYPE==DEVICE_CV
	#define CFG_MGNT_MAX_NODES_CNT					16						// CV����ȫ��������ڵ���, ���Խ׶�����һ��
#endif

#if NODE_TYPE==NODE_MASTER
	#define CFG_MGNT_MAX_PIPES_CNT					CFG_MGNT_MAX_NODES_CNT	// CC����ȫ�����PIPE��Ŀ
	#define CFG_TOPOLOGY_MAX_NODE_CHILDREN			65535

#if CPU_TYPE==CPU_STM32F103ZE
	#define CFG_MGNT_MAX_NEW_LINKS_BUFFER_CNT		64
#else
	#define CFG_MGNT_MAX_NEW_LINKS_BUFFER_CNT		16
#endif
	
	#define CFG_MAX_BUILD_CLASS						01						// build network�й㲥�õ������ͨ������ 0:bpsk; 1:ds15
	#define CFG_MAX_BUILD_WINDOW					06
	#define CFG_MAX_LINK_CLASS						1						// �Ż���·�����õ������ͨ������, ���ܴ���build networkʱʹ�ýϸߵ����ʹ㲥�õ����ӽڵ�, ���Ż�ʱѡ���˸������ʵ���·���������
	#define CFG_MAX_DIAG_TRY						1
	#define CFG_MAX_SETUP_PIPE_TRY 					2
	#define CFG_MAX_PSR_TRANSACTION_TRY				2
	#define CFG_MAX_DST_TRANSACTION_TRY				1
	#define CFG_MAX_DISCONNECT_REPAIR				3						// ��һ����������עΪdisconnect��pipe, ����ָ���������併�ٻ���Ѱ�µ�·��
	#define CFG_MAX_OPTIMAL_PROC_REPAIR				2						// һ��pipe,��������޸�ʧ�ܴ���,�����������ؽ�pipe

	#define PTRP_BPSK 								5482.5
	#define PTRP_DS15 								365.4971
	#define PTRP_DS63 								87.0231
	#define CFG_BPSK_PTRP_GATE						(PTRP_BPSK/2-100)
	#define CFG_DS15_PTRP_GATE						(PTRP_DS63/2)
	#define CFG_BPSK_SNR_GATE						10
	#define CFG_DS15_SNR_GATE						10
#endif

#endif

