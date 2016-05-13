/******************** (C) COPYRIGHT 2012 Belling Inc. ********************
* File Name          : ebc.c
* Author             : Lv Haifeng
* Version            : V2.0
* Date               : 2013-03-14
* Description        : EBC协议用于限定一系列相应条件, 寻找节点旁边可以双向通信的节点UID集合
					The dll layer of bl6810 powermesh is in charge of cover the phy layer details, and 
					appear a clean and clear view to upper layer: NW/MNGT/APP.

					To achieve this goal, I invented some approachs, the main method is:
					dll_diag(): a normal method to diag the link quality between two nodes;
					
					However, dll_diag() needs the nodes know each other. In the initial stage of 
					the network, nodes UIDs are unknown. So the first job in initial stage is to discover
					the connections around the node. 

					To get this goal, I create an set of protocals called EDP, (extended diag protocols)
					which is valid if the DLCT flags are special.

					The first protocol of EDP is EBC (extended broad cast), which implements a procedure I called EXPLORE
					the major process is:

					1. main node broadcast a special packet named NBF(neibour broadcast frame), which contains a set of filter conditions;
					2. all the node that got NBF should response it, if they match the conditions in NBF. These response packet
						called SBRF(short broadcast response frame), it is a special short 4-byte package contails only the broad-
						cast ID in the NBF and a 12-bit random id generated by random algorithm; SBRF is very short, considering
						the network competition;
					3. main node collected all these SBRF in the waiting windows and call for these nodes one by one by the packet called NIF;
					4. node got NIF and response with NAF (neibour affirmative frame);
					5. main node got NAF and confirm with NCF (neibour confirm frame);

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
//#include "compile_define.h"
//#include "powermesh_datatype.h"
//#include "powermesh_config.h"
//#include "powermesh_spec.h"
//#include "powermesh_timing.h"
//#include "hardware.h"
//#include "mem.h"
//#include "general.h"
//#include "timer.h"
//#include "uart.h"
//#include "phy.h"
//#include "dll.h"
//#include "ebc.h"

/* Private define ------------------------------------------------------------*/
#define 	CST_BPSK_DIFF	80			// BPSK下同相标准相位差
#define 	CST_DS15_DIFF	170			// DS15下同相标准相位差
#define 	CST_DS63_DIFF	35			// DS63下同相标准相位差

#ifdef BUILD_ID_16
u16 xdata	_ebc_build_id = 0;
#else
u8  xdata   _ebc_build_id = 0;		// ebc_build_id相同的ebc call, 不响应; 2013-08-08 三相设置为同一个build_id
#endif

/*******************************************************************************
* Function Name  : get_build_id
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u8 get_build_id(u8 phase)
{
	phase = phase;
	return _ebc_build_id;
}


#ifdef USE_EBC
/* Private typedef -----------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
	/* For Caller */
u16 xdata	_ebc_bsrf_db[EBC_NEIGHBOR_CNT]; 	/* _ebc_bsrf_db, 存储周边节点使用bsrf发来的randomid的集合 ***/
u8	xdata	_ebc_bsrf_db_usage;

u8  xdata  	_ebc_uid_db[EBC_NEIGHBOR_CNT][6];	/* 存储使用EBC找到的节点UID集合 ***/
u8  xdata  	_ebc_uid_db_usage;

	/* For Responser */
u8	xdata	_ebc_caller_uid[CFG_PHASE_CNT][6];			/* 发来EBC广播的节点UID */
u8	xdata	_ebc_response_info[CFG_PHASE_CNT][2];		// [0]:{broadcase_id{7:4}, random_id{11:8}}, [1]:{random_id{7:0}}
u8	xdata	_ebc_confirm[CFG_PHASE_CNT];
//u8  xdata   _ebc_build_id[CFG_PHASE_CNT];				// ebc_build_id相同的ebc call, 不响应;




/* Private function prototypes -----------------------------------------------*/
void reset_ebc_caller_db(void);

	/* 操作BSRF_DB */
void reset_bsrf_db(void);
STATUS add_bsrf_db(u16 rid);
STATUS inquire_bsrf_db(u8 index, u16 xdata * rid);
u8 get_bsrf_db_usage(void);

	/* 操作UID_DB */
void reset_neighbor_uid_db(void);
STATUS add_neighbor_uid_db(ARRAY_HANDLE uid);
STATUS inquire_neighbor_uid_db(u8 index, ARRAY_HANDLE ptw);
u8 get_neighbor_uid_db_usage(void);

	/* 操作RESPONSE_DB */
void reset_response_db(void);
u8 add_response_db(u8 phase, ARRAY_HANDLE src_uid, u16 random_id);
BOOL inquire_response_db(u8 phase, ARRAY_HANDLE src_uid, ARRAY_HANDLE random_id);
void confirm_response_db(u8 phase);

	/* EBC Operation */
u8 ebc_complete_nbf(EBC_BROADCAST_HANDLE pt_ebc, ARRAY_HANDLE lsdu);
u8 ebc_broadcast(EBC_BROADCAST_HANDLE pt_ebc);
STATUS ebc_identify(EBC_BROADCAST_HANDLE pt_ebc, u16 random_id, u8 xdata * indentified_uid);

u8 check_edp_protocol(ARRAY_HANDLE lpdu,u8 protocol, u8 frame_type);

#ifdef DEBUG_EBC
u8 xdata debug_ebc_fixed_window;
#endif

