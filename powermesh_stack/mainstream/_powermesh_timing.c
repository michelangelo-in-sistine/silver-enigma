/******************** (C) COPYRIGHT 2012 Belling Inc. ********************
* File Name          : powermesh_timing.c
* Author             : Lv Haifeng
* Version            : V2.0
* Date               : 2013-03-11
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
//#include "powermesh_spec.h"
//#include "general.h"
//#include "hardware.h"

extern DST_CONFIG_STRUCT xdata dst_config_obj;

/* Private define ------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
* Function Name  : scan_expiring_sticks
* Description    : 扫描等待超时
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
u32 scan_expiring_sticks(BASE_LEN_TYPE ppdu_len, u8 rate, u8 ch)
{
	TIMING_CALCULATION_TYPE xdata sticks;
	
	sticks = phy_trans_sticks(ppdu_len,rate,0);				// base time stick is 1/6 of bittime;
	sticks = (sticks + XMT_SCAN_INTERVAL_STICKS)*(3-ch);
	return (u32)(sticks + SOFTWARE_DELAY_STICKS + HARDWARE_DELAY_STICKS + RCV_SCAN_MARGIN_STICKS);
}

/*******************************************************************************
* Function Name  : non_scan_expiring_sticks
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u32 non_scan_expiring_sticks(u8 rate)
{
	switch(rate & 0x03)
	{
		case 0x00:
			return (u32)(RCV_SINGLE_MARGIN_STICKS);
		case 0x01:
			return (u32)(RCV_SINGLE_MARGIN_STICKS + 1);
		default:
			return (u32)(RCV_SINGLE_MARGIN_STICKS + 2);
	}
}

#ifdef USE_DIAG
/*******************************************************************************
* Function Name  : dll_ack_expiring_sticks
* Description    : 计算链路层响应时间
* Input          : pack_len为全包长度
* Output         : 
* Return         : 
*******************************************************************************/
u32 dll_ack_expiring_sticks(BASE_LEN_TYPE ppdu_len, unsigned char rate, unsigned char scan)
// 计算发送后等待对方响应应设定超时
{
	u32 sticks;

	sticks = phy_trans_sticks(ppdu_len,rate,scan);
	if(scan)
	{
		sticks = sticks + 2 * RCV_SCAN_MARGIN_STICKS;	// 考虑对方可能由于双方底层SCAN模式下没有收到CH3导致的接收延迟
	}
	return (sticks + ACK_DELAY_STICKS + SOFTWARE_DELAY_STICKS + HARDWARE_DELAY_STICKS + RCV_RESP_MARGIN_STICKS);
}

