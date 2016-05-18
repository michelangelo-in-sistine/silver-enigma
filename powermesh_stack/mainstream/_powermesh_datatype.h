/******************** (C) COPYRIGHT 2016 ***************************************
* File Name          : powermesh_datatype.h
* Author             : Lv Haifeng
* Version            : V 1.6.0
* Date               : 02/26/2013
* Description        : powermeshЭ�����õ�����������
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

#define INVALID_RESOURCE_ID		(-1)		//�Ե�ַ���ص���Դ, ������Ƿ�Ϊ0, ��ID(s8)���ص���Դ, ������Ƿ�Ϊ-1;
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
	u8 dll_rcv_indication;	//��Ҫ���̴߳���Ľ�������
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
	
	u8 max_identify_try;	// identify ʹ��
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
	u8 node_index;				//�ڵ����
	u8 node_num;				//�ڵ�����
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
	NO_ERROR = 0x00,			//û�д���
	TARGET_EXECUTE_ERR = 0xF0,	//Ŀ��ִ�д���, e.g. psr_inquireָ��ȡ�ؽڵ㷶ΧԽ��(���ش���)
	NO_PIPE_INFO = 0xF1,		//�ڵ�û�ж�Ӧ��PIPE��Ϣ
	NO_XPLAN = 0xF2,			//�ڵ�û����Ӧ�ķ���ģʽ��Ϣ
	ACK_TIMEOUT = 0xF3,			//�ڵ����´���ACK��ʱ, ����psr_send�Ĵ����pipe�м�ڵ㷵�ص�ack����
	PSR_TIMEOUT = 0xF4,			//��Ҫ�𸴵�PSR����ȴ��𸴳�ʱ: psr_send�ɹ�, ��������ʱû��psr reply����
	DIAG_TIMEOUT = 0xF5,		//psr_diag�Ľ��սڵ��ָ���ڵ�diag��ʱ�޷���ʱ���صĴ������, ��ACK_TIMEOUT��������, ���ִ�����Ҫrebuild pipe
	UID_NOT_MATCH = 0xF6,		//���յ�UID��PIPE�����(���ش���)
	MEM_OUT = 0xF7,				//�ڵ��޷����䵽�ڴ�(���ش���)
	TIMER_RUN_OUT = 0xF8,		//�ڵ��޷����䵽��ʱ��(���ش���)
	SENDING_FAIL = 0xF9,		//���ʹ���(���ش���)
	UNKNOWN_ERR = 0xFA			//����δ����Ĵ���(���ش���)
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
	u8 psr_rcv_indication;		//��Ҫ���̴߳���Ľ�������
	u8 psr_rcv_valid;			//psr_rcv_valid: [valid patrol biway uplink error index1 index0]
	ARRAY_HANDLE npdu;
	BASE_LEN_TYPE npdu_len;
} xdata PSR_RCV_STRUCT;

typedef PSR_SEND_STRUCT xdata * PSR_SEND_HANDLE;
typedef PSR_RCV_STRUCT xdata * PSR_RCV_HANDLE;

/* Topology ------------------------------------------------------------------------*/
#define NODE_STATE_DISCONNECT				0x0001	//�ڵ���ʱ�޷�����
#define NODE_STATE_EXCLUSIVE				0x0002	//�ڵ��ֹ��һ��explore(̨����ڵ�)
#define NODE_STATE_NEW_ARRIVAL				0x0004	//�·��ֵĽڵ�,��ȡ��һ�κ󼴳�����־
#define NODE_STATE_REFOUND					0x0008	//�Ѿ������������е��걸�ڵ�,��һ����Ӧ����ͬbuild_id�Ĺ㲥,������ζ���临λ��,����������ӹ�,
													//���ΪREFOUND֪ͨӦ�ò㴦��, ȡ��������־
