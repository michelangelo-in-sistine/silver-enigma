/******************** (C) COPYRIGHT 2012 Belling Inc. ********************
* File Name          : psr_manipulation.c
* Author             : Lv Haifeng
* Version            : V2.0
* Date               : 2013-03-18
* Description        : CC Only
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
#if NODE_TYPE==NODE_MASTER
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
//#include "mgnt_app.h"
/* Private define ------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/

/* Extern variables -------------------------------------------------------------*/
extern PSR_PIPE_RECORD_STRUCT xdata _psr_pipe_record_db[];
extern u16 xdata _psr_pipe_record_db_usage;


/* Private variables ---------------------------------------------------------*/
PIPE_MNPL_STRUCT psr_pipe_mnpl_db[CFG_PSR_PIPE_MNPL_CNT];
u16	xdata psr_pipe_mnpl_db_usage;

/* Private function prototypes -----------------------------------------------*/
STATUS update_pipe_mnpl_db(u8 phase, u16 pipe_id, u8 * script, u16 script_len);
void psr_transaction_clear_irrelevant(void);

/* Private functions ---------------------------------------------------------*/

/*********************** Belows Function are only available in CC *************/
/*******************************************************************************
* Function Name  : init_psr_mnpl
* Description    : CC PSR管理功能初始化
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void init_psr_mnpl(void)
{
	psr_pipe_mnpl_db_usage = 0;
	mem_clr(psr_pipe_mnpl_db,sizeof(PIPE_MNPL_STRUCT),CFG_PSR_PIPE_MNPL_CNT);
}

/*******************************************************************************
* Function Name  : reset_psr_mnpl
* Description    : CC PSR管理功能重置, 用于组网
					目前的做法是比较相位, 将相位相同的记录覆盖
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void reset_psr_mnpl(u8 phase)
{
	u16 i,j;
	for(i=0;i<psr_pipe_mnpl_db_usage;i++)
	{
		if(psr_pipe_mnpl_db[i].phase == phase)
		{
			for(j=i;j<psr_pipe_mnpl_db_usage-1;j++)
			{
				mem_cpy((ARRAY_HANDLE)(&psr_pipe_mnpl_db[j]),(ARRAY_HANDLE)(&psr_pipe_mnpl_db[j+1]),sizeof(PIPE_MNPL_STRUCT));
			}
			i--;
			psr_pipe_mnpl_db_usage--;
		}
	}
}

/*******************************************************************************
* Function Name  : psr_setup
* Description    : 
*					
* Input          : script format: "uid[6] downmode upmode/uid[6] downmode upmode/..."
* Output         : 
* Return         : 
*******************************************************************************/
STATUS psr_setup(u8 phase, u16 pipe_id, ARRAY_HANDLE script, u16 script_len)
{
	TIMER_ID_TYPE tid;
	ARRAY_HANDLE npdu;
	ARRAY_HANDLE npdu_duplicate;
	ARRAY_HANDLE ptr;
	ARRAY_HANDLE ptw;
	DLL_SEND_STRUCT dss;
	u32  expiring_sticks;
	STATUS status = FAIL;
	SEND_ID_TYPE sid;
	u16 len;
	u8 i;

	/* check sript format */
	if(script_len<8)
	{
#ifdef DEBUG_MANIPULATION
		my_printf("err:pipe script length not enough.\r\n");
#endif
		return FAIL;
	}

	if(((script_len>>3)<<3) != script_len)		//script_len必须是8的整倍数
	{
#ifdef DEBUG_MANIPULATION
		my_printf("err:pipe info format error.\r\n");
#endif
		return FAIL;
	}
	
	if(pipe_id > 0x0FFF)
	{
#ifdef DEBUG_MANIPULATION
		my_printf("err:pipe id is limited to 12 bits.\r\n");
#endif
		return FAIL;
	}

	/* req resources */
	tid = req_timer();
	if(tid == INVALID_RESOURCE_ID)
	{
		return FAIL;
	}

	npdu = OSMemGet(SUPERIOR);
	if(!npdu)
	{
		delete_timer(tid);
		return FAIL;
	}

	npdu_duplicate = OSMemGet(SUPERIOR);
	if(!npdu_duplicate)
	{
		delete_timer(tid);
		OSMemPut(SUPERIOR,npdu);
		return FAIL;
	}

	powermesh_main_thread_useless_resp_clear();

	/* fill DLL send Struct for PSR setup and send it*/
	dss.phase = phase;
	dss.prop = 0;
	dss.target_uid_handle = &script[0];
	dss.xmode = script[6];
	dss.rmode = script[7];

	//填NPCI
	npdu[SEC_NPDU_PSR_ID] = (u8)(pipe_id>>8) | CST_PSR_PROTOCOL;
	npdu[SEC_NPDU_PSR_ID2] = (u8)(pipe_id);
	npdu[SEC_NPDU_PSR_CONF] = BIT_PSR_CONF_SETUP | BIT_PSR_CONF_BIWAY | get_psr_index(phase);

	//填NSDU首字节,是第一个节点的下行模式和上行模式
	ptr = &script[6];
	ptw = &npdu[SEC_NPDU_PSR_NSDU];
	*ptw = encode_xmode(*ptr++,0);
	*ptw <<= 4;
	*(ptw++) |= encode_xmode(*ptr++,0);
	
	dss.lsdu = npdu;
	dss.delay = DLL_SEND_DELAY_STICKS;
	dss.lsdu_len = 4;
	
	//填NSDU剩余字节
	for(len=script_len-8; len>=8;  len-=8)
	{
		mem_cpy(ptw,ptr,6);					// fill address
		ptr += 6;
		ptw += 6;

		*ptw = encode_xmode(*ptr++,0);		// fill uplink & downlink plan
		*ptw <<= 4;
		*(ptw++) |= encode_xmode(*ptr++,0);
		dss.lsdu_len += 7;
	}

	//等待时间
	expiring_sticks = psr_setup_expiring_sticks(script,script_len);
	mem_cpy(npdu_duplicate,npdu,dss.lsdu_len);			//backup
	//启动发送
	for(i=0;i<CFG_MAX_SETUP_PIPE_TRY;i++)
	{
		SET_BREAK_THROUGH("quit psr_setup()\r\n");

		mem_cpy(npdu,npdu_duplicate,dss.lsdu_len);		//dll_send will modify dss

		sid = dll_send(&dss);
#ifdef USE_MAC
		if(sid != INVALID_RESOURCE_ID)
		{
			declare_channel_occupation(sid,expiring_sticks);
		}
#endif
		status = wait_until_send_over(sid);
		if(!status)
		{
			delete_timer(tid);
			OSMemPut(SUPERIOR,npdu);
			OSMemPut(SUPERIOR,npdu_duplicate);
			return status;
		}

		status = FAIL;
		set_timer(tid,expiring_sticks);
		do
		{
			PSR_RCV_HANDLE ps;

			SET_BREAK_THROUGH("quit psr_setup()\r\n");
			
			ps = psr_rcv();
			if(ps)
			{
				if((ps->psr_rcv_valid & BIT_PSR_RCV_UPLINK) && (ps->pipe_id == pipe_id))
				{
					pause_timer(tid);
#ifdef DEBUG_MANIPULATION	
					my_printf("GOT AFFM.");
					print_transaction_timing(tid,expiring_sticks);
#endif
					update_pipe_mnpl_db(phase, pipe_id, script, script_len);
					status = OKAY;
				}
				else
				{
#ifdef DEBUG_MANIPULATION	
					my_printf("GOT A WRONG AFFM.");
					print_phy_rcv(GET_PHY_HANDLE(ps));
#endif
				}
				psr_rcv_resume(ps);
			}
			FEED_DOG();
		}while(!is_timer_over(tid) && !status);
		
		if(status)
		{
			break;
		}

	}
	OSMemPut(SUPERIOR,npdu);
	OSMemPut(SUPERIOR,npdu_duplicate);
	delete_timer(tid);
	return status;
}