/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : set_build_id
* Description    : 2013-01-28, build_id() for pipe network building. once the node has been added to the topology 
					tree, it will be set a build_id, and it will not response later EBC broadcast even it is from 
					other nodes.
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void set_build_id(u8 phase, u8 new_build_id)
{
	phase = phase;					//保持接口兼容性
	_ebc_build_id = new_build_id;
}

/*******************************************************************************
* Function Name  : init_ebc
* Description    : Caller操作两个库, 一个是bsrf库, 一个是uid库
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void init_ebc(void)
{
	reset_ebc_caller_db();
	reset_response_db();
	
#ifdef DEBUG_EBC
	debug_ebc_fixed_window = 0;
#endif
}

/*******************************************************************************
* Function Name  : reset_ebc_caller_db
* Description    : bsrf database: EBC发送端用于存储b-srf响应信息的数据结构
* 					根据EBC协议, 当发起方发起NBF广播后, 凡符合响应条件的响应节点将按随机延时以bsrf(超短响应包)的方式响应, 
*					广播方在窗口时间内, 一一记录接收到的bsrf内的random_id, 接收的random_id存储在ebc_neighbor_bsrf_db数据库里
* 					记录B-SRF的randomID的数据结构 由一个u8变量记录总条目数, 一个u16数组记录所有接收到的randomID
* 					提供由四个函数组成的接口: reset_ebc_caller_db()/add_bsrf_db()/get_bsrf_db_usage()/inquire_bsrf_db()
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void reset_ebc_caller_db(void)
{
	//ebc_broad_id = 0;
	//ebc_xmode = 0;
	//ebc_rmode = 0;
	//ebc_scan = 0;

	reset_bsrf_db();
	reset_neighbor_uid_db();
}

/*******************************************************************************
* Function Name  : reset_bsrf_db
* Description    : add new received bsrf info into the database.
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void reset_bsrf_db(void)
{
	mem_clr(_ebc_bsrf_db,sizeof(_ebc_bsrf_db),1);	//sizeof直接计算数组总共占用的总内存字节大小, 计算结构体单个占用的字节大小
	_ebc_bsrf_db_usage = 0;
}

/*******************************************************************************
* Function Name  : add_bsrf_db
* Description    : add new received bsrf info into the database.
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS add_bsrf_db(u16 rid)
{
	if(_ebc_bsrf_db_usage<EBC_NEIGHBOR_CNT)
	{
		_ebc_bsrf_db[_ebc_bsrf_db_usage] = rid;
		_ebc_bsrf_db_usage++;
		return OKAY;
	}
	else
	{
		return FAIL;
	}
}

/*******************************************************************************
* Function Name  : get_bsrf_db_usage
* Description    : return the usage of bsrf db.
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u8 get_bsrf_db_usage(void)
{
	return _ebc_bsrf_db_usage;
}

/*******************************************************************************
* Function Name  : inquire_bsrf_db
* Description    : get rid by index
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS inquire_bsrf_db(u8 index, u16 xdata * rid)
{
	if(index >= _ebc_bsrf_db_usage)
	{
		return FAIL;
	}
	else
	{
		*rid = _ebc_bsrf_db[index];
		return OKAY;
	}
}


/*******************************************************************************
* Function Name  : reset_neighbor_uid_db
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void reset_neighbor_uid_db(void)
{
	mem_clr(_ebc_uid_db,sizeof(_ebc_uid_db),1);
	_ebc_uid_db_usage = 0;
}

/*******************************************************************************
* Function Name  : add_neighbor_uid_db
* Description    : add confirmed node uid into the database.
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS add_neighbor_uid_db(ARRAY_HANDLE uid)
{
	if(_ebc_uid_db_usage<EBC_NEIGHBOR_CNT)
	{
		mem_cpy(&_ebc_uid_db[_ebc_uid_db_usage][0], uid, 6);
		_ebc_uid_db_usage++;
		return OKAY;
	}
	else
	{
		return FAIL;
	}
}

#if DEVICE_TYPE==DEVICE_CC
/*******************************************************************************
* Function Name  : delete_neighbor_uid_db
* Description    : delete a uid from uid database.
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS delete_neighbor_uid_db(u8 index_to_delete)
{
	if(!_ebc_uid_db_usage)
	{
#ifdef DEBUG_MGNT
		my_printf("delete_neighbor_uid_db():EBC_NEIGHBOR_CNT==0!\r\n");
#endif
		return FAIL;
	}

	if(index_to_delete>=_ebc_uid_db_usage)
	{
#ifdef DEBUG_MGNT
		my_printf("delete_neighbor_uid_db(): delete index invalid!\r\n");
#endif
		return FAIL;
	}
	else
	{
		u8 i;

		for(i=index_to_delete;i<_ebc_uid_db_usage-1;i++)
		{
			mem_cpy((ARRAY_HANDLE)(&_ebc_uid_db[i][0]), (ARRAY_HANDLE)(&_ebc_uid_db[i+1][0]), 6);
		}

		_ebc_uid_db_usage--;
	}
	return OKAY;
}
#endif

/*******************************************************************************
* Function Name  : get_neighbor_uid_db_usage
* Description    : get the neighbor database count.
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
//u8 get_neighbor_uid_db_usage()
//{
//	return _ebc_uid_db_usage;
//}


/*******************************************************************************
* Function Name  : inquire_neighbor_uid_db
* Description    : inquire the neighbor database.
* Input          : index, pointer to write the uid
* Output         : target 6 bytes uid.
* Return         : OKAY/FAIL;
*******************************************************************************/
STATUS inquire_neighbor_uid_db(u8 index, ARRAY_HANDLE neighbor_uid_buffer)
{
	if(index >= _ebc_uid_db_usage)
	{
		return FAIL;
	}
	else
	{
		mem_cpy(neighbor_uid_buffer,&_ebc_uid_db[index][0],6);
		return OKAY;
	}
}