#define NODE_STATE_RECOMPLETED				0x0010	//��REFOUND������ͬ,ֻ����REFOUND��Ӧ�ò�ȡ�����ǳ���,��RECOMPLETED��־����һ����������������ʱ�ų���,��ֹ�ڵ㷴������,Ӱ��Ч��
#define NODE_STATE_TEMPORARY				0x0020	//���ֵ��½ڵ�,���PING�ɹ�,�򱻼�������,������PING��������Ϣ
													//��λ��set_build_id����
													//��������״̬�ﵽCOMPLETED����(��Ϊ��set_build_id�ɹ�����,�޳ɹ�����)
#define NODE_STATE_RECONNECT				0x0040	//�ڵ㱻�����޸�
#define NODE_STATE_UPGRADE					0x0080	//�ڵ㱻�Ż�����

// ��λ�ǽڵ���Ӳ����ص�״̬
#define NODE_STATE_OLD_VERSION				0x8000	//�ڵ�Ĺ̼��汾�ǹ��ڵ�
#define NODE_STATE_ZX_BROKEN				0x4000	//�ڵ�����·ʧЧ
#define NODE_STATE_AMP_BROKEN				0x2000	//�ڵ�VHH����·ʧЧ



typedef struct NODE_STRUCT
{
	u8 uid[6];
	u16 cnt_child_nodes;							//��һ���ӽڵ���, Ϊ�������, 

	struct NODE_STRUCT * parent_node;				//ָ�򸸽ڵ�;
	struct NODE_STRUCT * first_child_node;			//ָ����һ���ӽڵ�;
	struct NODE_STRUCT * next_bro_node;				//ָ����һ��ͬ���ڵ�;
	struct NODE_STRUCT * previous_bro_node;			//ָ����һ��ͬ���ڵ�, Ϊ���������

	u8 uplink_plan;									//��parent_nodeͨ�ŵ����з���
	u8 uplink_ss;									//��parent_nodeͨ�ŵ�����ss
	u8 uplink_snr;									//��parent_nodeͨ�ŵ�����ebn0;
	u8 downlink_plan;								//��parent_nodeͨ�ŵ����з���
	u8 downlink_ss;									//��parent_nodeͨ�ŵ�����ss
	u8 downlink_snr;								//��parent_nodeͨ�ŵ�����enb0;

	u8 broad_class:2;								// ��2λ:��ǰ�ڵ�Ĺ㲥����(broad_class); armΪС�˶���, ��д��Ϊ��2λ

	u8 repair_fail_cnt:3;							// ����pipe������, ����һ��������·��
													// ����ɹ�������
	u8 set_disconnect_cnt:3;						// ��ǰ�ڵ�PIPE����עΪdisconnect����
													// ����һ������Ҫ��PIPE, ��ʹ����ɹ�Ҳ��ʧ�ܴ���, ����ѡpipe
													// ���½�pipeʱ����

	u16 alternative_progress_phase:2;				// ������λ
	u16 alternative_progress_fail:1;				// ������λ,���нڵ����ؽ�·��ʧ��,�ýڵ�����Ѿ����׻������滻,�����ע��ֹ�޾����޸���ȥ
	u16 alternative_progress_cnt:10;				// back_trace���ҽڵ����
	u16 grade_drift_cnt:3;							// 3bit�з�����
	

	u16 state;										// bit0: NODE_STATE_DISCONNECT	0x01	//�ڵ���ʱ�޷�����
													// bit1: NODE_STATE_EXCLUSIVE	0x02	//�ڵ��ֹ��һ��explore(̨����ڵ�)
													// bit2: NODE_STATE_ZX_BROKEN	0x04	//�ڵ�����·ʧЧ
													// bit3: NODE_STATE_NEW_ARRIVAL	0x08	//�·��ֵĽڵ�,��ȡ��һ�κ󼴳�����־
													// bit4: NODE_STATE_OLD_VERSION	0x10	//�ڵ�Ĺ̼��汾�ǹ��ڵ�
													// bit5: NODE_STATE_REFOUND		0x20	//��һ��explore�����б������Ѵ����������еĽڵ�(�����и�λ����)���б��,ֻ������һ��,��ֹ�䲻�ϸ�������
													// bit6: NODE_STATE_PIPE_FAIL	0x40	//���ýڵ�pipe fail,��ֻ���������иýڵ�,pipe�����޶�Ӧpipe.
													// bit7: NODE_STATE_AMP_BROKEN	0x80	//�ڵ�VHH����·ʧЧ
	

	u8 user_data_len;								// 2014-09-24 �ڵ�����ʱ��CC���ݵ��û�����Ϣ
	u8 user_data[CFG_USER_DATA_BUFFER_SIZE];
	
}NODE_STRUCT;