/*******************************************************************************
* Function Name  : psr_patrol()
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
u16 psr_patrol(u8 phase, u16 pipe_id, ARRAY_HANDLE return_buffer)
{
	TIMER_ID_TYPE tid;
	ARRAY_HANDLE npdu;
	PIPE_REF pipe_ref;
	DLL_SEND_STRUCT dss;
	ADDR_REF_TYPE addr_ref;
	SEND_ID_TYPE sid;
	u32 expiring_sticks;
	STATUS status;
	u16 return_len;
	PHY_RCV_HANDLE pp;

	pipe_ref = inquire_pipe_mnpl_db(pipe_id);
	if(!pipe_ref)
	{
#ifdef DEBUG_MANIPULATION
		my_printf("pipe info not exist\r\n");
#endif
		return 0;
	}

	/* req resources */
	tid = req_timer();
	if(tid == INVALID_RESOURCE_ID)
	{
		return 0;
	}

	npdu = OSMemGet(MINOR);
	if(!npdu)
	{
		delete_timer(tid);
		return 0;
	}

	/* send patrol packet */
	npdu[SEC_NPDU_PSR_ID] = ((pipe_id)>>8) | CST_PSR_PROTOCOL;
	npdu[SEC_NPDU_PSR_ID2] = (u8)(pipe_id);
	npdu[SEC_NPDU_PSR_CONF] = BIT_PSR_CONF_BIWAY | BIT_PSR_CONF_PATROL;
	npdu[SEC_NPDU_PSR_CONF] |= get_psr_index(phase);

	addr_ref = pipe_ref->way_uid[0];
	dss.phase = phase;
	dss.target_uid_handle = inquire_addr_db(addr_ref);
	dss.lsdu = npdu;
	dss.lsdu_len = 3;
	dss.prop = 0;
	dss.xmode = inquire_addr_db_xmode(addr_ref);
	dss.delay = DLL_SEND_DELAY_STICKS;

	expiring_sticks = psr_patrol_sticks(pipe_ref);
#ifdef USE_MAC
	declare_channel_occupation(dss.phase,phy_trans_sticks(dss.lsdu_len + LEN_TOTAL_OVERHEAD_BEYOND_LSDU, dss.xmode & 0x03, dss.prop & BIT_DLL_SEND_PROP_SCAN) + expiring_sticks);
