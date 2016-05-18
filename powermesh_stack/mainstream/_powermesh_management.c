/******************** (C) COPYRIGHT 2012 Belling Inc. ********************
* File Name          : powermesh_management.c
* Author             : Lv Haifeng
* Version            : V2.0
* Date               : 2013-03-28
* Description        : mgnt_app提供的是MGNT的功能子集, 本函数提供powermesh定义在
					管理层内的管理功能实现
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

/* Private define ------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : psr_mgnt_cmd()
* Description    : mgnt命令的统一处理进程, 
* Input          : pt_prop分别针对额外属性数据体
* Output         : None
* Return         : 
*******************************************************************************/
STATUS psr_mgnt_cmd(u8 mgnt_cmd, u16 pipe_id, void * pt_prop, ARRAY_HANDLE return_buffer, AUTOHEAL_LEVEL autoheal_level)
{
	u32 expiring_sticks;
	PIPE_REF pipe_ref;
	STATUS result;
	ARRAY_HANDLE mpdu;
	u8 mpdu_len;
	ARRAY_HANDLE ptw;
	
	result = FAIL;

	/* check validity of pipe_id */
	pipe_ref = inquire_pipe_mnpl_db(pipe_id);
	if(!pipe_ref)
	{
#ifdef DEBUG_MGNT
		my_printf("PSR ERR: NO PIPE INFO.\r\n");
#endif
		if(return_buffer)
		{
			return_buffer[0] = NO_PIPE_INFO;
		}
		return FAIL;

	}

	mpdu = OSMemGet(MINOR);
	if(!mpdu)
	{
		if(return_buffer)
		{
			return_buffer[0] = MEM_OUT;
		}

		return result;
	}

	switch(mgnt_cmd)
	{
		case(EXP_MGNT_PING):
		{
			expiring_sticks = psr_ping_sticks(pipe_ref);
			mpdu[0] = (0x80 | EXP_MGNT_PING);
			mpdu_len = 1;
			break;
		}
		case(EXP_MGNT_DIAG):
		{
			DLL_SEND_HANDLE pds;
			
			pds = (DLL_SEND_HANDLE)(pt_prop);
			expiring_sticks = psr_diag_sticks(pipe_ref,pds);
			ptw = mpdu;
			*(ptw++) = (0x80 | EXP_MGNT_DIAG);
			mem_cpy(ptw,pds->target_uid_handle,6);
			ptw += 6;
			*(ptw++) = pds->xmode;
			*(ptw++) = pds->rmode;
			*(ptw++) = (pds->prop & BIT_DLL_SEND_PROP_SCAN)?1:0;
			mpdu_len = ptw-mpdu;
			break;
		}
		case(EXP_MGNT_EBC_BROADCAST):
		{
			EBC_BROADCAST_HANDLE ebc;

			ebc = (EBC_BROADCAST_HANDLE)(pt_prop);

			/* limit ebc window */
			switch(ebc->rmode & 0x03)
			{
				case(0x00):
				{
					if(ebc->window > CFG_EBC_MAX_BPSK_WINDOW)
					{
						ebc->window = CFG_EBC_MAX_BPSK_WINDOW;
					}
					break;
				}
				case(0x01):
				{
					if(ebc->window > CFG_EBC_MAX_DS15_WINDOW)
					{
						ebc->window = CFG_EBC_MAX_DS15_WINDOW;
					}
					break;
				}
				case(0x02):
				{
					if(ebc->window > CFG_EBC_MAX_DS63_WINDOW)
					{
						ebc->window = CFG_EBC_MAX_DS63_WINDOW;
					}
				}
			}

			expiring_sticks = psr_broadcast_sticks(pipe_ref,ebc);
			ptw = mpdu;
			*ptw++ = (0x80 | EXP_MGNT_EBC_BROADCAST);
			*ptw++ = ebc->bid;
			*ptw++ = ebc->xmode;
			*ptw++ = ebc->rmode;
			*ptw++ = ebc->scan;
			*ptw++ = ebc->window;
			*ptw++ = ebc->mask;
			mpdu_len = 7;

			/*** todo: add other conditions ***/
			if(ebc->mask & BIT_NBF_MASK_UID)
			{
				mem_cpy(ptw,ebc->uid,6);
				ptw += 6;
				mpdu_len += 6;
			}
			if(ebc->mask & BIT_NBF_MASK_METERID)
			{
				mem_cpy(ptw,ebc->mid,6);
				ptw += 6;
				mpdu_len += 6;
			}
			if(ebc->mask & BIT_NBF_MASK_BUILDID)
			{
				*ptw++ = ebc->build_id;
				mpdu_len++;
			}
			
			break;
		}
		case(EXP_MGNT_EBC_IDENTIFY):
		{
			EBC_IDENTIFY_HANDLE ebc;

			ebc = (EBC_IDENTIFY_HANDLE)(pt_prop);
			expiring_sticks = psr_identify_sticks(pipe_ref,ebc);
			ptw = mpdu;
			*ptw++ = (0x80 | EXP_MGNT_EBC_IDENTIFY);
			*ptw++ = ebc->bid;
			*ptw++ = ebc->num_bsrf;
			*ptw++ = ebc->max_identify_try;
			mpdu_len = 4;
			
			break;
		}
		case(EXP_MGNT_EBC_ACQUIRE):
		{
			EBC_ACQUIRE_HANDLE ebc;

			ebc = (EBC_ACQUIRE_HANDLE)(pt_prop);
			expiring_sticks = psr_acquire_sticks(pipe_ref,ebc);
			ptw = mpdu;
			*ptw++ = (0x80 | EXP_MGNT_EBC_ACQUIRE);
			*ptw++ = ebc->bid;
			*ptw++ = ebc->node_index;
			*ptw++ = ebc->node_num;
			mpdu_len = 4;
		
			break;
		}
		case(EXP_MGNT_SET_BUILD_ID):
		{
			expiring_sticks = psr_transaction_sticks(pipe_ref,2,9,0);
			ptw = mpdu;
			*ptw++ = (0x80 | EXP_MGNT_SET_BUILD_ID);
			*ptw++ = *(u8 *)(pt_prop);
			mpdu_len = 2;
			
			break;
		}
		default:
		{
#ifdef DEBUG_MANIPULATION
			my_printf("error mgnt cmd %bX\r\n",mgnt_cmd);
#endif
			OSMemPut(MINOR,mpdu);
			return result;
		}
	}

	// 2013-07-18 autoheal_level 作为参数传入
	// 一般地: ping命令的autoheal_level为0, ebc各个命令由于是组网过程中使用, autoheal_level为1(允许重刷pipe, 允许重试)
	// diag命令要看情况, 在组网过程中使用level1, 在诊断和修复过程中使用level1等等, 这样能保证代码的重用和最大的灵活性

	result = psr_transaction(pipe_id, mpdu, mpdu_len, expiring_sticks, return_buffer, autoheal_level);

	OSMemPut(MINOR,mpdu);
	if(!result)
	{
#ifdef DEBUG_MANIPULATION
		my_printf("mgnt psr comm fail\r\n");
#endif
		return FAIL;
	}
	if(result && ((*return_buffer & 0xF0) == 0xF0))	//2013-07-18 status为okay仅表示通信成功, return_buffer首字节为0xFX即表示mgnt层功能失败
	{
#ifdef DEBUG_MANIPULATION
		my_printf("mgnt cmd return fail\r\n");
#endif
		return FAIL;
	}
	else
	{
		return OKAY;
	}
	
}