/*******************************************************************************
* Function Name  : reset_response_db(void)
* Description    : clear all the ebc response memory;
* Input          : void
* Output         : None
* Return         : None
*******************************************************************************/
void reset_response_db(void)
{
	mem_clr(_ebc_confirm,sizeof(_ebc_confirm),1);
	mem_clr(_ebc_response_info,sizeof(_ebc_response_info),1);
	mem_clr(_ebc_caller_uid,sizeof(_ebc_caller_uid),1);
	mem_clr(&_ebc_build_id,sizeof(_ebc_build_id),1);
}

/*******************************************************************************
* Function Name  : add_response_db(...)
* Description    : add source uid and broadid and generated random id into the ebc response database;
* Input          : source id, random id
* Output         : None
* Return         : None
*******************************************************************************/
u8 add_response_db(u8 phase, ARRAY_HANDLE src_uid, u16 random_id)
{
	mem_cpy(_ebc_caller_uid[phase],src_uid,6);
	_ebc_response_info[phase][0] = (u8)(random_id>>8);
	_ebc_response_info[phase][1] = (u8)random_id;
	_ebc_confirm[phase] = FALSE;
	
	return OKAY;
}

/*******************************************************************************
* Function Name  : inquire_response_db(...)
* Description    : look up local ebc response database and judge whether the NIF and NCF should be responsed;
* Input          : source id, random id
* Output         : None
* Return         : None
*******************************************************************************/
BOOL inquire_response_db(u8 phase, u8 xdata * src_uid, u8 xdata * random_id)
// 判断nif携带的src uid, bid和 random_id是否与本地存储一致, 是则返回TRUE
{

	if(!mem_cmp(src_uid,&_ebc_caller_uid[phase][0],6))
	{
		return FALSE;
	}
	
	if(random_id[0] != _ebc_response_info[phase][0])
	{
		return FALSE;
	}
	
	if(random_id[1] != _ebc_response_info[phase][1])
	{
		return FALSE;
	}
	
	return TRUE;
	
}

/*******************************************************************************
* Function Name  : confirm_response_db(...)
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void confirm_response_db(u8 phase)
{
	_ebc_confirm[phase] = TRUE;
}


/*******************************************************************************
* Function Name  : ebc_complete_nbf
* Description    : 填写nbf的lsdu,最大写入19字节
* Input          : 
* Output         : None
* Return         : 
*******************************************************************************/
u8 ebc_complete_nbf(EBC_BROADCAST_HANDLE pt_ebc, ARRAY_HANDLE lsdu)
{
	BASE_LEN_TYPE lsdu_len;

	*(lsdu++) = EXP_EDP_EBC | (encode_xmode(pt_ebc->rmode,pt_ebc->scan) << 2) | EXP_EDP_EBC_NBF; //NBF_CONF						
	*(lsdu++) = ((pt_ebc->bid)<<4) | (pt_ebc->window);					//NBF_ID
	*(lsdu++) = pt_ebc->mask; 											//NBF_MASK
	lsdu_len = 3;

	if(pt_ebc->mask & BIT_NBF_MASK_ACPHASE)
	{
		*(lsdu++) = 0;						//AC Phase在发送时重新填写, 先把位置占住;
		lsdu_len++;
	}

	//if(pt_ebc->mask & BIT_NBF_MASK_SS)
	//{
	//	*(lsdu++) = pt_ebc->ss;
	//	lsdu_len++;
	//}

	//if(pt_ebc->mask & BIT_NBF_MASK_SNR)
	//{
	//	*(lsdu++) = pt_ebc->snr;
	//	lsdu_len++;
	//}

	if(pt_ebc->mask & BIT_NBF_MASK_UID)
	{
		mem_cpy(lsdu,pt_ebc->uid,6);
		lsdu += 6;
		lsdu_len += 6;
	}

	if(pt_ebc->mask & BIT_NBF_MASK_METERID)
	{
		mem_cpy(lsdu,pt_ebc->mid,6);
		lsdu += 6;
		lsdu_len += 6;
	}

	if(pt_ebc->mask & BIT_NBF_MASK_BUILDID) // 2013-01-28 add build_id configuration
	{
		*(lsdu++) = pt_ebc->build_id;
		lsdu_len++;
	}

	return lsdu_len;
}

