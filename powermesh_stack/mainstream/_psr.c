/******************** (C) COPYRIGHT 2016 ***************************************
* File Name          : psr.c
* Author             : Lv Haifeng
* Version            : V 2.0
* Date               : 2016-05-17
* Description        : 
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
//#include "compile_define.h"
//#include "powermesh_datatype.h"
//#include "powermesh_config.h"
//#include "powermesh_spec.h"
//#include "powermesh_timing.h"
//#include "general.h"
//#include "timer.h"
//#include "mem.h"
//#include "uart.h"
//#include "phy.h"
//#include "dll.h"
//#include "ebc.h"
//#include "addr_db.h"
//#include "psr.h"
//#include "psr_manipulation.h"

/* Private define ------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/


/* Private variables ---------------------------------------------------------*/
PSR_PIPE_RECORD_STRUCT xdata _psr_pipe_record_db[CFG_PSR_PIPE_RECORD_CNT];		
u16 xdata _psr_pipe_record_db_usage;

PSR_RCV_STRUCT xdata _psr_rcv_obj[CFG_PHASE_CNT];
u8 xdata _psr_index[CFG_PHASE_CNT];

/* Private function prototypes -----------------------------------------------*/
void init_psr(void);
BOOL is_pipe_endpoint_uplink(u16 pipe_id);
RESULT check_psr_index(u8 phase, u8 index);

// pipe record operation database
void init_psr_record_db(void);
PSR_PIPE_RECORD_HANDLE update_pipe_record_db(DLL_RCV_HANDLE pd);
PSR_PIPE_RECORD_HANDLE inquire_pipe_record_db(u16 pipe_id);

void psr_rcv_proc(PSR_RCV_HANDLE pn);
BOOL is_pipe_source(u16 pipe_id);

/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : nw_rcv_proc
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void nw_rcv_proc(PSR_RCV_HANDLE pn)
{
	DLL_RCV_HANDLE pd;

	pd = GET_DLL_HANDLE(pn);
	if(pd->dll_rcv_valid && !(pd->dll_rcv_valid & BIT_DLL_SRF))
	{
		if(!(pd->lpdu[SEC_LPDU_DLCT] & BIT_DLCT_DIAG))
		{
			switch(pd->lpdu[SEC_LPDU_PSR_ID]&0xF0)
			{
#ifdef USE_PSR
				case(CST_PSR_PROTOCOL):
				{
					if(!pn->psr_rcv_valid)
					{
						psr_rcv_proc(pn);
					}
					break;
				}
#endif

#ifdef USE_DST
				case(CST_DST_PROTOCOL):
				{
					DST_STACK_RCV_HANDLE pdst;
					pdst = &_dst_rcv_obj[pn->phase];
					if(!pdst->dst_rcv_indication)
					{
						dst_rcv_proc(pd);
					}
					break;
				}
#endif

#ifdef USE_PTP
				case(CST_PTP_PROTOCOL):
				{
					PTP_STACK_RCV_HANDLE pptp;

					pptp = &_ptp_rcv_obj[pn->phase];
					if(!pptp->ptp_rcv_indication)
					{
						ptp_rcv_proc(pd);
					}
					break;
				}
#endif

				default:
				{
#ifdef DEBUG_DISP_INFO
					my_printf("UNKNOWN NW PROTOCOL!\r\n");
					print_phy_rcv(GET_PHY_HANDLE(pn));
#endif
					dll_rcv_resume(pd);
				}
			}
		}
	}
}

#ifdef USE_PSR
/*******************************************************************************
* Function Name  : init_psr
* Description    : Initialize pipe database
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void init_psr(void)
{
	init_psr_record_db();
	mem_clr(&_psr_rcv_obj,sizeof(PSR_RCV_STRUCT),CFG_PHASE_CNT);
#ifdef USE_DST
	mem_clr(&_dst_rcv_obj,sizeof(DST_STACK_RCV_STRUCT),CFG_PHASE_CNT);
#endif
	mem_clr(_psr_index,sizeof(_psr_index),1);

#if NODE_TYPE==NODE_MASTER
	init_psr_mnpl();
#endif
}

/*******************************************************************************
* Function Name  : init_psr_record_db
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void init_psr_record_db(void)
{
	_psr_pipe_record_db_usage = 0;
	mem_clr(_psr_pipe_record_db, sizeof(PSR_PIPE_RECORD_STRUCT), CFG_PSR_PIPE_RECORD_CNT);
}

/*******************************************************************************
* Function Name  : get_psr_index
* Description    : Mantain a auto-increment index as a dll transaction id; add by 1 by each call;
* Input          : 
* Output         : 
* Return         : low 2 bits of the index value;
*******************************************************************************/
u8 get_psr_index(u8 phase)
{
	return ((_psr_index[phase])++) & 0x03;
}