#endif

	sid = dll_send(&dss);

	status = wait_until_send_over(sid);
	if(!status)
	{
		delete_timer(tid);
		OSMemPut(MINOR,npdu);
		return 0;
	}

	return_len = 0;
	set_timer(tid,expiring_sticks);
	do
	{
		PSR_RCV_HANDLE ps;
		
		ps = psr_rcv();
		if(ps)
		{
			//if((ps->psr_rcv_valid & BIT_PSR_RCV_UPLINK) && (ps->pipe_id == pipe_id))
			if((ps->psr_rcv_valid & BIT_PSR_RCV_UPLINK) && (ps->psr_rcv_valid & BIT_PSR_RCV_PATROL) && (ps->pipe_id == pipe_id))
			{
				pause_timer(tid);
#ifdef DEBUG_MANIPULATION	
				my_printf("GOT PATROL RETURN.");
				print_transaction_timing(tid,expiring_sticks);
#endif
				mem_cpy(return_buffer,&ps->npdu[SEC_NPDU_PSR_NSDU],ps->npdu_len-LEN_NPCI);
				return_len = ps->npdu_len-LEN_NPCI;

				// 2014-10-08 patrol的结果加上CC的接收结果
				{
					u8 i;
					u8 * ptw;

					pp = GET_PHY_HANDLE(ps);
					ptw = &return_buffer[return_len];

					get_uid(phase, ptw);
					ptw += 6;
					return_len += 6;
					for(i=0;i<4;i++)
					{
						if((pp->phy_rcv_supposed_ch) & (0x10<<i))
						{
							*(ptw++) = pp->phy_rcv_ss[i];
							*(ptw++) = pp->phy_rcv_ebn0[i];
							*(ptw++) = dss.xmode;
							return_len += 3;
						}
					}
				}
			}
			else
			{
#ifdef DEBUG_MANIPULATION	
				my_printf("GOT A WRONG RETURN.");
				print_phy_rcv(GET_PHY_HANDLE(ps));
#endif
			}
			psr_rcv_resume(ps);
		}
		FEED_DOG();
	}while(!is_timer_over(tid) && !return_len);
	
	OSMemPut(MINOR,npdu);
	delete_timer(tid);
	return return_len;
}


/*******************************************************************************
* Function Name  : notify_psr_mnpl_db_expired_addr()
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void notify_psr_mnpl_db_expired_addr(ADDR_REF_TYPE addr_ref)
{
	u16 i,j;
	u8 k;
	BOOL obsoleted;
	
	for(i=0;i<psr_pipe_mnpl_db_usage;i++)
	{
		obsoleted = FALSE;

		/* check if the record contains the obsoleted addr reference */
		for(k=0;k<psr_pipe_mnpl_db[i].pipe_stages;k++)
		{
			if(psr_pipe_mnpl_db[i].way_uid[k]==addr_ref)
			{
				obsoleted = TRUE;
				break;
			}
		}

		/* if it does contain the obsoleted addr reference, delete it by move the database */
		if(obsoleted)
		{
			psr_pipe_mnpl_db_usage--;
			for(j=i;j<psr_pipe_mnpl_db_usage;j++)
			{
				mem_cpy((ARRAY_HANDLE)(&psr_pipe_mnpl_db[j]), (ARRAY_HANDLE)(&psr_pipe_mnpl_db[j+1]), sizeof(PIPE_MNPL_STRUCT));
			}
		}
	}
}


///*******************************************************************************
//* Function Name  : psr_patrol()
//* Description    : 
//* Input          : None
//* Output         : None
//* Return         : None
//*******************************************************************************/
//void psr_patrol(u8 phase, u16 pipe_id)
//{
//	PSR_SEND_STRUCT pss;
//	PSR_RCV_STRUCT * pprs;
//	u8 * psr_lpdu;
//	TIMER_ID_TYPE tid;
//	u32 resp_timing;
//	PIPE_MNPL_STRUCT * pbps;
//	
//	static u16 call_cnt = 0;
//	static u16 reply_cnt = 0;
//	
//	pbps = inquire_pipe_by_pipe_id(pipe_id);
//	if(pbps)
//	{
//		if(req_timer(&tid) == OKAY)
//		{
//			psr_lpdu = OSMemGet(MINOR);
//			if(psr_lpdu != NULL)
//			{
//				pss.pipe_id = pipe_id;
//				pss.prop = BIT_PSR_CONF_BIWAY|BIT_PSR_CONF_PATROL|get_psr_index(phase);
//				pss.nsdu = psr_lpdu;
//				pss.nsdu_len = 0;
//				call_cnt++;
//				if(psr_send(phase, &pss) == OKAY)
//				{
//					resp_timing = calc_biway_psr_patrol_timing(pbps);
//#ifdef DEBUG_PSR
//					my_printf("PSR PIPE PATROL, PIPE_ID:0x%x, WAIT REPLY %d\r\n", pipe_id, resp_timing);
//#endif
//					set_timer(tid,resp_timing);
//					do
//					{
//						phy_resp_proc();
//						nw_rcv_proc(phase);

//						if(psr_rcv(phase,&pprs))
//						{
//							reply_cnt++;
//#ifdef DEBUG_PSR			
//							{
//							u8 xdata * pt;
//							u8 len;
//							
//							pt = pprs->npdu;
//							len = 0;
//							my_printf("GOT NW REPLY:");
//							uart_send_asc(pprs->npdu,pprs->npdu_len);
//							my_printf(" EXPECTED:%ldms,ACTUAL%ldms,DIFF%ldms\r\n",resp_timing,resp_timing-get_timer_val(tid),get_timer_val(tid));
//							
//							while(len<pprs->npdu_len)
//							{
//								u8 ss,snr,plan;
//								uart_send_asc(pt,6);
//								pt += 6;
//								ss = *(pt++);
//								snr = *(pt++);
//								plan = *(pt++);
//								
//								//my_printf(" SS:%bX SNR:%bX PLAN:%bX\r\n",ss,snr,plan);
//								my_printf(" %bX %bX %bX\r\n",ss,snr,plan);
//								len += 9;
//							}
//							
//							
//							my_printf("%d/%d\r\n",reply_cnt,call_cnt);
//							}
//							
//#endif
//							break;
//						}
//					}while(!is_timer_over(tid));
//#ifdef DEBUG_PSR
//					if(is_timer_over(tid))
//					{
//						my_printf("PATROL TIMEOUT.\r\n");
////						while(1);
//					}
//					delete_timer(tid);
//#endif
//				}
//				psr_rcv_resume(phase);
//				OSMemPut(MINOR,psr_lpdu);
//			}
//		}
//	}
//	else
//	{
//#ifdef DEBUG_PSR
//		my_printf("No Such PIPE \r\n");
//#endif
//	}
//}