/*******************************************************************************
* Function Name  : ebc_broadcast(...)
* Description    : ebc broadcast, 发送广播帧, 返回响应的bsrf数目;
* Input          : 
* Output         : nodes collected
* Return         : num of node found.
*******************************************************************************/
u8 ebc_broadcast(EBC_BROADCAST_HANDLE pt_ebc)
{
	ARRAY_HANDLE lsdu;
	TIMER_ID_TYPE tid;
	SEND_ID_TYPE sid;
	STATUS status;
	
	DLL_RCV_HANDLE pd;
	ARRAY_HANDLE ppdu;

	DLL_SEND_STRUCT xdata ds;
	u32 xdata expiring_sticks;
	u8 xdata all_zero_uid[6];
	u8 xdata neighbors = 0;

	/* Init, and Make Parameter Sure */
	reset_bsrf_db();

	/* limit ebc window */
	switch(pt_ebc->rmode & 0x03)
	{
		case(0x00):
		{
			if(pt_ebc->window > CFG_EBC_MAX_BPSK_WINDOW)
			{
				pt_ebc->window = CFG_EBC_MAX_BPSK_WINDOW;
			}
			break;
		}
		case(0x01):
		{
			if(pt_ebc->window > CFG_EBC_MAX_DS15_WINDOW)
			{
				pt_ebc->window = CFG_EBC_MAX_DS15_WINDOW;
			}
			break;
		}
		case(0x02):
		{
			if(pt_ebc->window > CFG_EBC_MAX_DS63_WINDOW)
			{
				pt_ebc->window = CFG_EBC_MAX_DS63_WINDOW;
			}
		}
	}

	pt_ebc->bid &= 0x0F;						// limit bid to 4 bit;
	
	/* Req Resources */
	tid = req_timer();
	if(tid==INVALID_RESOURCE_ID)
	{
		return 0;
	}
	lsdu = OSMemGet(MINOR);
	if(!lsdu)
	{
		delete_timer(tid);
		return 0;
	}

#ifdef DEBUG_EBC
	my_printf("NBF EXPLORE\r\n");
#endif	


	////backup ebc info for later indentify and confirm 
	//_ebc_broad_id = pt_ebc->bid;
	//_ebc_xmode = pt_ebc->xmode;
	//_ebc_rmode = pt_ebc->rmode;
	//_ebc_scan = pt_ebc->scan;

	// send nbf
	ds.phase = pt_ebc->phase;
	ds.xmode = pt_ebc->xmode;
	ds.rmode = pt_ebc->rmode;
	mem_clr(all_zero_uid,sizeof(all_zero_uid),1);
	ds.target_uid_handle = all_zero_uid;
	ds.lsdu = lsdu;
	ds.lsdu_len= ebc_complete_nbf(pt_ebc,lsdu);
	ds.prop = BIT_DLL_SEND_PROP_EDP|((pt_ebc->scan)?BIT_DLL_SEND_PROP_SCAN:0);
	if(pt_ebc->mask & BIT_NBF_MASK_ACPHASE)
	{
		ds.prop |= BIT_DLL_SEND_PROP_ACUPDATE;
	}
	ds.delay = DLL_SEND_DELAY_STICKS;

	expiring_sticks = (windows_delay_sticks(4, (pt_ebc->rmode & 0x03), pt_ebc->scan, 1, 0x01<<(pt_ebc->window)));
//	expiring_sticks += (expiring_sticks>>6);	// 2012-12-13 max waiting time * 1.0156(1/64)	
	expiring_sticks += (expiring_sticks>>4)+100;

#ifdef USE_MAC
	declare_channel_occupation(ds.phase,phy_trans_sticks(ds.lsdu_len + LEN_TOTAL_OVERHEAD_BEYOND_LSDU, ds.xmode & 0x03, ds.prop & BIT_DLL_SEND_PROP_SCAN) + expiring_sticks);
#endif

	sid = dll_send(&ds);

	// check send result
	if(sid==INVALID_RESOURCE_ID)
	{
		delete_timer(tid);
		OSMemPut(MINOR,lsdu);
		return 0;
	}
	status = wait_until_send_over(sid);
	if(!status)
	{
		delete_timer(tid);
		OSMemPut(MINOR,lsdu);
		return 0;
	}

	set_timer(tid,expiring_sticks);	//需要按最大时间设置延时等待
#ifdef DEBUG_EBC
	my_printf("WINDOW=%d,WAIT %ld ts\r\n",(0x01)<<pt_ebc->window,(u32)expiring_sticks);
#endif
	do
	{
#if DEVICE_TYPE==DEVICE_CC
		SET_BREAK_THROUGH("quit ebc_broadcast()\r\n");
#endif
		pd = dll_rcv();
		// 在最大为2^window的时间内, 记录所有接收到的bid匹配的随机数
		if(pd)
		{
			if((pd->dll_rcv_valid & (BIT_DLL_VALID|BIT_DLL_ACK|BIT_DLL_SRF)) == (BIT_DLL_VALID|BIT_DLL_ACK|BIT_DLL_SRF)
#if DEVICE_TYPE==DEVICE_CC
			 && (pd->phase==pt_ebc->phase)				// 三相设备比较相位
#endif
			)
			{
				ppdu = GET_PPDU(pd);
				if((*(ppdu++) & 0x03) == FIRM_VER)
				{
					if(((*ppdu)>>4) == pt_ebc->bid)
					{
						u16 temp16;
						
						temp16 = (*ppdu++) & 0x0F;		//响应包为B-SRF, 固定4字节, random_id 12 bits restored in low 4 bits of pt[1] & pt[2];
						temp16 <<= 8;
						temp16 += (*ppdu);
						add_bsrf_db(temp16);
#ifdef DEBUG_EBC
						uart_send8('.');
						print_phy_rcv(GET_PHY_HANDLE(pd));
#endif
					}
				}
			}
			dll_rcv_resume(pd);
		}
		FEED_DOG();
	}while(!is_timer_over(tid));
	neighbors = get_bsrf_db_usage();
#ifdef DEBUG_EBC
	my_printf("\r\nRCV %bd B-SRF RESP.\r\n", neighbors);
#endif

	delete_timer(tid);
	OSMemPut(MINOR,lsdu);
	return neighbors;
}