/*******************************************************************************
* Function Name  : mgnt_ping()
* Description    : ping 除返回0x80外, 还返回剩余资源信息
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
STATUS mgnt_ping(u16 pipe_id, ARRAY_HANDLE return_buffer)
{
	return psr_mgnt_cmd(EXP_MGNT_PING, pipe_id, NULL, return_buffer, AUTOHEAL_LEVEL0);
}

/*******************************************************************************
* Function Name  : mgnt_diag()
* Description    : 
* Input          : None
* Output         : None
* Return         : pt_return_buffer首字节表示返回长度
				 
*******************************************************************************/
STATUS mgnt_diag(u16 pipe_id, DLL_SEND_HANDLE pds, u8 * pt_return_buffer, AUTOHEAL_LEVEL autoheal_level)
{
	u8 i;

	STATUS status;

	for(i = 0;i<CFG_MAX_DIAG_TRY;i++)
	{
		mem_clr(pt_return_buffer,1,8);		//最少返回8字节,靠返回的8字节判断通信情况
		status = psr_mgnt_cmd(EXP_MGNT_DIAG, pipe_id, pds, pt_return_buffer, autoheal_level);

		if(status)
		{
			break;
		}
	}

	return status;
}


/*******************************************************************************
* Function Name  : mgnt_ebc_broadcast()
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
STATUS mgnt_ebc_broadcast(u16 pipe_id, EBC_BROADCAST_STRUCT * pt_ebc, u8 * num_bsrf, AUTOHEAL_LEVEL autoheal_level)
{
	STATUS status;
	u8 buffer[32];

	status = psr_mgnt_cmd(EXP_MGNT_EBC_BROADCAST, pipe_id, pt_ebc, buffer, autoheal_level);
	if(status)
	{
		*num_bsrf = buffer[2];
	}
	return status;
}

/*******************************************************************************
* Function Name  : mgnt_ebc_identify()
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
STATUS mgnt_ebc_identify(u16 pipe_id, EBC_IDENTIFY_STRUCT * pt_ebc, u8 * num_node, AUTOHEAL_LEVEL autoheal_level)
{
	STATUS status;
	u8 buffer[32];

	status = psr_mgnt_cmd(EXP_MGNT_EBC_IDENTIFY, pipe_id, pt_ebc, buffer, autoheal_level);
	if(status)
	{
		*num_node = buffer[2];
	}
	return status;
}

/*******************************************************************************
* Function Name  : mgnt_ebc_acquire()
* Description    : 
* Input          : None
* Output         : None
* Return         : 返回buffer第一字节:后续字节长度, 第二字节: bid, 第三字节开始:返回的uid
*******************************************************************************/
STATUS mgnt_ebc_acquire(u16 pipe_id, EBC_ACQUIRE_STRUCT * pt_ebc, ARRAY_HANDLE buffer, AUTOHEAL_LEVEL autoheal_level)
{
	STATUS status;

	// check validility
	if(pt_ebc->node_num > CFG_EBC_ACQUIRE_MAX_NODES)
	{
#ifdef DEBUG_MGNT
		my_printf("acquire node num no more than %bd\r\n",CFG_EBC_ACQUIRE_MAX_NODES);
#endif
		pt_ebc->node_num = CFG_EBC_ACQUIRE_MAX_NODES;
	}

	status = psr_mgnt_cmd(EXP_MGNT_EBC_ACQUIRE, pipe_id, pt_ebc, buffer, autoheal_level);

	if(status && (*buffer != TARGET_EXECUTE_ERR))		// 2013-07-18 status = okay仅表示通信成功
	{
		return OKAY;
	}
	else
	{
		return FAIL;
	}
}