/*******************************************************************************
* Function Name  : update_pipe_mnpl_db
* Description    : update a pipe mgnt
*					
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS update_pipe_mnpl_db(u8 phase, u16 pipe_id, u8 * script, u16 script_len)
{
	u16 i;
	PIPE_MNPL_STRUCT * ppms;
	ADDR_REF_TYPE addr_ref;
	ARRAY_HANDLE pt;
	
	ADDR_REF_TYPE addr_ref_vec[CFG_PIPE_STAGE_CNT];
	

	/* check sript format */
	if(script_len<8)
	{
#ifdef DEBUG_MANIPULATION
		my_printf("err:pipe info not enough.\r\n");
#endif
		return FAIL;
	}

	if(((script_len>>3)<<3) != script_len)		//script_len必须是8的整倍数
	{
#ifdef DEBUG_MANIPULATION
		my_printf("err:pipe info format error.\r\n");
#endif
		return FAIL;
	}
	
	if(pipe_id > 0x0FFF)
	{
#ifdef DEBUG_MANIPULATION
		my_printf("err:pipe id is limited to 12 bits.\r\n");
#endif
		return FAIL;
	}

	if(psr_pipe_mnpl_db_usage == CFG_PSR_PIPE_MNPL_CNT)
	{
#ifdef DEBUG_MANIPULATION
		my_printf("err:pipe manipulation database is full!\r\n");
#endif
	}

	// 首先添加script中的节点地址到数据库中, 并保留索引值, 以避免由于地址库溢出造成索引错误
	pt = script;
	for(i=0;i<(script_len / 8);i++)
	{
		addr_ref_vec[i] = add_addr_db(pt);
		pt += 8;
	}
	

	// look for same pipe_id in the mgnt database.
	for(i=0;i<psr_pipe_mnpl_db_usage;i++)
	{
		if((psr_pipe_mnpl_db[i].pipe_info & 0x0FFF) == (pipe_id&0x0FFF))
		{
			psr_pipe_mnpl_db_usage--;
			break;
		}
	}

	if(i==CFG_PSR_PIPE_MNPL_CNT)			//如果数据库已满,删除第一条记录,新记录加在末尾
	{
		for(i=0;i<CFG_PSR_PIPE_MNPL_CNT-1;i++)
		{
			mem_cpy((ARRAY_HANDLE)(&psr_pipe_mnpl_db[i]), (ARRAY_HANDLE)(&psr_pipe_mnpl_db[i+1]), sizeof(PIPE_MNPL_STRUCT));
		}
	}
	else
	{
		psr_pipe_mnpl_db_usage++;
	}

	
	// now ppms is the pointer of the pipe struct that new info can be putted into.
	ppms = &psr_pipe_mnpl_db[i];
	ppms->phase = phase;
	ppms->pipe_info = (pipe_id | BIT_PSR_PIPE_INFO_BIWAY | BIT_PSR_PIPE_INFO_ENDPOINT);
	ppms->pipe_stages = (script_len / 8);

	pt = script;
	for(i=0;i<ppms->pipe_stages;i++)
	{
		ppms->way_uid[i]=addr_ref_vec[i];
		ppms->xmode_rmode[i]=(encode_xmode(pt[6],0)<<4) | encode_xmode(pt[7],0);
		pt += 8;
	}

	/******************************* Update pipe record data_base ****************************/
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
#ifdef DEBUG_MANIPULATION
		my_printf("cc psr record db is full!\r\n");
#endif
		// 如果存储空间已满, 删除第一条记录, 并把新的pipe信息加到队尾
		for(i=0;i<CFG_PSR_PIPE_RECORD_CNT-1;i++)
		{
			mem_cpy((ARRAY_HANDLE)(&_psr_pipe_record_db[i]), (ARRAY_HANDLE)(&_psr_pipe_record_db[i+1]), sizeof(PSR_PIPE_RECORD_STRUCT));
		}
	}
	else
	{
		_psr_pipe_record_db_usage++;
	}

	// uplink, downlink are all the same node
	_psr_pipe_record_db[i].pipe_info = (pipe_id & 0x0FFF) | BIT_PSR_PIPE_INFO_BIWAY | BIT_PSR_PIPE_INFO_ENDPOINT;	// restore pipe_id
	addr_ref = addr_ref_vec[0];
	update_addr_db(addr_ref,script[6],script[7]);
	_psr_pipe_record_db[i].downlink_next = addr_ref;
	_psr_pipe_record_db[i].uplink_next = addr_ref;

	return OKAY;
}

/*******************************************************************************
* Function Name  : inquire_pipe_mnpl_db
* Description	 : 查询pipe_id, TEST_PIPE_ID的优先级最低
*					//2014-10-14 如果TEST_PIPE_ID为唯一检索结果,返回NULL
*					2015-09-29 允许传入的pipe_id为TEST_PIPE_ID
* Input 		 : 
* Output		 : 
* Return		 : 
*******************************************************************************/
PIPE_REF inquire_pipe_mnpl_db(u16 pipe_id)
{
	u16 i;
	u16 temp;
//	PIPE_REF tmp_pipe_ref = NULL;

	for(i=0;i<psr_pipe_mnpl_db_usage;i++)
	{
		temp = (psr_pipe_mnpl_db[i].pipe_info & 0x0FFF);
		if((temp == pipe_id))
		{
			//if(temp != TEST_PIPE_ID)	// 2015-09-29 允许以pipe_id的test_pipe的检索查询, 我认为不会影响正常功能, 禁止uid返回test_pipe是对的
			{
				return &psr_pipe_mnpl_db[i];
			}
			//else
			//{
			//	tmp_pipe_ref = &psr_pipe_mnpl_db[i];
			//}
		}
	}
	//return tmp_pipe_ref;			//如果test_pipe_id是唯一检索结果,返回NULL
	return NULL;
}