typedef NODE_STRUCT * NODE_HANDLE;

typedef enum
{
	ALL = 0				// ����������������
	,SOLO				// ���ڵ�
	,NON_SOLO			// �Ƕ��ڵ�
}TOPOLOGY_COND_ENUM;

typedef enum
{
	NODE_BROAD_CLASS = 0	// �ڵ�Ĺ㲥����
	,NODE_DISCONNECT		// �ڵ�״̬
}NODE_PROPERTY_ENUM;



/************ Management & APP *************************/
typedef struct
{
	u8		mgnt_rcv_indication;	//��Ҫ���̴߳���Ľ�������
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
}xdata APP_SEND_STRUCT;				//�û�ʹ�õķ������ݽṹ

typedef struct
{
	PROTOCOL_TYPE	protocol;

	/* use in psr */
	u16		pipe_id;

	/* use in dst */
	u8 		src_uid[6];
	COMM_MODE_TYPE		comm_mode;
//#if APP_RCV_SS_SNR == 1			//2015-03-30 �̶����Ӵ�����λ
	s8		ss;						//�ź�ǿ��
	s8		snr;					//�����
//#endif

	/* use in all protocol */
	u8 		phase;
	ARRAY_HANDLE 	apdu;
	BASE_LEN_TYPE	apdu_len;
}xdata APP_RCV_STRUCT;				//�û�ʹ�õĽ������ݽṹ

//typedef struct
//{
//	u8		phase;
//	u8 		target_mid[6];
//	ARRAY_HANDLE apdu;
//	u8		apdu_len;
//}xdata APP_UID_SEND_STRUCT;	//�û�ʹ�õ���uidΪ�����ķ������ݽṹ

typedef struct
{
	u8			phase;
	u8			search;
	u8			forward;			//1:ת��; 2:����
#if NODE_TYPE==NODE_MASTER
	u8			search_mid;			//valid only in meter_id search
#endif
	ARRAY_HANDLE 	nsdu;
	BASE_LEN_TYPE	nsdu_len;
}DST_SEND_STRUCT;		//DSTͨ�ŷ������ݽṹ, ��������������ȫ����������

//typedef struct
//{
//	u8			phase;
//	RATE_TYPE	rate;
//	u8			src_uid[6];
//	ARRAY_HANDLE apdu;
//	u8			apdu_len;
//}DST_RCV_STRUCT;		//DSTͨ�Ž������ݽṹ

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
}xdata DST_STACK_RCV_STRUCT;		//DSTͨ�Ž������ݽṹ

typedef struct
{
	COMM_MODE_TYPE comm_mode;
	u8 jumps;
	u8 forward_prop;	// [search_mid config_lock network_ena acps_ena 0 window_index[2:0]]	2014-11-17 ��λ��ʾ����������

	//	�ظ�,ת��ʹ��
	u8 conf;			// ����app��֮��, ������Ϣ, ���ڷ���ʱ����
	u8 stamps;
	BASE_LEN_TYPE ppdu_len;		// �������ݰ���
}DST_CONFIG_STRUCT;		// CC��������������÷���ģʽ, MT��������������ûظ�ģʽ