/*******************************************************************************
* Function Name  : mgnt_ebc_set_build_id()
* Description    : 
* Input          : None
* Output         : None
* Return         : 返回buffer第一字节:后续字节长度, 第二字节: bid, 第三字节开始:返回的uid
*******************************************************************************/
STATUS mgnt_ebc_set_build_id(u16 pipe_id, u8 build_id, AUTOHEAL_LEVEL autoheal_level)
{
	STATUS status;
	u8 buffer[32];

	// check validility
	if(build_id == 0 )
	{
#ifdef DEBUG_MGNT
		my_printf("invalid build id(0)\r\n");
#endif
		return FAIL;
	}

	status = psr_mgnt_cmd(EXP_MGNT_SET_BUILD_ID, pipe_id, &build_id, buffer, autoheal_level);
	
	if(status && (buffer[1]!=build_id))
	{
#ifdef DEBUG_MGNT
		my_printf("error returned build id %bx\r\n",buffer[1]);
#endif
		return FAIL;	
	}
	
	return status;
}

/*******************************************************************************
* Function Name  : mgnt_ebc_inphase_confirm()
* Description    : 当前的过零点电路还有不可靠的地方, 因此判断相位需要double确认, 在psr_explore收集完下级新节点后, 对新节点做一次相位确认
					确认原理是发一个需要ACPS和uid同时匹配的nbf广播
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
STATUS mgnt_ebc_inphase_confirm(u16 pipe_id, EBC_BROADCAST_HANDLE pt_ebc, ARRAY_HANDLE target_uid, AUTOHEAL_LEVEL autoheal_level)
{
	EBC_BROADCAST_STRUCT ebc;

	STATUS status;
	u8 buffer[32];

	mem_cpy((ARRAY_HANDLE)(&ebc),(ARRAY_HANDLE)(pt_ebc),sizeof(EBC_BROADCAST_STRUCT));

	ebc.bid = 0;
	ebc.window = 0;

	ebc.mask = 0x29;		// build id, uid, acphase
	mem_cpy(ebc.uid,target_uid,6);
	ebc.build_id = 0x00;

	status = psr_mgnt_cmd(EXP_MGNT_EBC_BROADCAST, pipe_id, &ebc, buffer, autoheal_level);
	if(status)
	{
		if(buffer[2]==1)
		{
			return OKAY;
		}
	}
	return FAIL;
}


/*******************************************************************************
* Function Name  : ebc_inphase_confirm
* Description    : 
* Input          : 
* Output         : target_id
* Return         : 
*******************************************************************************/
STATUS ebc_inphase_confirm(EBC_BROADCAST_STRUCT * pt_ebc, ARRAY_HANDLE target_uid)
{
	EBC_BROADCAST_STRUCT ebc;
	u8 bsrf_num;

	mem_cpy((ARRAY_HANDLE)(&ebc),(ARRAY_HANDLE)(pt_ebc),sizeof(EBC_BROADCAST_STRUCT));

	ebc.bid = 0;
	ebc.window = 0;
	ebc.build_id = 0x00;

	ebc.mask = 0x09;		// uid, acphase
	mem_cpy(ebc.uid,target_uid,6);

	bsrf_num =  ebc_broadcast(&ebc);
	if(bsrf_num)
	{
		return OKAY;
	}
	else
	{
		return FAIL;
	}
}



/******************* (C) COPYRIGHT 2012 Belling *****END OF FILE****/