/*******************************************************************************
* Function Name  : get_pipe_phase
* Description	 : 查询pipe所在phase
*					
* Input 		 : 
* Output		 : 
* Return		 : 
*******************************************************************************/
u8 get_pipe_phase(u16 pipe_id)
{
	PIPE_REF pipe_ref;

	pipe_ref = inquire_pipe_mnpl_db(pipe_id);
	if(pipe_ref)
	{
		return pipe_ref->phase;
	}
	return (u8)INVALID_RESOURCE_ID;
}

/*******************************************************************************
* Function Name  : inquire_pipe_mnpl_db_by_uid
* Description	 : 以目的节点uid为查询索引, 返回pipe句柄
*					2013-07-23 如TEST_ID是唯一的索引结果, 将其返回
								如不是, 返回非TEST_ID的索引结果
					2014-10-14 TEST_ID不作为返回索引的结果
* Input 		 : 
* Output		 : 
* Return		 : 
*******************************************************************************/
PIPE_REF inquire_pipe_mnpl_db_by_uid(u8 * uid)
{
	u16 i;
	PIPE_REF pipe;
	ADDR_REF_TYPE addr_ref;
//	PIPE_REF tmp_pipe_ref = NULL;

	for(i=0;i<psr_pipe_mnpl_db_usage;i++)
	{
		pipe = &psr_pipe_mnpl_db[i];
		addr_ref = pipe->way_uid[pipe->pipe_stages-1];
		if(mem_cmp(inquire_addr_db(addr_ref),uid,6))
		{
			if((pipe->pipe_info&0x0FFF) != TEST_PIPE_ID)
			{
				return pipe;
			}
			//else
			//{
			//	tmp_pipe_ref = pipe;
			//}
		}
	}
	//return tmp_pipe_ref;
	return NULL;			//2014-10-14 如只有TEST_PIPE_ID作为检索唯一结果返回NULL
}


/*******************************************************************************
* Function Name  : is_pipe_recepient_mnpl_db
* Description	 : check if the return uid is the recipient of the right pipe
*					
* Input 		 : pipe_id, pt of target_uid
* Output		 : 
* Return		 : TRUE/FALSE
*******************************************************************************/
BOOL is_pipe_recepient_mnpl_db(u16 pipe_id, u8 * uid)
{
	PIPE_REF pipe;
	
	pipe = inquire_pipe_mnpl_db(pipe_id);
	if(pipe)
	{
		ADDR_REF_TYPE addr_ref;

		addr_ref = pipe->way_uid[pipe->pipe_stages-1];
		if(mem_cmp(inquire_addr_db(addr_ref),uid,6))
		{
			return TRUE;
		}
	}
	return FALSE;
}

/*******************************************************************************
* Function Name  : inquire_pipe_recepient
* Description	 : inquire the psr mnpl db, return the address of the recepient of the pipe
*					
* Input 		 : pipe_id
* Output		 : 
* Return		 : recepient uid address
*******************************************************************************/
UID_HANDLE inquire_pipe_recepient(u16 pipe_id)
{
	PIPE_REF pipe_ref;
	
	pipe_ref = inquire_pipe_mnpl_db(pipe_id);
	if(pipe_ref)
	{
		return inquire_addr_db(pipe_ref->way_uid[pipe_ref->pipe_stages-1]);
	}
	return NULL;
}

/*******************************************************************************
* Function Name  : is_pipe_forward_mnpl_db
* Description    : check if the return uid is one of the delivers of the pipe
*					检查uid是否是pipe的转发节点
* Input          : pipe_id, pt of target_uid
* Output         : 
* Return         : TRUE/FALSE
*******************************************************************************/
BOOL is_pipe_forward_mnpl_db(u16 pipe_id, u8 * uid)
{
	PIPE_REF pipe;
	u8 i;
	
	pipe = inquire_pipe_mnpl_db(pipe_id);
	if(pipe)
	{
		ADDR_REF_TYPE addr_ref;

		for(i=0;i<pipe->pipe_stages-1;i++)
		{
			addr_ref = pipe->way_uid[i];
			if(mem_cmp(inquire_addr_db(addr_ref),uid,6))
			{
				return TRUE;
			}
		}
	}
	return FALSE;

}