/*******************************************************************************
* Function Name  : check_psr_index
* Description    : 
* Input          : 
* Output         : 
* Return         : CORRECT/WRONG
*******************************************************************************/
//RESULT check_psr_index(u8 phase, u8 index)
//{
//	if(((index+1) & 0x03) == (_psr_index[phase] & 0x03))
//	{
//		return CORRECT;
//	}
//	else
//	{
//		return WRONG;
//	}
//}

/*******************************************************************************
* Function Name  : psr_rcv
* Description    : Check if nw layer receive a packet
* Input          : phase
* Output         : None
* Return         : None
*******************************************************************************/
PSR_RCV_HANDLE psr_rcv()
{
	u8 i;
	
	for(i=0;i<CFG_PHASE_CNT;i++)
	{
		if(_psr_rcv_obj[i].psr_rcv_indication)
		{
			return &_psr_rcv_obj[i];
		}
	}
	return NULL;
}

/*******************************************************************************
* Function Name  : psr_rcv_resume
* Description    : 
* Input          : phase
* Output         : None
* Return         : None
*******************************************************************************/
void psr_rcv_resume(PSR_RCV_HANDLE pn) reentrant
{
	dll_rcv_resume(GET_DLL_HANDLE(pn));
	pn->psr_rcv_valid = 0;
	pn->psr_rcv_indication = 0;
}


/*******************************************************************************
* Function Name  : update_pipe_record_db
* Description    : update the pipe database according to the setup packet.
				Judgement:
				1. if same-id pipe record exists, update the existed pipe record;
				2. if not exists, then:
					1. if the database is full, shift the database above and add the 
					new record into the bottom of the database;
					2. if not full, add the new pipe record to the database
* Input          : setup packet phy address
* Output         : None
* Return         : pipe record address
*******************************************************************************/
PSR_PIPE_RECORD_HANDLE update_pipe_record_db(DLL_RCV_HANDLE pd)
{
	u16 xdata pipe_id;
	u16 i;
	PSR_PIPE_RECORD_HANDLE ppr;
	ADDR_REF_TYPE addr_ref_uplink;
	ADDR_REF_TYPE addr_ref_downlink;
	

	ARRAY_HANDLE lpdu=pd->lpdu;

	// check handle validility
	if(!pd)
	{
		return NULL;
	}

	// check setup
	if(!(lpdu[SEC_LPDU_PSR_CONF] & BIT_PSR_CONF_SETUP) || (lpdu[SEC_LPDU_PSR_CONF] & BIT_PSR_CONF_UPLINK))
	{
		return NULL;
	}

	// 在搜索数据库条目之前先增加地址数据库, 以避免由于地址库溢出造成的条目移动引起索引错误
	addr_ref_uplink = add_addr_db(&lpdu[SEC_LPDU_SRC]);
	if(pd->lpdu_len == LEN_LPCI + 4)
	{
		addr_ref_downlink = addr_ref_uplink;		//if recepient, downlink is uplink
	}
	else
	{
		addr_ref_downlink = add_addr_db(&lpdu[SEC_LPDU_PSR_NSDU+1]);
	}

	// Get the pipe_id in the lpdu
	pipe_id = lpdu[SEC_LPDU_PSR_ID];
	pipe_id = ((pipe_id)<<8) + (lpdu[SEC_LPDU_PSR_ID2]);
	pipe_id &= 0x0FFF;

	// Search the pipe_id in the data_base
	for(i = 0; i<_psr_pipe_record_db_usage; i++)
	{
		if((_psr_pipe_record_db[i].pipe_info & 0x0FFF) == pipe_id)
		{
			_psr_pipe_record_db_usage--;			// update old record doesn't increase the usage
			break;
		}
	}

	// if search returns 0, shift the database above and put the result in the last one;
	if(i == CFG_PSR_PIPE_RECORD_CNT)
	{
		// 如果存储空间已满, 所有的pipe record向下移动, 并把新的pipe信息加到队尾
		for(i=0;i<CFG_PSR_PIPE_RECORD_CNT-1;i++)
		{
			mem_cpy((ARRAY_HANDLE)(&_psr_pipe_record_db[i]), (ARRAY_HANDLE)(&_psr_pipe_record_db[i+1]), sizeof(PSR_PIPE_RECORD_STRUCT));
		}
	}
	else
	{
		_psr_pipe_record_db_usage++;
	}

	// i is the index that new record can be putted into
	ppr = &_psr_pipe_record_db[i];
	ppr->pipe_info = (pipe_id & 0x0FFF);					// restore pipe_id

	if(lpdu[SEC_LPDU_PSR_CONF] & BIT_PSR_CONF_BIWAY)		
	{
		ppr->pipe_info |= BIT_PSR_PIPE_INFO_BIWAY;			// mark the biway property on the highest bit of pipe_info;

		/*** recepient of the pipe***/
		if(pd->lpdu_len == LEN_LPCI + 4)					// nsdu of recipient is only 1;
		{
			ppr->pipe_info |= (BIT_PSR_PIPE_INFO_ENDPOINT|BIT_PSR_PIPE_INFO_UPLINK);	// mark recipient if lpdu_len is 4
		}

		ppr->uplink_next = addr_ref_uplink;
		ppr->downlink_next = addr_ref_downlink;
		update_addr_db(addr_ref_uplink, decode_xmode(lpdu[SEC_LPDU_PSR_NSDU] & 0x0F), decode_xmode(lpdu[SEC_LPDU_PSR_NSDU] >> 4));
		
		if(!(ppr->pipe_info & BIT_PSR_PIPE_INFO_ENDPOINT))	// 
		{
			update_addr_db(addr_ref_downlink, decode_xmode(lpdu[SEC_LPDU_PSR_NSDU+7] >> 4), decode_xmode(lpdu[SEC_LPDU_PSR_NSDU+7] & 0x0F));
		}
	}
	else
	{
		// todo: SINGLE-WAY pipe setup process.
	}
	return ppr;
}