/*******************************************************************************
* Function Name  : ebc_identify_transaction(...)
* Description    : 使用NIF对发出BSRF的节点识别, 并用NCF确认
* Input          : 
* Output         : 
* Return         : num of node found.
*******************************************************************************/
u8 ebc_identify_transaction(EBC_BROADCAST_HANDLE pt_ebc, u8 num_nodes)
{
	u16 xdata random_id;
	u8 i;
	u8 xdata indentified_uid[6];

	
	reset_neighbor_uid_db();
	for(i = num_nodes; i>0; i--)
	{
#if DEVICE_TYPE==DEVICE_CC	
		SET_BREAK_THROUGH("quit ebc_identify_transaction().\r\n");
#endif
#ifdef DEBUG_EBC
		my_printf("INQUIRING BSRF #%bd...\r\n", i);
#endif
		if(inquire_bsrf_db(i-1,&random_id) == OKAY)
		{
			if(ebc_identify(pt_ebc,random_id,indentified_uid) == OKAY)
			{
#ifdef DEBUG_EBC
				my_printf("OK. UID: ", i);
				uart_send_asc(indentified_uid,6);
#endif
				add_neighbor_uid_db(indentified_uid);
#ifdef DEBUG_EBC
				my_printf("CMPT.\r\n");
#endif
			}
			else
			{
#ifdef DEBUG_EBC
				my_printf("identify fail.\r\n");
#endif
				num_nodes --;
			}
		}
		else
		{
			num_nodes --;
		}
	}
	return num_nodes;
}


/*******************************************************************************
* Function Name  : ebc_identify(...)
* Description    : After collecting all the B-SRF , the main node inquire nodes by the broad_id and random_id restored
					in the B-SRFs, one by one, by NIF;
					and confirm
* Input          : 
* Output         : target_id
* Return         : 
*******************************************************************************/
STATUS ebc_identify(EBC_BROADCAST_HANDLE pt_ebc, u16 random_id, u8 xdata * identified_uid)
{
	u8 xdata retry;
	TIMER_ID_TYPE tid;
	SEND_ID_TYPE sid;
	DLL_RCV_HANDLE pd;
	STATUS status = FAIL;
	ARRAY_HANDLE lsdu_send;
	ARRAY_HANDLE lpdu_rcv;
	
	DLL_SEND_STRUCT xdata ds;
	u8 xdata all_zero_uid[6];
	u16 xdata rid;
	u32 xdata expiring_sticks;

	u8 xdata rmode = pt_ebc->rmode;
	u8 xdata scan = pt_ebc->scan;

	tid=req_timer();
	if(tid==INVALID_RESOURCE_ID)
	{
#ifdef DEBUG_EBC
		my_printf("identify req timer fail.\r\n");
#endif
		return FAIL;
	}

	lsdu_send = OSMemGet(MINOR);
	if(!lsdu_send)
	{
#ifdef DEBUG_EBC
		my_printf("identify req mem fail.\r\n");
#endif
		delete_timer(tid);
		return FAIL;
	}
	
	for(retry=0;retry<pt_ebc->max_identify_try;retry++)		// max retry times : CFG_EBC_INDENTIFY_TRY
	{
		ds.phase = pt_ebc->phase;
		ds.xmode = pt_ebc->xmode;
		ds.rmode = pt_ebc->rmode;
		mem_clr(all_zero_uid,sizeof(all_zero_uid),1);
		ds.target_uid_handle = all_zero_uid;
		ds.lsdu = lsdu_send;
		lsdu_send[0] = EXP_EDP_EBC | EXP_EDP_EBC_NIF | (encode_xmode(rmode,scan) << 2);
		lsdu_send[1] =  (pt_ebc->bid<<4) + ((u8)(random_id>>8)&0x0F);
		lsdu_send[2] = (u8)random_id;
		ds.lsdu_len= 3;
		ds.prop = BIT_DLL_SEND_PROP_EDP|((scan)?BIT_DLL_SEND_PROP_SCAN:0);
		ds.delay = DLL_SEND_DELAY_STICKS;

		expiring_sticks = dll_ack_expiring_sticks(ds.lsdu_len + LEN_TOTAL_OVERHEAD_BEYOND_LSDU, rmode&0x03, scan);
#ifdef USE_MAC
		declare_channel_occupation(ds.phase,phy_trans_sticks(ds.lsdu_len + LEN_TOTAL_OVERHEAD_BEYOND_LSDU, ds.xmode & 0x03, ds.prop & BIT_DLL_SEND_PROP_SCAN) + expiring_sticks);
#endif

		sid = dll_send(&ds);
		if(sid==INVALID_RESOURCE_ID)
		{
#ifdef DEBUG_EBC
			my_printf("identify dll send fail.\r\n");
#endif

			delete_timer(tid);
			OSMemPut(MINOR,lsdu_send);
			return FAIL;
		}
		status = wait_until_send_over(sid);
		if(!status)
		{
#ifdef DEBUG_EBC
			my_printf("identify phy send fail.\r\n");
#endif

			delete_timer(tid);
			OSMemPut(MINOR,lsdu_send);
			return FAIL;
		}

		set_timer(tid,expiring_sticks);
#ifdef DEBUG_EBC
		my_printf("RID:0x%X, WAITING:%ld\r\n",random_id,expiring_sticks);
#endif
		status = FAIL;
		do
		{
#if DEVICE_TYPE==DEVICE_CC	
			SET_BREAK_THROUGH("quit ebc_identify().\r\n");
#endif
			pd = dll_rcv();
			if(pd)
			{
				if(((pd->dll_rcv_valid & (BIT_DLL_VALID | BIT_DLL_ACK)) == (BIT_DLL_VALID | BIT_DLL_ACK)) &&
						!(pd->dll_rcv_valid & BIT_DLL_SRF)
#if DEVICE_TYPE==DEVICE_CC
						&& (pd->phase == pt_ebc->phase)
#endif
						)
				{
					lpdu_rcv = pd->lpdu;
					if(check_edp_protocol(lpdu_rcv,EXP_EDP_EBC,EXP_EDP_EBC_NAF) == OKAY)
					{
						rid = lpdu_rcv[SEC_LPDU_NAF_ID] & 0x0F;
						rid <<= 8;
						rid += lpdu_rcv[SEC_LPDU_NAF_ID2];
	
						if(((lpdu_rcv[SEC_LPDU_NAF_ID]>>4)== (pt_ebc->bid&0x0F)) && (rid==random_id))
						{
#ifdef DEBUG_EBC
							my_printf("GOT NAF,");
#endif
							mem_cpy(identified_uid,&lpdu_rcv[SEC_LPDU_SRC],6);
							status = OKAY;
						}
						else
						{
#ifdef DEBUG_EBC
							my_printf("error 1, %bx,%bx,rid:%x, random_id:%x",(lpdu_rcv[SEC_LPDU_NAF_ID]>>4),pt_ebc->bid,rid,random_id);
							print_phy_rcv(GET_PHY_HANDLE(pd));
#endif
						
						}
					}
					// 多个集中器组网时完全可以出现接收到其他数据包的问题
//					else
//					{
//#ifdef DEBUG_EBC
//						my_printf("error 2");
//						print_phy_rcv(GET_PHY_HANDLE(pd));
//#endif
//					}
				}
//				else
//				{
//#ifdef DEBUG_EBC
//					my_printf("error 3");
//					print_phy_rcv(GET_PHY_HANDLE(pd));
//#endif
//				}
				dll_rcv_resume(pd);
			}
			FEED_DOG();
		}while(!is_timer_over(tid));
		
		if(status)
		{
#ifdef DEBUG_EBC
			my_printf("CONFIRM IT.\r\n");
#endif
			ds.target_uid_handle = identified_uid;
			lsdu_send[0] = EXP_EDP_EBC | EXP_EDP_EBC_NCF;					//需要重写lsdu, 因为发送后lsdu已经被移位;
			lsdu_send[1] =	(pt_ebc->bid<<4) + ((u8)(random_id>>8)&0x0F);
			lsdu_send[2] = (u8)random_id;
			ds.lsdu = lsdu_send;
			ds.lsdu_len = 3;
			sid = dll_send(&ds);
			wait_until_send_over(sid);
			break;
		}
		else
		{
#ifdef DEBUG_EBC
			my_printf("TIMEOUT.\r\n");
#endif
		}
	}

	delete_timer(tid);
	OSMemPut(MINOR,lsdu_send);
	return status;
}