/*******************************************************************************
* Function Name  : diag_expiring_sticks
* Description    : 计算diag响应时间, 从数据包发送完之后算起
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u32 diag_expiring_sticks(DLL_SEND_HANDLE pdss)
{
	BASE_LEN_TYPE lsdu_len;
	
	if(pdss->prop & BIT_DLL_SEND_PROP_SCAN)
	{
		lsdu_len = 8;
	}
	else
	{
		if((pdss->rmode & 0xF0) == 0xF0)
		{
			lsdu_len = 8;
		}
		else
		{
			lsdu_len = 2;
		}
	}
	return dll_ack_expiring_sticks(lsdu_len + LEN_TOTAL_OVERHEAD_BEYOND_LSDU, (pdss->rmode & 0x03), (pdss->prop & BIT_DLL_SEND_PROP_SCAN)) + 2000;	
}
#endif

/*******************************************************************************
* Function Name  : windows_delay_sticks
* Description    : get bsrf delay time
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
u32 windows_delay_sticks(BASE_LEN_TYPE ppdu_len, u8 xmode, u8 scan, u8 srf, u8 window) reentrant
{
	TIMING_CALCULATION_TYPE timing;

	if(srf)
	{
		timing = srf_trans_sticks(xmode,scan);
	}
	else
	{
		timing = phy_trans_sticks(ppdu_len,xmode,scan);
	}


	timing = timing + EBC_WINDOW_MARGIN_STICKS;
	timing = timing * window + ACK_DELAY_STICKS + RCV_SCAN_MARGIN_STICKS;
	
	return (u32)timing;
}

#if DEVICE_TYPE == DEVICE_CC || DEVICE_TYPE == DEVICE_CV
/*******************************************************************************
* Function Name  : psr_setup_expiring_sticks
* Description    : 
*					
* Input          : a string formatted as "uid[6] upmode downmode/uid[6] upmode downmode/..."
* Output         : 
* Return         : 
*******************************************************************************/
u32 psr_setup_expiring_sticks(u8 * script, u8 script_len)
{
	u8 stages;
	u32 timing;
	u8 i;
	u8 * pt;
	BASE_LEN_TYPE pack_len;
	BASE_LEN_TYPE resp_len;
	u32 pack_timing;

	stages = script_len>>3;	// stages = script_len/8
	timing = 0;
	pack_len = 18+3+1+7*(stages-1);
	resp_len = LEN_TOTAL_OVERHEAD_BEYOND_LSDU + 3; 

	// downlink timing 
	pt = &script[6];
	/*** down link timing ***/
	for(i=0; i<stages; i++)
	{
		pack_timing = phy_trans_sticks(pack_len,*pt,0);
		timing = timing + pack_timing + SOFTWARE_DELAY_STICKS + HARDWARE_DELAY_STICKS + ACK_DELAY_STICKS + PSR_STAGE_DELAY_STICKS;;

		pt += 8;
		pack_len -= 7;
		if(i==0)
		{
			timing = 0;		// first stage downlink timing is not included;
		}
	}

	/*** affirmative up link timing ***/
	pt = &script[7];
	for(i=0; i<stages; i++)
	{
		pack_timing = phy_trans_sticks(resp_len,*pt,0);
		timing = timing + pack_timing + SOFTWARE_DELAY_STICKS + HARDWARE_DELAY_STICKS + ACK_DELAY_STICKS + PSR_STAGE_DELAY_STICKS;
		pt += 8;
	}
	return (timing + PSR_MARGIN_STICKS);
}

/*******************************************************************************
* Function Name  : psr_trans_sticks
* Description    : 从源开始发送数据包开始计算, 整个pipe下行 + 上行总时间
					假设: 1. 上行下行数据包长度不变;
						  2. 所有link层的传输都是一次成功;
						  3. recepient的从接收到发出响应不需要额外时间;
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u32 psr_trans_sticks(PIPE_REF pipe, BASE_LEN_TYPE downlink_nsdu_len, BASE_LEN_TYPE uplink_nsdu_len)
{
	u8 stages;
	u32 timing;
	u8 i;
	u32 pack_timing;
	
	stages = pipe->pipe_stages;
	timing = 0;

	/*** down link timing ***/
	for(i=0; i<stages; i++)
	{
		pack_timing = phy_trans_sticks(downlink_nsdu_len+LEN_TOTAL_OVERHEAD_BEYOND_NSDU, decode_xmode(pipe->xmode_rmode[i]>>4), 0);
		timing = timing + pack_timing + SOFTWARE_DELAY_STICKS + HARDWARE_DELAY_STICKS + ACK_DELAY_STICKS + PSR_STAGE_DELAY_STICKS;;
	}

	/*** uplink timing ***/
	
	for(i=0; i<stages; i++)
	{
		pack_timing = phy_trans_sticks(uplink_nsdu_len+LEN_TOTAL_OVERHEAD_BEYOND_NSDU, decode_xmode(pipe->xmode_rmode[i]&0x0F), 0);
		timing = timing + pack_timing + SOFTWARE_DELAY_STICKS + HARDWARE_DELAY_STICKS + ACK_DELAY_STICKS + PSR_STAGE_DELAY_STICKS;
	}

	return (u32)(timing + PSR_MARGIN_STICKS);

	// worst situation: every dll_ack_send send 3 packet and get acknowledge, this consideration should be completed by the caller;
}