/*******************************************************************************
* Function Name  : inquire_pipe_record_db
* Description    : inquire the local pipe database and check whether the pipe_id pipe info is in the database.
					If it exists, return the address of the pipe record; otherwise, return null;
* Input          : pipe_id
* Output         : 
* Return         : pipe record address
*******************************************************************************/
PSR_PIPE_RECORD_HANDLE inquire_pipe_record_db(u16 pipe_id)
{
	u16 i;
	
	for(i=0;i<_psr_pipe_record_db_usage;i++)
	{
		if((_psr_pipe_record_db[i].pipe_info & 0x0FFF) == pipe_id)
		{
			return &_psr_pipe_record_db[i];
		}
	}
	return NULL;
}

///********************************************************************************
//* Function Name  : is_pipe_biway
//* Description    : Judge whether the pipe is biway.
//* Input          : pipe record address
//* Output         : 
//* Return         : TRUE/FALSE
//********************************************************************************/
//BOOL is_pipe_biway(PSR_PIPE_RECORD_HANDLE ref)
//{
//	if(ref->pipe_info & 0x8000)
//	{
//		return TRUE;
//	}
//	else
//	{
//		return FALSE;
//	}
//}

/********************************************************************************
* Function Name  : is_pipe_endpoint
* Description    : Judge self node is the recipient or not.
* Input          : pipe record address
* Output         : 
* Return         : TRUE/FALSE
********************************************************************************/
BOOL is_pipe_endpoint(u16 pipe_id)
{
	PSR_PIPE_RECORD_HANDLE handle;

	handle = inquire_pipe_record_db(pipe_id);
	if(handle)
	{
		return (handle->pipe_info & BIT_PSR_PIPE_INFO_ENDPOINT) ? TRUE : FALSE;
	}
	return FALSE;
}

/********************************************************************************
* Function Name  : is_pipe_endpoint_uplink
* Description    : if the psr direction is downlink by PSR_CONF byte.
* Input          : 
* Output         : 
* Return         : TRUE/FALSE
********************************************************************************/
BOOL is_pipe_endpoint_uplink(u16 pipe_id)
{
	PSR_PIPE_RECORD_HANDLE handle;

	handle = inquire_pipe_record_db(pipe_id);
	if(handle)
	{
		return (handle->pipe_info & BIT_PSR_PIPE_INFO_UPLINK)?TRUE:FALSE;
	}
	return FALSE;
}