/*******************************************************************************
* Function Name  : check_edp_protocol(...)
* Description    : check frame type before process.
* Input          : 
* Output         : None
* Return         : None
*******************************************************************************/
u8 check_edp_protocol(ARRAY_HANDLE lpdu, u8 protocol, u8 frame_type)
// 检查接收到的数据包属于EDP协议的帧类型
{
	if((lpdu[SEC_LPDU_DLCT] & EXP_DLCT_EDP) == EXP_DLCT_EDP)	// 是否属于EDP协议
	{
		if(protocol == EXP_EDP_EBC)
		{
			if((lpdu[SEC_LPDU_NBF_CONF]&0xC0) == EXP_EDP_EBC)
			{
				if(frame_type == EXP_EDP_EBC_NBF && (lpdu[SEC_LPDU_NBF_CONF]&0x03) == EXP_EDP_EBC_NBF)
				{
					return OKAY;
				}
				if(frame_type == EXP_EDP_EBC_NIF && (lpdu[SEC_LPDU_NIF_CONF]&0x03) == EXP_EDP_EBC_NIF)
				{
					return OKAY;
				}
				if(frame_type == EXP_EDP_EBC_NAF && (lpdu[SEC_LPDU_NAF_CONF]&0x03) == EXP_EDP_EBC_NAF)
				{
					return OKAY;
				}
				if(frame_type == EXP_EDP_EBC_NCF && (lpdu[SEC_LPDU_NCF_CONF]&0x03) == EXP_EDP_EBC_NCF)
				{
					return OKAY;
				}
			}
		}
	}
	return FAIL;
}