/*******************************************************************************
* Function Name  : psr_transaction_core()
* Description    : 
* Input          : pipe_id,npdu,npdu_len,resp_timing
* Output         : returned npdu, returned npdu length, psr_err
					2013-05-07 if error & return_buffer valid, write the error code in the first byte of return_buffer
* Return         : OKAY/FAIL
					result=OKAY, 表示psr层通信成功, 但mgnt层的功能是否成功, 需要检查return_buffer
					(e.g. psr_diag)
*******************************************************************************/
STATUS psr_transaction_core(u16 pipe_id, ARRAY_HANDLE nsdu, BASE_LEN_TYPE nsdu_len, u32 expiring_sticks, 
 						ARRAY_HANDLE return_buffer, ERROR_STRUCT * psr_err)
{
	u8 phase;
	PIPE_REF pipe_ref;
	PSR_SEND_STRUCT pss;
	PSR_RCV_HANDLE pn;
	MGNT_RCV_HANDLE pm;
	TIMER_ID_TYPE tid;
	STATUS status;
	SEND_ID_TYPE sid;

	psr_err->err_code = NO_ERROR;

	pipe_ref = inquire_pipe_mnpl_db(pipe_id);
	if(pipe_ref)
	{
		phase = pipe_ref->phase;
	}
	else
	{
#ifdef DEBUG_MANIPULATION
		my_printf("PSR ERR: NO PIPE INFO.\r\n");
#endif
		psr_err->pipe_id = pipe_id;
		//get_uid(phase, psr_err->uid);		//2015-11-23 phase not assignned
		psr_err->err_code = NO_PIPE_INFO;
		if(return_buffer)
		{
			return_buffer[0] = psr_err->err_code;
		}
		return FAIL;
	}

	tid = req_timer();
	if(tid==INVALID_RESOURCE_ID)
	{
#ifdef DEBUG_MANIPULATION
		my_printf("psr err: req timer fail.\r\n");
#endif
		psr_err->pipe_id = pipe_id;
		get_uid(phase, psr_err->uid);
		psr_err->err_code = TIMER_RUN_OUT;
		if(return_buffer)
		{
			return_buffer[0] = psr_err->err_code;
		}
		return FAIL;
	}

#ifdef DEBUG_MANIPULATION
	my_printf("PSR TRANSACTION, PIPE_ID:0x%x, TIMING ESTIMATE %ldts\r\n", pipe_id, expiring_sticks);
#endif

	powermesh_main_thread_useless_resp_clear();

	pss.phase = phase;
	pss.pipe_id = pipe_id;
	pss.prop = 0;
	pss.uplink = 0;
	pss.index = get_psr_index(phase)&0x03;
	pss.nsdu = nsdu;
	pss.nsdu_len = nsdu_len;

	sid = psr_send(&pss);
#ifdef USE_MAC
	if(sid != INVALID_RESOURCE_ID)
	{
		declare_channel_occupation(sid,expiring_sticks);
	}
#endif

	status = wait_until_send_over(sid);
	if(!status)
	{
#ifdef DEBUG_DISP_INFO
		my_printf("psr err: send fail.\r\n");
#endif	
		psr_err->pipe_id = pipe_id;
		get_uid(phase, psr_err->uid);
		psr_err->err_code = UNKNOWN_ERR;
		delete_timer(tid);
		if(return_buffer)
		{
			return_buffer[0] = psr_err->err_code;
		}
		return FAIL;
	}
	
	status = FAIL;
	set_timer(tid,expiring_sticks);
	do
	{
		SET_BREAK_THROUGH("quit psr_transaction_core()\r\n");

		pm = mgnt_rcv();
		if(pm)
		{	
			pn = GET_NW_HANDLE(pm);
			if((pn->pipe_id == pipe_id) 
				&& (pn->phase == phase)
				&& ((pn->npdu[SEC_NPDU_PSR_CONF]&0x03) == pss.index)
				)
			{
				if(pn->npdu[SEC_NPDU_PSR_CONF]&BIT_PSR_CONF_ERROR)	// got a error report package
				{
					psr_err->pipe_id = pipe_id;
					mem_cpy(psr_err->uid, &pn->npdu[SEC_NPDU_MSDU], 6);
					psr_err->err_code = (PSR_ERROR_TYPE)(pn->npdu[SEC_NPDU_MSDU+6]);
					mgnt_rcv_resume(pm);
					if(return_buffer)
					{
						return_buffer[0] = psr_err->err_code;
					}
					break;
				}
				else if(is_pipe_recepient_mnpl_db(pipe_id,&pm->mpdu[SEC_MPDU_MSDU]))
				{
					pause_timer(tid);
					if(pm->mpdu[SEC_MPDU_MSDU+6]==0x80)
					{
#ifdef DEBUG_MANIPULATION
						print_transaction_timing(tid, expiring_sticks);
#endif
						if(return_buffer)
						{
							*return_buffer++ = pm->mpdu_len-LEN_MPCI-7;	// return buffer首字节表示返回数据长度;
							mem_cpy(return_buffer, &pm->mpdu[SEC_MPDU_MSDU+7], pm->mpdu_len-LEN_MPCI-7);
						}
						status = OKAY;
					}
					else
					{
#ifdef DEBUG_DISP_INFO
						my_printf("psr err:target returns error:%bx.\r\n",pm->mpdu[SEC_MPDU_MSDU+6]);	//目标节点返回执行错误, mgnt_diag 返回 time_out, psr_inquire返回execute_err
#endif
						psr_err->pipe_id = pipe_id;
						mem_cpy(psr_err->uid, &pn->npdu[SEC_NPDU_MSDU], 6);
						psr_err->err_code = (PSR_ERROR_TYPE)(pn->npdu[SEC_NPDU_MSDU+6]);
						mgnt_rcv_resume(pm);
						if(return_buffer)
						{
							return_buffer[0] = psr_err->err_code;
						}
						status = OKAY;						// 2013-07-18 psr_transaction仅负责psr通信, 不负责上层功能的成功性
					}
				}
				else
				{
					if(is_pipe_forward_mnpl_db(pipe_id,&pm->mpdu[SEC_MPDU_MSDU]))
					{
#ifdef DEBUG_DISP_INFO
						my_printf("psr err:pipe feedback.\r\n");
//						while(1);
#endif
						psr_err->pipe_id = pipe_id;
						mem_cpy(psr_err->uid, &pn->npdu[SEC_NPDU_MSDU], 6);
						psr_err->err_code = (PSR_ERROR_TYPE)(pn->npdu[SEC_NPDU_MSDU+6]);
						mgnt_rcv_resume(pm);
						if(return_buffer)
						{
							return_buffer[0] = psr_err->err_code;
						}
						break;
					}
					else
					{
#ifdef DEBUG_DISP_INFO
						my_printf("psr err:uid not match pipe.\r\n");
						print_phy_rcv(GET_PHY_HANDLE(pn));
//						while(1);
#endif
						psr_err->pipe_id = pipe_id;
						mem_cpy(psr_err->uid, pn->npdu, 6);
						psr_err->err_code = UID_NOT_MATCH;
						mgnt_rcv_resume(pm);
						if(return_buffer)
						{
							return_buffer[0] = psr_err->err_code;
						}
						break;
					}
				}
			}
			else
			{
#ifdef DEBUG_DISP_INFO
				my_printf("pipeid/phase/index error abandon.\r\n");
				print_phy_rcv(GET_PHY_HANDLE(pn));
//				while(1);
#endif
			}
			mgnt_rcv_resume(pm);
		}
		else			// 清除掉所有接收的其他无关的数据包
		{
			psr_transaction_clear_irrelevant();
		}
		FEED_DOG();
	}while(!is_timer_over(tid) && !status && psr_err->err_code==NO_ERROR);
	
	if(is_timer_over(tid))
	{
		psr_err->pipe_id = pipe_id;
		get_uid(phase, psr_err->uid);
		psr_err->err_code = PSR_TIMEOUT;
		if(return_buffer)
		{
			return_buffer[0] = psr_err->err_code;
		}
	}

	delete_timer(tid);
	return status;
}