/*******************************************************************************
* Function Name  : psr_send
* Description    : 用于psr_rcv_proc, 以及管道两端, 转发, 不用于psr_setup
					本地需要准备一个dll发送结构的数据体, 由于中断调用, 因此不能使用xdata, 选择使用动态内存;
					如果传入的ppss的数据长度为空(例如psr setup的回复包), 需要本地准备数据缓冲
					2014-11-07 返回类型改成SEND_ID_TYPE便于查询发送状态
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
//PSR_ERROR_TYPE psr_send(PSR_SEND_HANDLE ppss) reentrant
//{
//	DLL_SEND_HANDLE p_dll_send;
//	PSR_PIPE_RECORD_HANDLE pipe_ref;
//	ADDR_REF_TYPE addr_ref;
//	ARRAY_HANDLE npdu;
//	u8 temp;
//	u8 dynamic_mem = FALSE;


//	/* Check ppss validility */
//	pipe_ref = inquire_pipe_record_db(ppss->pipe_id);
//	if(!pipe_ref)
//	{
//#ifdef DEBUG_PSR
//		my_printf("no pipe info.\r\n");
//#endif
//		return NO_PIPE_INFO;
//	}

//	/* Check address validility */
//	addr_ref = (ppss->uplink) ? pipe_ref->uplink_next : pipe_ref->downlink_next;
//	if(!check_addr_ref_validity(addr_ref))
//	{
//#ifdef DEBUG_PSR
//		my_printf("invalid addr ref.\r\n");
//#endif
//		return UNKNOWN_ERR;
//	}

//	/* resource application */
//	p_dll_send = OSMemGet(MINOR);
//	if(!p_dll_send)
//	{
//		return MEM_OUT;
//	}

//	if(ppss->nsdu_len == 0)
//	{
//		npdu = OSMemGet(MINOR);
//		dynamic_mem = TRUE;
//		if(!npdu)
//		{
//			OSMemPut(MINOR,p_dll_send);
//			return MEM_OUT;
//		}
//	}
//	else
//	{
//		npdu = ppss->nsdu;
//	}	

//	mem_shift(npdu, ppss->nsdu_len, LEN_NPCI);
//	npdu[SEC_NPDU_PSR_ID] = ((ppss->pipe_id)>>8) | CST_PSR_PROTOCOL;
//	npdu[SEC_NPDU_PSR_ID2] = (u8)(ppss->pipe_id);
//	//npdu[SEC_NPDU_PSR_CONF] = BIT_PSR_CONF_BIWAY;
//	//npdu[SEC_NPDU_PSR_CONF] |= (ppss->prop&BIT_PSR_SEND_PROP_SETUP)?BIT_PSR_CONF_SETUP:0;
//	//npdu[SEC_NPDU_PSR_CONF] |= (ppss->prop&BIT_PSR_SEND_PROP_PATROL)?BIT_PSR_CONF_PATROL:0;
//	//npdu[SEC_NPDU_PSR_CONF] |= (ppss->prop&BIT_PSR_SEND_PROP_ERROR)?BIT_PSR_CONF_ERROR:0;
//	//npdu[SEC_NPDU_PSR_CONF] |= ppss->uplink?BIT_PSR_CONF_UPLINK:0;
//	//npdu[SEC_NPDU_PSR_CONF] |= ppss->index & BIT_PSR_CONF_INDEX;

//	temp = BIT_PSR_CONF_BIWAY|(ppss->prop&(BIT_PSR_SEND_PROP_SETUP|BIT_PSR_SEND_PROP_PATROL|BIT_PSR_SEND_PROP_ERROR));
//	temp |= ppss->uplink?BIT_PSR_CONF_UPLINK:0;
//	temp |= ppss->index & BIT_PSR_CONF_INDEX;
//	npdu[SEC_NPDU_PSR_CONF] = temp;
//	

//	mem_clr(p_dll_send,sizeof(DLL_SEND_STRUCT),1);
//	p_dll_send->phase = ppss->phase;
//	p_dll_send->prop = 0;
//	p_dll_send->target_uid_handle = inquire_addr_db(addr_ref);
//	p_dll_send->xmode = inquire_addr_db_xmode(addr_ref);
//	p_dll_send->lsdu = npdu;
//	p_dll_send->lsdu_len = ppss->nsdu_len + LEN_NPCI;
//	p_dll_send->delay = DLL_SEND_DELAY_STICKS;
//	dll_send(p_dll_send);

//	OSMemPut(MINOR,p_dll_send);
//	if(dynamic_mem)
//	{
//		OSMemPut(MINOR,npdu);
//	}
//	
//	return NO_ERROR;
//}