/*******************************************************************************
* Function Name  : ext_diag_protocol_response(...)
* Description    : called in dll receive process procedure					
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void ext_diag_protocol_response(PHY_RCV_HANDLE pp)
{
	DLL_SEND_STRUCT xdata ds;
	ARRAY_HANDLE lsdu;
	
	ARRAY_HANDLE ppdu;
	ARRAY_HANDLE cond;
	
	u8 conf;
	u8 nbf_id;
	BOOL xdata pass1;
	BOOL xdata pass2;
	BOOL build_id_different;
	u8 window;
	u8 mask;					//[X X buildid mid uid snr ss acPhase]
	u16 rd;
	u8 relation;
	u16 random_id;
	u8 rmode;
	u8 scan;
	u8 delay;

	ppdu = pp->phy_rcv_data;
	conf = ppdu[SEC_LSDU];

	lsdu = OSMemGet(MINOR);
	if(!lsdu)
	{
		return;
	}
	
	if((conf & 0xC0) == EXP_EDP_EBC)			//EBC Protocol
	{
		if((conf&0x03) == EXP_EDP_EBC_NBF)		//NBF Frame
		{
			nbf_id = ppdu[SEC_NBF_ID];
			mask = ppdu[SEC_NBF_MASK];
			cond = &ppdu[SEC_NBF_COND];

			pass1 = FALSE;
			pass2 = FALSE;
			//第一次过滤: 同样uid的节点, 同样的bid号, 同样的build id, 且已经被NCF确认过, 则不响应
			if(!_ebc_confirm[pp->phase])					
			{
				pass1 = TRUE;
			}
			else if((nbf_id & 0xF0)==0)
			{
				pass1 = TRUE;		//2013-05-13 bid=0, 一定回复
			}
			else if((_ebc_response_info[pp->phase][0] & 0xF0) != (nbf_id & 0xF0))		
			{
				pass1 = TRUE;
			}
			else
			{
				if(!mem_cmp(_ebc_caller_uid[pp->phase],&ppdu[SEC_SRC],6))
				{
					pass1 = TRUE;
				}
			}

			/********************************************************************************
			第二次过滤: 2013-04-01 build id不同, 则必须响应
			为了限制所有节点都响应广播, NBF包括一系列限制条件,
			NBF_MASK段指示了这些限制条件的有效性, NBF_COND段是这些限制条件的具体数值
			这些值按NBF_MASK段从低到高的有效性指示, 一个接一个的连续放置(from ac_phase to meterID)
			e.g.: NBF_MASK = 8'b00000101 NBF_COND[] = {0x38, 0x10}
			则表示发起源的AC相位为0x38, 要求SNR大于等于16
			则只有那些接收到此帧, 且与发起源节点同相位, 且接收SNR大于等于16的节点才响应此广播
			NBF_MASK指示位与NBF_COND的数据对应正确性由发送方保证, 接收方不做检查
			********************************************************************************/

			pass2 = TRUE;
			build_id_different = FALSE;
			{
				window = (nbf_id&0x0F);
				rmode = decode_xmode((conf>>2) & 0x0F);
				if((rmode&0x03)==0x00 && window>CFG_EBC_MAX_BPSK_WINDOW)
				{
#ifdef DEBUG_EBC
					my_printf("BPSK WIN LIM 127\r\n");
#endif			
					window = CFG_EBC_MAX_BPSK_WINDOW;
				}
				else if((rmode&0x03)==0x01 && window>CFG_EBC_MAX_DS15_WINDOW)
				{
#ifdef DEBUG_EBC
					my_printf("DS15 WIN LIM 16\r\n");
#endif			
					window = CFG_EBC_MAX_DS15_WINDOW;
				}
				else if((rmode&0x03)==0x02 && window>CFG_EBC_MAX_DS63_WINDOW)
				{
#ifdef DEBUG_EBC
					my_printf("DS63 WIN LIM 8\r\n");
#endif			
					window = CFG_EBC_MAX_DS63_WINDOW;
				}

				if(mask == 0)				// 如无mask, 则任何接到nbf的节点都必须响应
				{
					pass2 = TRUE;
				}
				else
				{
					cond = &ppdu[SEC_NBF_COND];
					if(mask & BIT_NBF_MASK_ACPHASE)
					{
						if(is_zx_valid(pp->phase))				// if zx电路坏损,则不比较相位,直接回复
						{
							relation = phy_acps_compare(pp);
							if(relation != 0)
							{
								pass2 &= 0;
#ifdef DEBUG_EBC
								my_printf("PHASE DIFF\r\n");
#endif
							}
						}
						cond++;
					}
//					if(pass1 && (mask & BIT_NBF_MASK_SS))
//					{
//						//... to do: add more filters
//					}
					if(mask & BIT_NBF_MASK_UID)					// if 指定uid, 则必须uid完全匹配的节点才能回应
					{
						if(check_uid(pp->phase,cond)!=CORRECT)
						{
							pass2 &= 0;
#ifdef DEBUG_EBC
							my_printf("UID DIFF\r\n");
#endif
						}
						cond += 6;
					}

					if(mask & BIT_NBF_MASK_METERID)
					{
						if(check_meter_id((METER_ID_HANDLE)cond)!=CORRECT)
						{
							pass2 &= 0;
#ifdef DEBUG_EBC
							my_printf("MID DIFF\r\n");
#endif
						}
						cond += 6;
					}

					if(mask & BIT_NBF_MASK_BUILDID)
					{
						if((*cond != 0) && (*cond == get_build_id(pp->phase)))	//build_id can't be zero
						{
							pass2 &= 0;			// 2013-01-28 same build id doesn't response;
#ifdef DEBUG_EBC
							my_printf("BUILD ID REPEATED\r\n");
#endif
						}
						else
						{
							build_id_different = TRUE;
						}
						cond++;
					}
				}
			}

			// 通过的条件:
			// if(build_id有效) + 不同的build_id + 满足其他过滤条件;
			// if(build_id无效) + 不同的bid或不同源节点或没确认过 + 满足其他过滤条件
			if((build_id_different&&pass2) || (pass1&&pass2))
			{
				// 生成随机数, 并存储到当前响应数据库中
				rd = rand();
				random_id = (nbf_id&0xF0);
				random_id <<= 8;
				random_id |= (rd & 0x0FFF);
				add_response_db(pp->phase,&ppdu[SEC_SRC], random_id);

				// 准备B-SRF
				ds.phase = pp->phase;
				ds.xmode = decode_xmode((conf>>2) & 0x0F);
				ds.prop = ((pp->phy_rcv_valid & BIT_PHY_RCV_FLAG_SCAN)?BIT_DLL_SEND_PROP_SCAN:0)|BIT_DLL_SEND_PROP_SRF;
				lsdu[0] = random_id>>8;
				lsdu[1] = random_id;
				ds.lsdu = lsdu;
				ds.lsdu_len = 2;

				scan = (pp->phy_rcv_valid & BIT_PHY_RCV_FLAG_SCAN);
				delay = lsdu[1]&(~(0xFF<<window));
#ifdef DEBUG_EBC
				if(debug_ebc_fixed_window==0)
				{
					ds.delay = windows_delay_sticks(4,rmode, scan, 1, delay);
					my_printf("Random:%bu,Delay:%ldms\r\n",delay,ds.delay);
				}
				else
				{
					ds.delay = windows_delay_sticks(4,rmode, scan, 1, debug_ebc_fixed_window);
					my_printf("Random:%bu,Delay:%ldms\r\n",debug_ebc_fixed_window,ds.delay);
				}
#else
				ds.delay = windows_delay_sticks(4,rmode, scan, 1, delay);
#endif
				dll_send(&ds);
			}
		}
		else if((conf&0x03) == EXP_EDP_EBC_NIF)	//NIF Frame
		{
			if(inquire_response_db(pp->phase, &ppdu[SEC_SRC], &ppdu[SEC_NIF_ID]))
			{
				ds.phase = pp->phase;
				ds.xmode = decode_xmode((conf>>2) & 0x0F);
				ds.target_uid_handle = &ppdu[SEC_SRC];
				ds.prop = ((pp->phy_rcv_valid & BIT_PHY_RCV_FLAG_SCAN)?BIT_DLL_SEND_PROP_SCAN:0)|BIT_DLL_SEND_PROP_EDP|BIT_DLL_SEND_PROP_ACK;	//BIT_DLL_SEND_PROP_ACK需要加
				lsdu[0] = EXP_EDP_EBC | EXP_EDP_EBC_NAF;
				lsdu[1] = _ebc_response_info[pp->phase][0];
				lsdu[2] = _ebc_response_info[pp->phase][1];
				ds.lsdu = lsdu;
				ds.lsdu_len = 3;
				ds.delay = ACK_DELAY_STICKS;
				dll_send(&ds);
			}
		}
		else if((conf&0x03) == EXP_EDP_EBC_NCF)	//NCF Frame
		{
			if(inquire_response_db(pp->phase, &ppdu[SEC_SRC], &ppdu[SEC_NCF_ID]))
			{
				confirm_response_db(pp->phase);
#ifdef DEBUG_EBC
				my_printf("NCF Accepted and Confirmed!\r\n");
#endif
			}
		}
	}
	OSMemPut(MINOR,lsdu);
}