typedef struct
{
	u8		app_rcv_indication;
	u8		phase;
	u16		pipe_id;
	ARRAY_HANDLE apdu;
	u8			uplink;
	BASE_LEN_TYPE	apdu_len;
}xdata APP_STACK_RCV_STRUCT;//Э��ջʹ�õĽ������ݽṹ

typedef APP_SEND_STRUCT xdata * APP_SEND_HANDLE;
typedef APP_RCV_STRUCT xdata * APP_RCV_HANDLE;
typedef APP_STACK_RCV_STRUCT xdata * APP_STACK_RCV_HANDLE;

typedef DST_SEND_STRUCT xdata * DST_SEND_HANDLE;
typedef DST_STACK_RCV_STRUCT xdata * DST_STACK_RCV_HANDLE;

typedef struct				//�������洢�����PIPE�����ݸ�ʽ
{
	u8	phase;
	u16 pipe_info;								// pipe_info: [biway 0 0 0 pipe_id(12 bits)]
	u8  pipe_stages;							// pipe����
	ADDR_REF_TYPE way_uid[CFG_PIPE_STAGE_CNT];	// ÿһ���ڵ��uid
	u8	xmode_rmode[CFG_PIPE_STAGE_CNT];		// ÿһ��������,����ͨ�ŷ�ʽ
}PIPE_MNPL_STRUCT;

typedef PIPE_MNPL_STRUCT xdata * PIPE_REF;

typedef struct
{
	u16 pipe_id;
	u8 uid[6];
	PSR_ERROR_TYPE err_code;
}xdata ERROR_STRUCT;
typedef ERROR_STRUCT xdata * ERR_HANDLE;
#define TEST_PIPE_ID	0x0FFF					// ������pipe_id, ���ܱ���ѯ


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
	BUILD_RESTART = 0,			// ɾ���������˽ڵ�, ����������������
	BUILD_RENEW = 1,			// ������ǰ����������, ֻ�ǽ����нڵ��������������Ϊ0, ��δִ������״̬, �������������¼���ڵ��������������
	BUILD_CONTINUE
}BUILD_CONTROL;

typedef enum
{
	TO_BE_CONTINUE = 0,
	COMPLETED,
	ZERO_SEARCHED				// 2014-04-21 Ϊ��ǰ��������һ��״̬
}BUILD_RESULT;

typedef struct
{
	u8 max_broad_class; 	// ����ʹ�õ�������ʼ��� 0: bpsk; 1: ds15
	u8 max_build_depth; 	// ���������� 1: ���ڵ��һ���ڵ�; 0: ������
	u8 default_broad_windows;
	u8 current_broad_class;	// ��ǰʹ�õ����ʼ���
}BUILD_CONFIG_STRUCT;


/************ Network Optimization *************************/
typedef struct
{
	u8 num_nodes;
	u8 uid[CFG_PIPE_STAGE_CNT][6];
}PATH_STRUCT;

typedef enum
{
	AUTOHEAL_LEVEL0 = 0,	//LEVEL0: ������, �����޸�
	AUTOHEAL_LEVEL1,		//LEVEL1: ����, ����ȷ�յ�NO_PIPE_INFOʱ��ˢPIPE
	AUTOHEAL_LEVEL2,		//LEVEL2: ���� + ��ˢPIPE + �޸�·����·
	AUTOHEAL_LEVEL3,		//LEVEL3: ���� + ��ˢPIPE + �޸�·����· + �ؽ�·��
}AUTOHEAL_LEVEL;

	/****** ·���ؽ���ѡ����� ******/
#define REBUILD_PATH_CONFIG_TRY_OTHER_PHASE	0x80	//��ԭ��λ����ʧ��ʱ, ��̽������λ
#define REBUILD_PATH_CONFIG_LONGER_JUMPS	0x40	//�ø�����·������
#define REBUILD_PATH_CONFIG_SLOWER_RATE		0x20	//�ø������ٶ�����
#define REBUILD_PATH_CONFIG_SYNC_TOPOLOGY 	0x10	//��PIPE�ı�Ľ��ͬ�������˿���


#endif