SEND_ID_TYPE psr_send(PSR_SEND_HANDLE ppss) reentrant
{
	DLL_SEND_HANDLE p_dll_send;
	PSR_PIPE_RECORD_HANDLE pipe_ref;
	ADDR_REF_TYPE addr_ref;
	ARRAY_HANDLE npdu;
	u8 temp;
	u8 dynamic_mem = FALSE;
	SEND_ID_TYPE send_id = INVALID_RESOURCE_ID;


	/* Check ppss validility */
	pipe_ref = inquire_pipe_record_db(ppss->pipe_id);
	if(!pipe_ref)
	{
#ifdef DEBUG_PSR
		my_printf("no pipe info.\r\n");
#endif
		return INVALID_RESOURCE_ID;
	}

	/* Check address validility */
	addr_ref = (ppss->uplink) ? pipe_ref->uplink_next : pipe_ref->downlink_next;
	if(!check_addr_ref_validity(addr_ref))
	{
#ifdef DEBUG_PSR
		my_printf("invalid addr ref.\r\n");
#endif
		return INVALID_RESOURCE_ID;
	}

	/* resource application */
	p_dll_send = OSMemGet(MINOR);
	if(!p_dll_send)
	{
		return INVALID_RESOURCE_ID;
	}

	if(ppss->nsdu_len == 0)
	{
		npdu = OSMemGet(MINOR);
		dynamic_mem = TRUE;
		if(!npdu)
		{
			OSMemPut(MINOR,p_dll_send);
			return INVALID_RESOURCE_ID;
		}
	}
	else
	{
		npdu = ppss->nsdu;
	}	

	mem_shift(npdu, ppss->nsdu_len, LEN_NPCI);
	npdu[SEC_NPDU_PSR_ID] = ((ppss->pipe_id)>>8) | CST_PSR_PROTOCOL;
	npdu[SEC_NPDU_PSR_ID2] = (u8)(ppss->pipe_id);
	//npdu[SEC_NPDU_PSR_CONF] = BIT_PSR_CONF_BIWAY;
	//npdu[SEC_NPDU_PSR_CONF] |= (ppss->prop&BIT_PSR_SEND_PROP_SETUP)?BIT_PSR_CONF_SETUP:0;
	//npdu[SEC_NPDU_PSR_CONF] |= (ppss->prop&BIT_PSR_SEND_PROP_PATROL)?BIT_PSR_CONF_PATROL:0;
	//npdu[SEC_NPDU_PSR_CONF] |= (ppss->prop&BIT_PSR_SEND_PROP_ERROR)?BIT_PSR_CONF_ERROR:0;
	//npdu[SEC_NPDU_PSR_CONF] |= ppss->uplink?BIT_PSR_CONF_UPLINK:0;
	//npdu[SEC_NPDU_PSR_CONF] |= ppss->index & BIT_PSR_CONF_INDEX;

	temp = BIT_PSR_CONF_BIWAY|(ppss->prop&(BIT_PSR_SEND_PROP_SETUP|BIT_PSR_SEND_PROP_PATROL|BIT_PSR_SEND_PROP_ERROR));
	temp |= ppss->uplink?BIT_PSR_CONF_UPLINK:0;
	temp |= ppss->index & BIT_PSR_CONF_INDEX;
	npdu[SEC_NPDU_PSR_CONF] = temp;
	

	mem_clr(p_dll_send,sizeof(DLL_SEND_STRUCT),1);
	p_dll_send->phase = ppss->phase;
	p_dll_send->prop = 0;
	p_dll_send->target_uid_handle = inquire_addr_db(addr_ref);
	p_dll_send->xmode = inquire_addr_db_xmode(addr_ref);
	p_dll_send->lsdu = npdu;
	p_dll_send->lsdu_len = ppss->nsdu_len + LEN_NPCI;
	p_dll_send->delay = DLL_SEND_DELAY_STICKS;
	send_id = dll_send(p_dll_send);

	OSMemPut(MINOR,p_dll_send);
	if(dynamic_mem)
	{
		OSMemPut(MINOR,npdu);
	}
	
	return send_id;
}