/*******************************************************************************
* Function Name  : psr_transaction_clear_irrelevant()
* Description    : clear irrelevant package received when waiting PSR transaction
				2013-07-28 在组网过程中,一旦受到一个dst包就阻塞了后面的接收, 归根到底是
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void psr_transaction_clear_irrelevant()
{
	DLL_RCV_HANDLE pd;
	PSR_RCV_HANDLE pn;
	u8 i;

	/* 清除app层的无用接收包, 释放接收机 */
	for(i=0;i<CFG_PHASE_CNT;i++)
	{
		APP_STACK_RCV_HANDLE papp;
	
		papp = &_app_rcv_obj[i];				//用指针取代数组取址,可减小代码体积
		if(papp->app_rcv_indication)
		{
			app_rcv_resume(papp);
		}

#ifdef USE_DST
		{
			DST_STACK_RCV_HANDLE pdst;
			
			pdst = &_dst_rcv_obj[i];
			if(pdst->dst_rcv_indication)
			{
				dst_rcv_resume(pdst);
			}
		}
#endif

#ifdef USE_PTP
		{
			PTP_STACK_RCV_HANDLE pptp;
			
			pptp = &_ptp_rcv_obj[i];
			if(pptp->ptp_rcv_indication)
			{
				ptp_rcv_resume(pptp);
			}
		}
#endif


	}	

	/* 清除dll层的无用接收包, 释放接收机 */
	pd = dll_rcv();								//dll_rcv()接收到的都是主线程处理的包,如ack,srf,diag ack etc.
	if(pd)
	{
		dll_rcv_resume(pd);
	}

	/* 清除psr层的无用接收包, 释放接收机 */
	pn = psr_rcv();								//psr_rcv()接收到的是需要主线程处理的psr层数据包,如patrol返回,psr setup affm,error report等
	if(pn)
	{
		psr_rcv_resume(pn);
	}

}