/*******************************************************************************
* Function Name  : psr_transaction_sticks
* Description    : 计算 
*					resp_delay:用于估算对方延时, 例如: 抄表
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u32 psr_transaction_sticks(PIPE_REF pipe, BASE_LEN_TYPE nsdu_downlink_len, BASE_LEN_TYPE nsdu_uplink_len, u16 resp_delay)
{
	u32 timing;
	
	timing = psr_trans_sticks(pipe, nsdu_downlink_len, nsdu_uplink_len) + resp_delay;
	return timing;
}

/*******************************************************************************
* Function Name  : app_transaction_sticks
* Description    : 计算有回复的app通信过程所需等待延时, 单位: 毫秒
*					resp_delay:用于估算对方延时, 例如: 抄表
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u32 app_transaction_sticks(u16 pipe_id, BASE_LEN_TYPE asdu_downlink_len, BASE_LEN_TYPE asdu_uplink_len, u16 resp_delay)
{
	u32 timing = 0;
	PIPE_REF pipe_ref;

	pipe_ref = inquire_pipe_mnpl_db(pipe_id);
	if(pipe_ref)
	{
		timing = psr_trans_sticks(pipe_ref, asdu_downlink_len+1, asdu_uplink_len+1) + resp_delay;
	}
	return timing;
}

/*******************************************************************************
* Function Name  : psr_ping_sticks
* Description    : 
*					
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u32 psr_ping_sticks(PIPE_REF pipe)
{
	u32 timing;

	timing = psr_transaction_sticks(pipe, 1, 1 + 7 + 4 + 1 + 1 + CFG_USER_DATA_BUFFER_SIZE, 0);
	return timing;
}

/*******************************************************************************
* Function Name  : psr_diag_sticks
* Description    : 
*					
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u32 psr_diag_sticks(PIPE_REF pipe_ref, DLL_SEND_HANDLE pds)
{
	u32 psr_timing;
	u32 diag_timing;
	u32 diag_resp_timing;
	u32 final_timing;

	psr_timing = psr_trans_sticks(pipe_ref, LEN_MPCI+3, LEN_MPCI+6+1+8);
	diag_timing = phy_trans_sticks (18, pds->xmode, (pds->prop&BIT_DLL_SEND_PROP_SCAN));
	diag_resp_timing = diag_expiring_sticks(pds);
	final_timing = psr_timing + diag_timing + diag_resp_timing;

	return final_timing;
}

/*******************************************************************************
* Function Name  : psr_broadcast_sticks
* Description    : 
*					
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u32 psr_broadcast_sticks(PIPE_REF pipe_ref, EBC_BROADCAST_HANDLE pt_ebc_prop)
{
	u32 psr_timing;
	u32 nbf_timing;
	u32 bsrf_resp_timing;
	u32 final_timing;

	BASE_LEN_TYPE ebc_nsdu_downlink_len;
	BASE_LEN_TYPE ebc_nsdu_uplink_len;
	BASE_LEN_TYPE broadcast_ppdu_len;

	ebc_nsdu_downlink_len = 6;			//1B MPDU_CMD; 1B broad_id; 1B xmode; 1B rmode; 1B scan; 1B window; 1B mask
	if(pt_ebc_prop->mask & BIT_NBF_MASK_ACPHASE)
	{
		ebc_nsdu_downlink_len += 1;
	}
	// ... other conditions;
	if(pt_ebc_prop->mask & BIT_NBF_MASK_BUILDID)
	{
		ebc_nsdu_downlink_len += 1;
	}
	ebc_nsdu_uplink_len = 1+6+1+1+1;	//1B MPDU_ACK_CMD; 6B uid, 1B 80; 1B broad_id; 1B bsrf_num;

	
	
	broadcast_ppdu_len = LEN_TOTAL_OVERHEAD_BEYOND_LSDU + 3; //; 1B nbf_conf; 1B nbf_id; 1B mask; 
	if(pt_ebc_prop->mask & BIT_NBF_MASK_ACPHASE)
	{
		broadcast_ppdu_len += 1;
	}
	// ... other conditions;
	if(pt_ebc_prop->mask & BIT_NBF_MASK_BUILDID)
	{
		broadcast_ppdu_len += 1;
	}


	psr_timing = psr_trans_sticks(pipe_ref, ebc_nsdu_downlink_len, ebc_nsdu_uplink_len);
	nbf_timing = phy_trans_sticks(broadcast_ppdu_len, pt_ebc_prop->xmode, pt_ebc_prop->scan);
	bsrf_resp_timing = windows_delay_sticks(4, pt_ebc_prop->rmode, pt_ebc_prop->scan, 1, (0x01<<pt_ebc_prop->window));
	final_timing = psr_timing + nbf_timing + bsrf_resp_timing;
	return final_timing;
}

/*******************************************************************************
* Function Name  : psr_identify_sticks
* Description    : 
*					
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u32 psr_identify_sticks(PIPE_REF pipe_ref, EBC_IDENTIFY_HANDLE pt_ebc_prop)
{
	u32 psr_timing;
	u32 nif_timing;
	u32 naf_timing;
	u32 ncf_timing;
	u32 final_timing;
	
	BASE_LEN_TYPE nif_ppdu_len;
	BASE_LEN_TYPE naf_ppdu_len;
	BASE_LEN_TYPE ncf_ppdu_len;

	BASE_LEN_TYPE ebc_nsdu_downlink_len;
	BASE_LEN_TYPE ebc_nsdu_uplink_len;
	
	ebc_nsdu_downlink_len = 3;		//1B MPDU_CMD; 1B broad_id; 1B bsrf_num;
	ebc_nsdu_uplink_len = 1+6+1+1+1;			//1B MPDU_ACK_CMD; 6B uid, 1B 80; 1B broad_id; 1B bsrf_num;

	nif_ppdu_len = LEN_TOTAL_OVERHEAD_BEYOND_LSDU + 3;	//1B conf; 1B NIF_BID1 ; 1B NIF_BID2
	naf_ppdu_len = LEN_TOTAL_OVERHEAD_BEYOND_LSDU + 3;	//1B conf; 1B NIF_BID1 ; 1B NIF_BID2
	ncf_ppdu_len = LEN_TOTAL_OVERHEAD_BEYOND_LSDU + 3;	//1B conf; 1B NIF_BID1 ; 1B NIF_BID2
	
	psr_timing = psr_trans_sticks(pipe_ref, ebc_nsdu_downlink_len, ebc_nsdu_uplink_len);
	nif_timing = phy_trans_sticks(nif_ppdu_len, pt_ebc_prop->xmode, pt_ebc_prop->scan);
	//naf_timing = phy_trans_sticks(naf_ppdu_len, pt_ebc_prop->rmode, pt_ebc_prop->scan);
	naf_timing = dll_ack_expiring_sticks(naf_ppdu_len, pt_ebc_prop->rmode&0x03, pt_ebc_prop->scan);
	ncf_timing = phy_trans_sticks(ncf_ppdu_len, pt_ebc_prop->xmode, pt_ebc_prop->scan);
	
	final_timing = psr_timing + (nif_timing + naf_timing + ncf_timing ) * pt_ebc_prop->num_bsrf * pt_ebc_prop->max_identify_try;

	return final_timing;
}

/*******************************************************************************
* Function Name  : psr_acquire_sticks
* Description    : 
*					
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u32 psr_acquire_sticks(PIPE_REF pipe_ref, EBC_ACQUIRE_HANDLE pt_ebc_prop)
{
	u32 psr_timing;

	BASE_LEN_TYPE ebc_nsdu_downlink_len;
	BASE_LEN_TYPE ebc_nsdu_uplink_len;

	ebc_nsdu_downlink_len = 3;
	ebc_nsdu_uplink_len = 1 + 6 + 1 + 6*pt_ebc_prop->node_num;

	psr_timing = psr_trans_sticks(pipe_ref, ebc_nsdu_downlink_len, ebc_nsdu_uplink_len);

	return psr_timing;
}

/*******************************************************************************
* Function Name  : psr_patrol_sticks
* Description    : 
*					
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u32 psr_patrol_sticks(PIPE_REF pipe_ref)
{
	u8 stages;
	u32 timing;
	u8 i;
	BASE_LEN_TYPE pack_len;
	u32 pack_timing;
	u32 ack_timing;
	

	stages = pipe_ref->pipe_stages; // stages = len/8
	timing = 0;
	pack_len = 18+3;


	/*** patrol down link timing ***/
	pack_len += 9;
	for(i=1; i<stages; i++)			// 计时从patrol包发出开始计算
	{
		pack_timing = phy_trans_sticks(pack_len, decode_xmode((pipe_ref->xmode_rmode[i]) >> 4), 0);
		ack_timing = 0;
		timing = timing + pack_timing + ack_timing + SOFTWARE_DELAY_STICKS + HARDWARE_DELAY_STICKS + ACK_DELAY_STICKS + PSR_STAGE_DELAY_STICKS;
		pack_len += 9;
	}

	/*** patrol up link timing ***/
	
	for(i=0; i<stages; i++)
	{
		pack_timing = phy_trans_sticks(pack_len, decode_xmode((pipe_ref->xmode_rmode[stages - 1 - i]) & 0x0F), 0);
		ack_timing = 0;
		timing = timing + pack_timing + ack_timing + SOFTWARE_DELAY_STICKS + HARDWARE_DELAY_STICKS + ACK_DELAY_STICKS + PSR_STAGE_DELAY_STICKS;
		pack_len += 9;
	}

	return (timing + PSR_MARGIN_STICKS);
}