/*******************************************************************************
* Function Name  : psr_rcv_proc
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void psr_rcv_proc(PSR_RCV_HANDLE pn)
{
	u16 xdata pipe_id;
	u8  conf;
	PSR_PIPE_RECORD_HANDLE pipe_ref;
	ARRAY_HANDLE xdata nsdu;
	PSR_SEND_HANDLE xdata pps;
	DLL_RCV_HANDLE xdata pd;


	/*** Req Resources ***/
	pps = OSMemGet(MINOR);							// apply for PSR_SEND_STRUCT
	if(!pps)
	{
		return;
	}
	else
	{
		mem_clr(pps,CFG_MEM_MINOR_BLOCK_SIZE,1);
	}

	nsdu = OSMemGet(SUPERIOR);
	if(!nsdu)
	{
		OSMemPut(MINOR,pps);
		return;
	}

	/*** Get Pipe id and conf info of the psr packet***/
	pd = GET_DLL_HANDLE(pn);
	pipe_id = pd->lpdu[SEC_LPDU_PSR_ID];
	pipe_id = (pipe_id<<8) + (pd->lpdu[SEC_LPDU_PSR_ID2]);
	pipe_id &= 0x0FFF;
	conf = pd->lpdu[SEC_LPDU_PSR_CONF];
	
	pps->phase = pd->phase;
	pps->pipe_id = pipe_id;

	/* Setup Processing */
	//if((conf & BIT_PSR_CONF_SETUP) && (conf & BIT_PSR_CONF_BIWAY) &&  !(conf & BIT_PSR_CONF_ERROR) && !(conf & BIT_PSR_CONF_UPLINK))		// psr setup could generate error feedback, if it is EF, just process it as a regular packet
	if((conf & (BIT_PSR_CONF_SETUP|BIT_PSR_CONF_BIWAY|BIT_PSR_CONF_ERROR|BIT_PSR_CONF_UPLINK))==(BIT_PSR_CONF_SETUP|BIT_PSR_CONF_BIWAY))		// 精简写法
	{
		/* update local pipe record db */
		pipe_ref = update_pipe_record_db(pd);
		
		if(!pipe_ref)
		{
#ifdef DEBUG_PSR
			my_printf("PSR record add error.\r\n");
#endif
			dll_rcv_resume(pd);
		}
//		else if(!(conf & BIT_PSR_CONF_BIWAY))		//无效的比较, 精简之
//		{
//#ifdef DEBUG_PSR
//			my_printf("one_way setup.\r\n");
//#endif
//			dll_rcv_resume(pd);
//		}
		else
		{
			if(pipe_ref->pipe_info & BIT_PSR_PIPE_INFO_ENDPOINT) // if setup, self_node is the recepient, reply a regular empty packet as affirmative
			{
#ifdef DEBUG_PSR
				my_printf("AFFM SETUP\r\n");
#endif
				//pps->phase = pd->phase;
				//pps->pipe_id = pipe_id;
				pps->prop = 0;
				pps->index = conf & 0x03;
				pps->uplink = 1;
				pps->nsdu_len = 0;
				psr_send(pps);
			}
			else	/*** if setup, this is not the recipent, add into pipe and forward the packet ***/
			{
#ifdef DEBUG_PSR
				my_printf("FORW SETUP\r\n");
#endif
				//pps->phase = pd->phase;
				//pps->pipe_id = pipe_id;
				pps->prop = BIT_PSR_SEND_PROP_SETUP;
				pps->index = conf & 0x03;
				pps->uplink = conf & BIT_PSR_CONF_UPLINK; 
				pps->nsdu = &pd->lpdu[SEC_LPDU_PSR_NSDU+7];
				pps->nsdu_len = pd->lpdu_len-LEN_LPCI-LEN_NPCI-7;
				psr_send(pps);
			}
		}
	}
	/*** Pipe Patrol Proc***/ //Patrol格式: 本节点UID + SS + SNR + XMODE
	else if(conf & BIT_PSR_CONF_PATROL)
	{
		pipe_ref = inquire_pipe_record_db(pipe_id);
		if(pipe_ref)
		{
			BASE_LEN_TYPE nsdu_len;
			ARRAY_HANDLE ptw;
			u8 i;
			PHY_RCV_HANDLE pp;
			ADDR_REF_TYPE addr;
			
#ifdef DEBUG_PSR
			my_printf("PSR PATROL\r\n");
#endif
			// 如果是pipe发起者收到patrol包, 上报
			if((pipe_ref->pipe_info & BIT_PSR_PIPE_INFO_ENDPOINT) && !(pipe_ref->pipe_info & BIT_PSR_PIPE_INFO_UPLINK))
			{
#if NODE_TYPE==NODE_MASTER							//节省MT空间
				pn->phase = pd->phase;
				pn->pipe_id = pipe_id;
#endif
				pn->psr_rcv_valid = conf | BIT_PSR_RCV_VALID | BIT_PSR_RCV_PATROL;
				pn->npdu = &pd->lpdu[SEC_LPDU_PSR_NPDU];
				pn->npdu_len = pd->lpdu_len - LEN_LPCI;
				pn->psr_rcv_indication = 1;			// 需主线程处理.
			}
			else
			{
				nsdu_len = pd->lpdu_len-LEN_LPCI-LEN_NPCI;
				if(nsdu_len+9 < CFG_MEM_SUPERIOR_BLOCK_SIZE)
				{
					mem_cpy(nsdu,&pd->lpdu[SEC_LPDU_PSR_NSDU],nsdu_len);	// copy original NSDU
					ptw = nsdu+nsdu_len;

					// append format [uid ss snr xmode]
					get_uid(pd->phase,ptw);
					ptw += 6;
					nsdu_len += 6;

					// append ss snr
					pp = GET_PHY_HANDLE(pn);
					for(i=0;i<4;i++)
					{
						if((pp->phy_rcv_supposed_ch) & (0x10<<i))
						{
							*(ptw++) = pp->phy_rcv_ss[i];
							*(ptw++) = pp->phy_rcv_ebn0[i];
							nsdu_len += 2;
						}
					}

					// append xmode
					if(pipe_ref->pipe_info & BIT_PSR_PIPE_INFO_ENDPOINT)
					{
						addr = pipe_ref->uplink_next;
						pps->uplink = 1;
					}
					else
					{
						addr = (conf&BIT_PSR_CONF_UPLINK)?(pipe_ref->uplink_next):(pipe_ref->downlink_next);
						pps->uplink = conf & BIT_PSR_CONF_UPLINK;
					}
					*(ptw++) = inquire_addr_db_xmode(addr);
					nsdu_len += 1;

					// send
					//pps->phase = pd->phase;
					//pps->pipe_id = pipe_id;
					pps->prop = BIT_PSR_SEND_PROP_PATROL;
					pps->index = conf & 0x03;
					pps->nsdu = nsdu;
					pps->nsdu_len = nsdu_len;
					psr_send(pps);
				}
			}
		}
	}

	/*** Normal Pipe Transmit Proc***/
	else		
	{
		pipe_ref = inquire_pipe_record_db(pipe_id);

		// 如果pipe_info为空:
		// 1. 如果是下行, 如果存在上一节点的通信方式, 返回错误提示包.适用于多条路径共存于一个节点且出现pipe_info被冲刷掉的情况(目前节点不支持此上行报告)
		// 2. 如果方向是上行, 向上层对象汇报(可能是Setup完成提示)
		if(!pipe_ref)
		{
			if(!(conf & BIT_PSR_CONF_UPLINK) && (pd->lpdu[SEC_LPDU_MPDU]&0x80))		// 如果本地无info信息, 如果下行, 如果存在上一级节点通信方式, 返回错误报告
			{																		// 目前仅支持mgnt信息的error feedback
#if NODE_TYPE==NODE_MASTER
				DLL_SEND_HANDLE pds;				// 因为本地不存在pipe info, 无法调用psr_send方法; 只能调用dll_send方法
				ARRAY_HANDLE lsdu;
				ADDR_REF_TYPE addr_ref;
#ifdef DEBUG_PSR
				my_printf("no local pipe info");
#endif
				addr_ref = inquire_addr_db_by_uid(&pd->lpdu[SEC_LPDU_SRC]);
				if(addr_ref != INVALID_RESOURCE_ID)
				{
#ifdef DEBUG_PSR
					my_printf("send error feedback.");
#endif

					pds = (DLL_SEND_HANDLE)(pps);
					lsdu = nsdu;
					lsdu[SEC_LSDU_PSR_ID] = (pipe_id>>8)| CST_PSR_PROTOCOL;
					lsdu[SEC_LSDU_PSR_ID2] = (u8)(pipe_id);
					lsdu[SEC_LSDU_PSR_CONF] = (conf | BIT_PSR_CONF_ERROR) ^ BIT_PSR_CONF_UPLINK;
					lsdu[SEC_LSDU_PSR_NSDU] = pd->lpdu[SEC_LPDU_MPDU] | 0xC0;		//目前仅对mgnt有返回error report机制
					get_uid(pd->phase,&lsdu[SEC_LSDU_PSR_NSDU+1]);
					lsdu[SEC_LSDU_PSR_NSDU+7] = NO_PIPE_INFO;

					////精简写法
					//*ptw++ = (pipe_id>>8)| CST_PSR_PROTOCOL;
					//*ptw++ = (u8)(pipe_id);
					//*ptw++ = (conf | BIT_PSR_CONF_ERROR) ^ BIT_PSR_CONF_UPLINK;
					//*ptw++ = pd->lpdu[SEC_LPDU_MPDU] | 0xC0;
					//get_uid(pd->phase,ptw);
					//ptw += 6;
					//*ptw++ = NO_PIPE_INFO;
					
					pds->phase = pd->phase;
					pds->target_uid_handle = &pd->lpdu[SEC_LPDU_SRC];
					pds->xmode = inquire_addr_db_xmode(addr_ref);
					pds->prop = 0;
					pds->delay = DLL_SEND_DELAY_STICKS;
					pds->lsdu = lsdu;
					pds->lsdu_len = 11;
					dll_send(pds);
				}
#endif
			}
			else					// if uplink and no_pipe_info, report it to nw layer(maybe setup affm)
			{
#ifdef DEBUG_PSR
				my_printf("NO PIPE INFO\r\n");
#endif
				if(pd->lpdu_len == (LEN_LPCI + LEN_NPCI))
				{
					pn->phase = pd->phase;
					pn->psr_rcv_valid = conf | BIT_PSR_RCV_VALID;
					pn->pipe_id = pipe_id;
					pn->npdu = &pd->lpdu[SEC_LPDU_PSR_NPDU];
					pn->npdu_len = pd->lpdu_len - LEN_LPCI;
					pn->psr_rcv_indication = 1;			// 需主线程处理.
				}
			}
		}
		else
		{
			if(is_pipe_endpoint(pipe_id))			// if endpoint, report it.
			{
#ifdef DEBUG_PSR
				my_printf("GOT PSR RECEPIENT\r\n");
#endif
				pn->phase = pd->phase;
				pn->psr_rcv_valid = (conf | BIT_PSR_RCV_VALID);
				pn->pipe_id = pipe_id;
				pn->npdu = &pd->lpdu[SEC_LPDU_PSR_NPDU];
				pn->npdu_len = pd->lpdu_len - LEN_LPCI;
				if(pn->npdu_len == LEN_NPCI)
				{
					pn->psr_rcv_indication = 1;		// 如果MPDU长度为0, 需主线程处理(PSR AFFM)
				}
			}
			else									// if not endpoint, forward it.
			{
				//pps->phase = pd->phase;
				//pps->pipe_id = pipe_id;
				pps->prop = (conf&BIT_PSR_CONF_ERROR)?BIT_PSR_SEND_PROP_ERROR:0;
				pps->index = conf & 0x03;
				pps->uplink = conf & BIT_PSR_CONF_UPLINK;
				pps->nsdu = &pd->lpdu[SEC_LPDU_PSR_NSDU];
				pps->nsdu_len = pd->lpdu_len-LEN_LPCI-LEN_NPCI;
				psr_send(pps);
			}
		}
	}

	if(!pn->psr_rcv_valid)
	{
#ifdef DEBUG_PSR
//		my_printf("release a psr stack package\r\n");
#endif
		dll_rcv_resume(pd);
	}
	OSMemPut(MINOR,pps);
	OSMemPut(SUPERIOR,nsdu);
}