/*******************************************************************************
* Function Name  : psr_transaction()
* Description    : psr_transaction()根据传入的autoheal_level确定通信失败时的措施
					AUTOHEAL_LEVEL:
					LEVEL0: 无重试, 无自修复;
					LEVEL1: 超时重试(CFG_MAX_PSR_TRANSACTION_TRY-1次),收到no_pipe_info的error feedback时补发refresh_pipe; 
					LEVEL2: 重试失败时修复PIPE链路, 若成功最多重试一次;
					LEVEL3: 修复失败时重建路径, 若成功最多重试一次;
					
* Input          : pipe_id,npdu,npdu_len,resp_timing
* Output         : returned npdu, returned npdu length, psr_err
* Return         : OKAY/FAIL
*******************************************************************************/
STATUS psr_transaction(u16 pipe_id, ARRAY_HANDLE nsdu, BASE_LEN_TYPE nsdu_len, u32 expiring_sticks, 
 						ARRAY_HANDLE return_buffer, AUTOHEAL_LEVEL autoheal_level)
{
	STATUS status = FAIL;
	ARRAY_HANDLE nsdu_duplicate;
	u8 try_times = 0;
	ERROR_STRUCT psr_err;

	nsdu_duplicate = OSMemGet(SUPERIOR);
	if(!nsdu_duplicate)
	{
		return FAIL;
	}

	/*AUTOHEAL_LEVEL1 above: retry.*/
	for(try_times=0;try_times<CFG_MAX_PSR_TRANSACTION_TRY;try_times++)
	{
		SET_BREAK_THROUGH("quit psr_transaction()\r\n");

		mem_cpy(nsdu_duplicate,nsdu,nsdu_len);
		status = psr_transaction_core(pipe_id, nsdu_duplicate, nsdu_len, expiring_sticks, return_buffer, &psr_err);

		if(status)
		{
#ifdef DEBUG_MANIPULATION
			my_printf("PSR_TRANSACTION: okay\r\n");
#endif		
			break;		
		}
		else
		{
			/*AUTOHEAL_LEVEL0: quit when fail.*/
			if(autoheal_level==AUTOHEAL_LEVEL0)
			{
				break;
			}
			else
			{
				/*AUTOHEAL_LEVEL1 above: when got no_pipe_info, refresh pipe.*/
				if(psr_err.err_code == NO_PIPE_INFO)
				{
#ifdef DEBUG_MANIPULATION
					my_printf("psr_transaction(autoheal level 1+):got no_pipe_info, refresh\r\n");
#endif
					if(refresh_pipe(pipe_id))
					{
						mem_cpy(nsdu_duplicate,nsdu,nsdu_len);
						status = psr_transaction_core(pipe_id, nsdu_duplicate, nsdu_len, expiring_sticks, return_buffer, &psr_err);
					}
				}
			}
		}
	}

	/*AUTOHEAL_LEVEL2 above: repair pipe.*/
	if(!status && (psr_err.err_code == PSR_TIMEOUT))
	{
		if(autoheal_level>=AUTOHEAL_LEVEL2)
		{
#ifdef DEBUG_MANIPULATION
			my_printf("psr_transaction(autoheal level 2+):repair\r\n");
#endif
			if(repair_pipe(pipe_id))
			{
				mem_cpy(nsdu_duplicate,nsdu,nsdu_len);
				status = psr_transaction_core(pipe_id, nsdu_duplicate, nsdu_len, expiring_sticks, return_buffer, &psr_err);
			}
		}
	}

	/*AUTOHEAL_LEVEL2 above: repair pipe.*/
#ifdef USE_DST	
	if(!status && (psr_err.err_code == PSR_TIMEOUT))
	{
		if(autoheal_level>=AUTOHEAL_LEVEL3)
		{
#ifdef DEBUG_MANIPULATION
			my_printf("psr_transaction(autoheal level 3+):rebuild\r\n");
#endif
			if(rebuild_pipe_by_dst(pipe_id, 0))			//以相同跳数, 相同速率在原相位重建
			{
				mem_cpy(nsdu_duplicate,nsdu,nsdu_len);
				status = psr_transaction_core(pipe_id, nsdu_duplicate, nsdu_len, expiring_sticks, return_buffer, &psr_err);
			}
		}
	}
#endif
	
	if(!status && psr_err.err_code != PSR_TIMEOUT)
	{
#ifdef DEBUG_MANIPULATION
		my_printf("PSR_TRANSACTION: misc error! needs check!\r\n");
		print_psr_err(&psr_err);
#endif
	}

	OSMemPut(SUPERIOR, nsdu_duplicate);
	return status;
}




/*******************************************************************************
* Function Name  : get_available_pipe_id
* Description    : get an available pipe id
*					
* Input          : pipe id
* Output         : 
* Return         : address of uid
*******************************************************************************/
u16 get_available_pipe_id()
{
	u16 i;
	PIPE_REF pipe_ref;

	for(i=1;i<0x1000;i++)			// pipe_id is not 0
	{
		pipe_ref = inquire_pipe_mnpl_db(i);
		if(pipe_ref == NULL)
		{
			return (i & 0x0FFF);
		}
	}
#ifdef DEBUG_MANIPULATION
	my_printf("FATAL: OUT OF PIPE ID RESOURCE!\r\n");
#endif
	return 0;
}


/*******************************************************************************
* Function Name  : get_path_script_from_pipe_mnpl_db
* Description    : get pipe setup script from pipe id
*					
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u16 get_path_script_from_pipe_mnpl_db(u16 pipe_id, ARRAY_HANDLE script_buffer)
{
	PIPE_REF pipe_ref;
	u8 i;
	ARRAY_HANDLE ptw;

	pipe_ref = inquire_pipe_mnpl_db(pipe_id);
	if(!pipe_ref)
	{
		return 0;
	}

	ptw = script_buffer;
	for(i=0;i<pipe_ref->pipe_stages;i++)
	{
		mem_cpy(ptw, inquire_addr_db(pipe_ref->way_uid[i]), 6);
		ptw += 6;
		*ptw++ = decode_xmode((pipe_ref->xmode_rmode[i])>>4);
		*ptw++ = decode_xmode((pipe_ref->xmode_rmode[i])&0x0F);
	}

	return (u16)(ptw-script_buffer);
}

/*******************************************************************************
* Function Name  : get_pipe_usage
* Description    : 获得pipe的总数
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u16 get_pipe_usage(void)
{
	return psr_pipe_mnpl_db_usage;
}

/*******************************************************************************
* Function Name  : get_pipe_id
* Description    : 通过pipe_ref获得pipe_id
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u16 get_pipe_id(PIPE_REF pipe_ref)
{
	if(pipe_ref)
	{
		return (pipe_ref->pipe_info&0x0FFF);
	}
	else
	{
		return 0;
	}
}

/*******************************************************************************
* Function Name  : inquire_pipe_stages
* Description    : 输入pipe_id查询pipe级数, 最小为1; 如pipe_id错误, 返回0;
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u8 inquire_pipe_stages(u16 pipe_id)
{
	PIPE_REF pipe_ref;

	pipe_ref = inquire_pipe_mnpl_db(pipe_id);
	if(pipe_ref)
	{
		return pipe_ref->pipe_stages;
	}
	else
	{
		return 0;
	}
}

#endif
/******************* (C) COPYRIGHT 2012 Belling *****END OF FILE****/