/*******************************************************************************
* Function Name  : u32 dst_flooding_sticks(BASE_LEN_TYPE apdu_len)
* Description    : dst 洪泛广播下行转发的时间, 从启动发送开始计算
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
u32 dst_flooding_sticks(BASE_LEN_TYPE apdu_len)
{
	BASE_LEN_TYPE ppdu_len;
	u32 total_sticks;
	u32 sticks;

	total_sticks = 0;
	ppdu_len = apdu_len + LEN_TOTAL_OVERHEAD_BEYOND_LSDU + LEN_DST_NPCI + 1;
	
	/* 发送时间 */
	sticks = phy_trans_sticks(ppdu_len, dst_config_obj.comm_mode & 0x03, dst_config_obj.comm_mode & CHNL_SCAN);
	total_sticks += sticks;

	/* 下行洪泛传播时间 */
	sticks = windows_delay_sticks(ppdu_len, dst_config_obj.comm_mode & 0x03, dst_config_obj.comm_mode & CHNL_SCAN, 0, 0x01<<(dst_config_obj.forward_prop & BIT_DST_FORW_WINDOW));
	total_sticks += sticks * (dst_config_obj.jumps>>4);

	total_sticks += PSR_MARGIN_STICKS;

	return total_sticks;
}

/*******************************************************************************
* Function Name  : u32 dst_flooding_sticks(BASE_LEN_TYPE apdu_len)
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
u32 dst_flooding_search_sticks(void)
{
	u32 sticks,total_sticks;
	BASE_LEN_TYPE ppdu_len;
	u8 i;
	

	total_sticks = 0;
	ppdu_len = LEN_TOTAL_OVERHEAD_BEYOND_LSDU + LEN_DST_NPCI + 6 + 7;
	for(i=0;i<dst_config_obj.jumps>>4;i++)
	{
		sticks = windows_delay_sticks(ppdu_len, dst_config_obj.comm_mode & 0x03, dst_config_obj.comm_mode & CHNL_SCAN, 0, 0x01<<(dst_config_obj.forward_prop & BIT_DST_FORW_WINDOW));
		total_sticks += sticks;
		ppdu_len += 7;
	}

	sticks = phy_trans_sticks(ppdu_len, dst_config_obj.comm_mode & 0x03, dst_config_obj.comm_mode & CHNL_SCAN);
	total_sticks += sticks;
	ppdu_len += 7;

	for(i=0;i<dst_config_obj.jumps>>4;i++)
	{
		sticks = windows_delay_sticks(ppdu_len, dst_config_obj.comm_mode & 0x03, dst_config_obj.comm_mode & CHNL_SCAN, 0, 0x01<<(dst_config_obj.forward_prop & BIT_DST_FORW_WINDOW));
		total_sticks += sticks;
		ppdu_len += 7;
	}

	return total_sticks + PSR_MARGIN_STICKS;
}

/*******************************************************************************
* Function Name  : u32 dst_transaction_sticks
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
u32 dst_transaction_sticks(BASE_LEN_TYPE asdu_downlink_len, BASE_LEN_TYPE asdu_uplink_len, u16 resp_delay)
{
	u32 sticks,total_sticks;
	BASE_LEN_TYPE down_ppdu_len, up_ppdu_len;

	total_sticks = 0;
	down_ppdu_len = LEN_TOTAL_OVERHEAD_BEYOND_LSDU + LEN_DST_NPCI + 1 + asdu_downlink_len;
	up_ppdu_len = LEN_TOTAL_OVERHEAD_BEYOND_LSDU + LEN_DST_NPCI + 1 + asdu_uplink_len;

	/* 发送时间 */
	sticks = phy_trans_sticks(down_ppdu_len, dst_config_obj.comm_mode & 0x03, dst_config_obj.comm_mode & CHNL_SCAN);
	total_sticks += sticks;

	/* 下行洪泛传播时间 */
	sticks = windows_delay_sticks(down_ppdu_len, dst_config_obj.comm_mode & 0x03, dst_config_obj.comm_mode & CHNL_SCAN, 0, 0x01<<(dst_config_obj.forward_prop & BIT_DST_FORW_WINDOW));
	total_sticks += sticks * (dst_config_obj.jumps>>4);

	/* 回复时间 */
	sticks = phy_trans_sticks(up_ppdu_len, dst_config_obj.comm_mode & 0x03, dst_config_obj.comm_mode & CHNL_SCAN);
	total_sticks += sticks;

	/* 上行洪泛传播时间 */
	sticks = windows_delay_sticks(up_ppdu_len, dst_config_obj.comm_mode & 0x03, dst_config_obj.comm_mode & CHNL_SCAN, 0, 0x01<<(dst_config_obj.forward_prop & BIT_DST_FORW_WINDOW));
	total_sticks += sticks * (dst_config_obj.jumps>>4);

	return total_sticks + PSR_MARGIN_STICKS + resp_delay;
}

/*******************************************************************************
* Function Name  : u32 app_dst_expiring_sticks
* Description    : dst事务时间,从cc发送完开始计算
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
u32 app_dst_expiring_sticks(BASE_LEN_TYPE asdu_downlink_len, BASE_LEN_TYPE asdu_uplink_len, u16 resp_delay)
{
	u32 sticks, total_sticks;
	BASE_LEN_TYPE down_ppdu_len;

	down_ppdu_len = LEN_TOTAL_OVERHEAD_BEYOND_LSDU + LEN_DST_NPCI + 1 + asdu_downlink_len;
	
	total_sticks = dst_transaction_sticks(asdu_downlink_len, asdu_uplink_len, resp_delay);
	sticks = phy_trans_sticks(down_ppdu_len, dst_config_obj.comm_mode & 0x03, dst_config_obj.comm_mode & CHNL_SCAN);
	total_sticks -= sticks;

	return total_sticks;
}




#endif

/******************* (C) COPYRIGHT 2012 Belling *****END OF FILE****/