/*******************************************************************************
* Function Name  : notify_expired_addr
* Description    : 当addr管理库有addr被删除时, 通知所有引用addr的对象, 对引用了过期addr索引的数据条目进行处理
					以免发生错误.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void notify_psr_record_db_expired_addr(ADDR_REF_TYPE expired_addr)
{
	u16 i,j;
	
	for(i=0;i<_psr_pipe_record_db_usage;i++)
	{
		if(_psr_pipe_record_db[i].downlink_next==expired_addr || _psr_pipe_record_db[i].uplink_next==expired_addr)
		{
			_psr_pipe_record_db_usage--;
			for(j=i;j<_psr_pipe_record_db_usage;j++)
			{
				_psr_pipe_record_db[j].pipe_info = _psr_pipe_record_db[j+1].pipe_info;
				_psr_pipe_record_db[j].downlink_next = _psr_pipe_record_db[j+1].downlink_next;
				_psr_pipe_record_db[j].uplink_next = _psr_pipe_record_db[j+1].uplink_next;
			}
		}
	}
}





/*******************************************************************************
* Function Name  : notify_expired_addr
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void notify_psr_expired_addr(ADDR_REF_TYPE expired_addr)
{
	notify_psr_record_db_expired_addr(expired_addr);
#if NODE_TYPE==NODE_MASTER
	notify_psr_mnpl_db_expired_addr(expired_addr);
#endif
}
#endif
/******************** (C) COPYRIGHT 2016 ***************************************/