#endif
/*******************************************************************************
* Function Name  : get_acps_relation(...)
* Description    : judge phase relationship by xmode, xmt_ac_phase, rcv_ac_phase
* Input          : None
* Output         : None
* Return         : assume local as phase A, return remote:
					0x00: A(same phase);
					0x01: B(later phase);
					0xFF: C(lead phase);
*******************************************************************************/
u8 get_acps_relation(u8 mode, u8 local_ph, u8 remote_ph)
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
	
	// 分支判断
	// 分界算法: round([0:200/6:200]-200/12)
	// [0,17]:A; [18,50]:B^; [51,83]:C; [84,117]:A^; [118,150]B; [151:183]:C^; [184:200]:A
//#ifdef DEBUG_EBC
	my_printf("acps delta:%bu\r\n",delta);
//#endif	
	if(delta<=17)
	{
		return 0x00;
	}
	else if(delta<=50)	
	{
		return 0xFF;
	}
	else if(delta<=83)
	{
		return 0x01;
	}
	else if(delta<=117)
	{
		return 0x00;
	}
	else if(delta<=150)
	{
		return 0xFF;
	}
	else if(delta<=183)
	{
		return 0x01;
	}
	else
	{
		return 0x00;
	}	
}

/*******************************************************************************
* Function Name  : phy_acps_compare()
* Description    : Judge phase relationship in receive package
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
u8 phy_acps_compare(PHY_RCV_STRUCT * phy)
{
    u8 i;
	u8 phy_rcv_valid = phy->phy_rcv_valid;
	
	if((phy_rcv_valid & 0xF0) != 0x00)
	{
		for(i=0;i<4;i++)
		{	
			if(phy_rcv_valid & (0x10<<i))
			{
				return get_acps_relation(phy_rcv_valid&0x03, phy->plc_rcv_acps[i], phy->plc_rcv_buffer[i][SEC_NBF_COND]);	//2012-12-10 correct ACPS should got from plc_rcv_buffer, not phy_rcv_data;
			}
		}
	}
	return 0xFF;
}


/******************* (C) COPYRIGHT 2012 Belling *****END OF FILE****/

