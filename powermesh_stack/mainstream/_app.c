/******************** (C) COPYRIGHT 2016 ***************************************
* File Name          : app.c
* Author             : Lv Haifeng
* Version            : V 2.0
* Date               : 2016-05-20
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
#include "_userflash.h"
/* Private Macro--------------------------------------------------------------*/
#define PROTOCOL_ACP		0x18

#define SEC_ACP_ACPH		0x00
#define SEC_ACP_ACPR		0x01
#define SEC_ACP_DOMH		0x02
#define SEC_ACP_DOML		0x03
#define SEC_ACP_DB_BODY		0x04

#define SEC_ACP_VC_VIDH		0x04
#define SEC_ACP_VC_VIDL		0x05
#define SEC_ACP_VC_BODY		0x06

#define SEC_ACP_MC_STRH		0x04
#define SEC_ACP_MC_STRL		0x05
#define SEC_ACP_MC_ENDH		0x06
#define SEC_ACP_MC_ENDL		0x07
#define SEC_ACP_MC_SELH		0x08
#define SEC_ACP_MC_SELL		0x09
#define SEC_ACP_MC_BODY		0x0A

#define SEC_ACP_GB_GIDH		0x04
#define SEC_ACP_GB_GIDL		0x05
#define SEC_ACP_GB_BODY		0x06

#define BIT_ACP_ACPR_IDTP	0x60					//ID寻址类型
#define BIT_ACP_ACPR_FOLW	0x10
#define BIT_ACP_ACPR_TRAN	0x0F

#define BIT_ACP_ACMD_EXCEPT		0x80
#define BIT_ACP_ACMD_REQ		0x40
#define BIT_ACP_ACMD_RES		0x20
#define BIT_ACP_ACMD_CMD		0x0F

#define EXP_ACP_ACPR_IPTP_DB	0x00					//域广播
#define EXP_ACP_ACPR_IPTP_VC	0x20					//单地址
#define EXP_ACP_ACPR_IPTP_MC	0x40					//多地址
#define EXP_ACP_ACPR_IPTP_GB	0x60

/* Command ----*/
#define CMD_ACP_SET_ADDR		0x01					//设置地址
#define CMD_ACP_READ_CURR_PARA	0x02					//读当前参数
#define CMD_ACP_FRAZ_PARA		0x03					//保存冻结参数
#define CMD_ACP_READ_FRAZ_PARA	0x04					//读冻结参数
#define CMD_ACP_SET_COMM 		0x05					//设置通信参数

#define CMD_ACP_READ_CALI		0x08					//读取校正参数
#define CMD_ACP_SET_CALI		0x09					//设置校正参数
#define CMD_ACP_SAVE_CALI		0x0A					//保存校正参数

#define CMD_ACP_CALI_T			0x0B					//校正T
#define CMD_ACP_CALI_UI			0x0C					//校正电压电流

#define CMD_ACP_ERAZ_NVR		0x0D					//擦除NVR
#define CMD_ACP_READ_NVR		0x0E					//读取NVR
#define CMD_ACP_WRIT_NVR		0x0F					//写入NVR

#define BIT_ACP_CMD_MASK_T		0x04
#define BIT_ACP_CMD_MASK_U		0x02
#define BIT_ACP_CMD_MASK_I		0x01



#define EXCEPTION_HARDFAULT		0x81
#define EXCEPTION_ERRORFORMAT	0x82
#define EXCEPTION_ERRORCMD		0x83
#define EXCEPTION_VID_MISMATCH	0x84


/* private variables ---------------------------------------------------------*/
u16 xdata _self_domain; 
u16 xdata _self_vid;
u16 xdata _self_gid;

SEND_ID_TYPE xdata ass_send_id;
static u8 _acp_index[CFG_PHASE_CNT];

#if DEVICE_TYPE==DEVICE_CV
u8 _comm_mode = CHNL_CH3;
#endif

/* Private Functions Prototype ---------------------------------------*/
u8 wsp_cmd_proc(ARRAY_HANDLE body, u8 body_len, ARRAY_HANDLE return_buffer);
STATUS app_call(UID_HANDLE target_uid, ARRAY_HANDLE apdu, u8 apdu_len, ARRAY_HANDLE return_buffer, u8 return_len);
u8 get_acp_index(u8 phase);
STATUS acp_call_by_uid(UID_HANDLE target_uid, ARRAY_HANDLE cmd_body, u8 cmd_body_len, u8* return_buffer, u8 return_cmd_body_len);

/* Functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : init_app()
* Description    : All addresses would be 0xFFFF if they were not assigned by CV.
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void init_app(void)
{
	if(is_app_nvr_data_valid())
	{
		_self_domain = get_app_nvr_data_domain_id();
		_self_vid = get_app_nvr_data_vid();
		_self_gid = get_app_nvr_data_gid();
	}
	else
	{
		_self_domain = 0xFFFF;
		_self_vid = 0xFFFF;
		_self_gid = 0xFFFF;
	}
	my_printf("Domain:%X, VID:%X, GID:%X\r\n",_self_domain,_self_vid,_self_gid);
}


/*******************************************************************************
* Function Name  : save_id_info_into_app_nvr()
* Description    : save app assigned address to nvr
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS save_id_info_into_app_nvr(void)
{
	set_app_nvr_data_domain_id(_self_domain);
	set_app_nvr_data_vid(_self_vid);
	set_app_nvr_data_gid(_self_gid);

	return save_app_nvr_data();
}


/*******************************************************************************
* Function Name  : calc_mc_response_pending_sticks()
* Description    : mc broadcast revoke multiple responses. this function calculate the time that its response should be delayed if self nodes
					is in the calling range.
* Input          : mc_index: mc window index 
* Output         : 
* Return         : 
*******************************************************************************/
u32 calc_mc_response_pending_sticks(APP_RCV_HANDLE pt_app_rcv, u16 app_len, u16 mc_index)
{
	u32 sticks = ACK_DELAY_STICKS;
	u16 ppdu_len;

	if(pt_app_rcv->protocol == PROTOCOL_DST)
	{
		if(mc_index)
		{
			ppdu_len = app_len + LEN_PPCI + LEN_LPCI + LEN_DST_NPCI + LEN_MPCI + 10 + 1; //IPTP MC App overhead is 11 bytes (including CS).
			sticks = phy_trans_sticks(ppdu_len,get_ptp_comm_mode() & 0x03,get_ptp_comm_mode() & CHNL_SCAN);
			sticks += APP_MC_MARGIN_STICKS;
			sticks = sticks * mc_index;
			sticks += APP_MC_MARGIN_STICKS + APP_MC_INITIAL_STICKS;	//for mc_index >0, plus an aditional delay to cancel out packet preparation timing expense
		}
		else
		{
			sticks = APP_MC_INITIAL_STICKS;
		}
		
	}
	
	return sticks;
}


/*******************************************************************************
* Function Name  : acp_rcv_proc()
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void acp_rcv_proc(APP_RCV_HANDLE pt_app_rcv)
{

	ARRAY_HANDLE apdu;
	u8 apdu_len;
	ARRAY_HANDLE return_buffer;
	u8 ret_len;
	u8 head_len;

	apdu = pt_app_rcv->apdu;
	apdu_len = pt_app_rcv->apdu_len;

	if(!check_cs(apdu, apdu_len))
	{
		return;
	}
	else
	{
		// req resource
		return_buffer = OSMemGet(SUPERIOR);
		if(return_buffer == NULL)
		{
			return;
		}

		if(apdu[SEC_ACP_ACPH] == PROTOCOL_ACP)
		{
			u16 target_xid;
			u16 caller_vid;

			target_xid = (apdu[SEC_ACP_VC_VIDH] << 8) + apdu[SEC_ACP_VC_VIDL];

			if(((apdu[SEC_ACP_DOMH] == (u8)(_self_domain>>8)) && (apdu[SEC_ACP_DOML] == ((u8)_self_domain))) || ((apdu[SEC_ACP_DOMH] == 0) && (apdu[SEC_ACP_DOML] == 0)))
			{
				u8 idtp;
				u8 req = 0;
				BOOL id_match = FALSE;
				ARRAY_HANDLE pt_body;
				
				idtp = (apdu[SEC_ACP_ACPR]) & BIT_ACP_ACPR_IDTP;
				switch(idtp)
				{
					case(EXP_ACP_ACPR_IPTP_DB):								//domain broadcast
					{
						pt_body = &apdu[SEC_ACP_DB_BODY];
						head_len = SEC_ACP_DB_BODY;
						if(!(*pt_body & BIT_ACP_ACMD_RES))					//make sure it is not res frame
						{
							id_match = TRUE;
							req = apdu[SEC_ACP_DB_BODY] & BIT_ACP_ACMD_REQ;
						}
						break;
					}
					case(EXP_ACP_ACPR_IPTP_VC):								//single node
					{
						if((target_xid == _self_vid) || (target_xid == 0))
						{
							id_match = TRUE;
							req = apdu[SEC_ACP_VC_BODY] & BIT_ACP_ACMD_REQ;
							pt_body = &apdu[SEC_ACP_VC_BODY];
							head_len = SEC_ACP_VC_BODY;
						}
						break;
					}

					/* in IPTP_MC type, one call can be responsed by multiple nodes one by one. And a node could miss call but be revoked by other nodes' responses
					   if only its vid is in the calling range */
					case(EXP_ACP_ACPR_IPTP_MC):								//multiple nodes
					{
						u16 end_vid;

						end_vid = (u16)(apdu[SEC_ACP_MC_ENDH] << 8) + apdu[SEC_ACP_MC_ENDL];
						caller_vid = (u16)(apdu[SEC_ACP_MC_SELH] << 8) + apdu[SEC_ACP_MC_SELL];
						if(_self_vid<end_vid)	//target_xid is start_vid now; selected range is [start_vid,end_vid), like Python.
						{
							if(caller_vid ? (_self_vid>caller_vid) : (_self_vid>=target_xid))	//call_vid is not zero means it's a mc response from other SE node
							{
								req = apdu[SEC_ACP_MC_BODY] & (BIT_ACP_ACMD_REQ|BIT_ACP_ACMD_RES);	// in MC type, req frame and res frame can all invoke response
								id_match = TRUE;
								pt_body = &apdu[SEC_ACP_MC_BODY];
								head_len = SEC_ACP_MC_BODY;
							}
						}
						break;
					}
					case(EXP_ACP_ACPR_IPTP_GB):
					{
						if((target_xid == _self_gid) || (target_xid == 0))
						{
							req = apdu[SEC_ACP_GB_BODY] & BIT_ACP_ACMD_REQ;
							id_match = TRUE;
							pt_body = &apdu[SEC_ACP_GB_BODY];
							head_len = SEC_ACP_GB_BODY;
						}
						break;
					}
					default:
					{
						break;
					}
				}

				if(id_match)
				{
					APP_SEND_STRUCT xdata ass;
					u32 mc_pending_sticks;
					u16 mc_response_window_index;

					ret_len = wsp_cmd_proc(pt_body, apdu_len - head_len, &return_buffer[head_len]);		// 包括了cs, 无所谓了

					if(req)
					{
						if(idtp == EXP_ACP_ACPR_IPTP_MC)
						{
							if(apdu[SEC_ACP_MC_BODY] & BIT_ACP_ACMD_REQ)	//if revoker is not CV, resp index should be calced by the caller_vid
							{
								mc_response_window_index = _self_vid - target_xid;
							}
							else
							{
								if(_self_vid > caller_vid)
								{
									mc_response_window_index = _self_vid - caller_vid - 1;		//_self_vid must larger than call_vid
								}
								else
								{
									my_printf("error! caller_vid:%x, _self_vid:%x!\r\n",_self_vid,caller_vid);
								}
							}
						
							mc_pending_sticks = calc_mc_response_pending_sticks(pt_app_rcv, ret_len, mc_response_window_index);
							my_printf("mc response was adjusted into index %u, pending %u\r\n", mc_response_window_index, mc_pending_sticks);
						}

						/* if ass_send_id is in pending status, it means mc response has been pushed in the queue, all need to do is just its */
						if(idtp == EXP_ACP_ACPR_IPTP_MC && get_send_status(ass_send_id)==SEND_STATUS_PENDING)
						{
							set_queue_delay(ass_send_id, mc_pending_sticks);
						}
						else
						{
							mem_cpy(return_buffer, apdu, head_len);
							ret_len += head_len;
							return_buffer[SEC_ACP_ACPR] |= BIT_ACP_ACPR_FOLW;
							if(idtp == EXP_ACP_ACPR_IPTP_MC)
							{
								return_buffer[SEC_ACP_MC_SELH] = _self_vid>>8;
								return_buffer[SEC_ACP_MC_SELL] = _self_vid;
							}

							return_buffer[ret_len] = calc_cs(return_buffer, ret_len);
							ret_len++;
							
							ass.phase = pt_app_rcv->phase;
							ass.protocol = pt_app_rcv->protocol;
							ass.pipe_id = pt_app_rcv->pipe_id;
							
							ass.apdu = return_buffer;
							ass.apdu_len = ret_len;
							ass_send_id = app_send(&ass);
							if(ass_send_id != INVALID_RESOURCE_ID && idtp == EXP_ACP_ACPR_IPTP_MC)
							{
								set_queue_delay(ass_send_id, mc_pending_sticks);
							}
						}
					}
				}
			}
		}
		OSMemPut(SUPERIOR,return_buffer);
	}
	return;
}

/*******************************************************************************
* Function Name  : check_passwd()
* Description    : for sensitive and dangerous operation(i.e. set addr), check passwd
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
const u8 __passwd__[] = {0x95, 0x27};

BOOL check_passwd(ARRAY_HANDLE passwd)
{
	const u8 * ptr = __passwd__;
	u8 i;
	for(i=0;i<sizeof(__passwd__);i++)
	{
		if(*passwd++ != *ptr++)
		{
			return FALSE;
		}
	}
	return TRUE;
}


/*******************************************************************************
* Function Name  : wsp_cmd_proc()
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u8 wsp_cmd_proc(ARRAY_HANDLE body, u8 body_len, ARRAY_HANDLE return_buffer)
{
	u8 cmd;
	u8 ret_len = 0;
	u8 mask;
	ARRAY_HANDLE ptw;
	s16 para;
	u16 temp;
	BOOL update_nvr = FALSE;
	ARRAY_HANDLE body_bkp = body;

	cmd = *body++;
	ptw = return_buffer;
	
	*ptw++ = cmd & (~BIT_ACP_ACMD_REQ) | BIT_ACP_ACMD_RES;
	ret_len = 1;

	switch(cmd & BIT_ACP_ACMD_CMD)
	{
		case(CMD_ACP_SET_ADDR):
		{
			if(!(cmd & BIT_ACP_ACMD_RES))					//avoid set address by MC res frame, that would be big error!
			{
				if(body_len>= 9 && check_passwd(&body_bkp[body_len-3]))		//format: 0x01 domain_id vid group_id passwd cs
				{
					temp = (*body++ << 8);
					temp += *body++;
					if(temp != 0xFFFF && temp != _self_domain)
					{
						_self_domain = temp;
						update_nvr = TRUE;
					}

					temp = (*body++ << 8);
					temp += *body++;
					if(temp != 0xFFFF && temp != _self_vid)
					{
						_self_vid = temp;
						update_nvr = TRUE;
					}

					temp = (*body++ << 8);
					temp += *body++;
					if(temp != 0xFFFF && temp != _self_gid)
					{
						_self_gid = temp;
						update_nvr = TRUE;
					}

					if(update_nvr)
					{
						save_id_info_into_app_nvr();
					}
				}
			}

			*ptw++ = _self_domain>>8;
			*ptw++ = _self_domain;
			*ptw++ = _self_vid>>8;
			*ptw++ = _self_vid;
			*ptw++ = _self_gid>>8;
			*ptw++ = _self_gid;
			ret_len += 6;
			
			break;
		}
		case(CMD_ACP_READ_CURR_PARA):
		{
			mask = *body++;
			//if(mask & BIT_ACP_CMD_MASK_T)			//measure current temperature
			//{
			//	para = measure_current_t();
			//	*ptw++ = (u8)(para>>8);
			//	*ptw++ = (u8)(para);
			//	ret_len += 2;
			//}
			if(mask & BIT_ACP_CMD_MASK_U)			//measure current voltage
			{
				para = measure_current_v();
				*ptw++ = (u8)(para>>8);
				*ptw++ = (u8)(para);
				ret_len += 2;
			}
			if(mask & BIT_ACP_CMD_MASK_I)			//measure current i
			{
				para = measure_current_i();
				*ptw++ = (u8)(para>>8);
				*ptw++ = (u8)(para);
				ret_len += 2;
			}
			break;
		}

		case(CMD_ACP_SAVE_CALI):
		{
			
			break;
		}
		
		case(CMD_ACP_ERAZ_NVR):
		{
			erase_user_storage();
			break;
		}
		case(CMD_ACP_READ_NVR):
		{
			read_user_storage(ptw,*body);		//body now is read length
			ret_len = *body;
			break;
		}
		case(CMD_ACP_WRIT_NVR):
		{
			write_user_storage(body, body_len-1);
			ret_len = 0;
			
			//if(compare_user_storage(body, body_len-1))
			//{
			//	ret_len = 0;
			//}
			//else
			//{
			//	*ptw = EXCEPTION_ERRORCMD;
			//	ret_len = 1;
			//}
			break;
		}
		default:
		{
			return_buffer[0] = EXCEPTION_ERRORCMD;
			ret_len = 1;
			break;
		}
	}
	return ret_len;
}

#if DEVICE_TYPE==DEVICE_CV

/*******************************************************************************
* Function Name  : set_comm_mode()
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void set_comm_mode(u8 comm_mode)
{
	_comm_mode = comm_mode;
}


/*******************************************************************************
* Function Name  : call_vid_for_current_parameter()
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS call_vid_for_current_parameter(UID_HANDLE target_uid, u8 mask, s16* return_parameter)
{
	u8 cmd_body[2];
	u8 cmd_body_len;

	u8 return_body[6];
	u8 i;
	u8 cnt;

	u16 parameter;

	
	ARRAY_HANDLE ptw, ptr;
	STATUS status;

	ptw = cmd_body;
	*ptw++ = CMD_ACP_READ_CURR_PARA | BIT_ACP_ACMD_REQ;
	*ptw++ = mask;
	cmd_body_len = 2;
	
	cnt = 0;
	for(i=0x80;i>0;i>>=1)
	{
		(mask&i) ? cnt++ : 0;
	}

	status = acp_call_by_uid(target_uid, cmd_body, cmd_body_len, return_body, cnt*2+1);
	if(status)
	{
		ptr = return_body;
		ptr++;
		for(i=0x80;i>0;i>>=1)
		{
			if(mask & i)
			{
				parameter = (*ptr++<<8);
				parameter += *ptr++;
				*return_parameter++ = (s16)parameter;
			}
		}
	}
	return status;
}

const char *_exception_code[]=
	{
		"Error Exception ID",
		"Hardware Fault",
		"Format Error",
		"Unexecutable Command",
		"Unmatched ID"
	};


const char * exception_meaning(u8 exception_id)
{
	if(exception_id>=sizeof(_exception_code))
	{
		return _exception_code[0];
	}
	else
	{
		return _exception_code[exception_id];
	}
}


/*******************************************************************************
* Function Name  : acp_call_by_uid()
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS acp_call_by_uid(UID_HANDLE target_uid, ARRAY_HANDLE cmd_body, u8 cmd_body_len, u8* return_buffer, u8 return_cmd_body_len)
{
	u8 send_apdu[100];
	u8 rec_apdu[100];
	ARRAY_HANDLE ptw;
	u8 send_apdu_len;
	STATUS status;

	ptw = send_apdu;
	
	*ptw++ = PROTOCOL_ACP;
	*ptw++ = EXP_ACP_ACPR_IPTP_VC | get_acp_index(0);
	*ptw++ = 0;
	*ptw++ = 0;											//domain
	*ptw++ = 0;
	*ptw++ = 0;											//vid
	mem_cpy(ptw, cmd_body, cmd_body_len);
	send_apdu_len = 6 + cmd_body_len;
	send_apdu[send_apdu_len] = calc_cs(send_apdu, send_apdu_len);
	send_apdu_len++;

	status = app_call(target_uid, send_apdu, send_apdu_len, rec_apdu, return_cmd_body_len + 7);
	if(status)
	{
		if(rec_apdu[SEC_ACP_VC_BODY] & BIT_ACP_ACMD_EXCEPT)
		{
			my_printf("Exception occured! ID:%bd(%s)\r\n",rec_apdu[SEC_ACP_VC_BODY]&BIT_ACP_ACMD_CMD,exception_meaning(rec_apdu[SEC_ACP_VC_BODY]&BIT_ACP_ACMD_CMD));
			status = FAIL;
		}
		else
		{
			mem_cpy(return_buffer, &rec_apdu[SEC_ACP_VC_BODY], return_cmd_body_len);
		}
	}
	return status;
}



/*******************************************************************************
* Function Name  : get_acp_index()
* Description    : 用app协议, 权宜之计
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
u8 get_acp_index(u8 phase)
{
	return ((_acp_index[phase]++) & BIT_ACP_ACPR_TRAN);
}

/*******************************************************************************
* Function Name  : acp_send()
* Description    : if in acp_send_struct, uid is NULL, it is an broadcast frame
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
SEND_ID_TYPE acp_send(ACP_SEND_HANDLE ptr_acp)
{
	APP_SEND_STRUCT ass;
	ARRAY_HANDLE apdu;
	u8 apdu_len;
	SEND_ID_TYPE sid;

	sid = INVALID_RESOURCE_ID;
	apdu = OSMemGet(SUPERIOR);
	if(apdu)
	{
		u8 * ptw;

		ptw = apdu;
		*ptw++ = PROTOCOL_ACP;
		*ptw = ptr_acp->follow ? (BIT_ACP_ACPR_FOLW|(ptr_acp->tran_id&BIT_ACP_ACPR_TRAN)) : get_acp_index(0);


		switch(ptr_acp->acp_idtp)
		{
			case(ACP_IDTP_DB):
			{
				*ptw++ |= EXP_ACP_ACPR_IPTP_DB;
				*ptw++ = ptr_acp->domain_id>>8;
				*ptw++ = ptr_acp->domain_id;
				break;
			}
			case(ACP_IDTP_VC):
			{
				*ptw++ |= EXP_ACP_ACPR_IPTP_VC;
				*ptw++ = ptr_acp->domain_id>>8;
				*ptw++ = ptr_acp->domain_id;
				*ptw++ = ptr_acp->vid>>8;
				*ptw++ = ptr_acp->vid;
				break;
			}
			case(ACP_IDTP_MC):
			{
				*ptw++ |= EXP_ACP_ACPR_IPTP_MC;
				*ptw++ = ptr_acp->domain_id>>8;
				*ptw++ = ptr_acp->domain_id;
				*ptw++ = ptr_acp->start_vid>>8;
				*ptw++ = ptr_acp->start_vid;
				*ptw++ = ptr_acp->end_vid>>8;
				*ptw++ = ptr_acp->end_vid;
				*ptw++ = ptr_acp->self_vid>>8;
				*ptw++ = ptr_acp->self_vid;
				break;
			}
			default:				//ACP_IDTP_GB
			{
				*ptw++ |= EXP_ACP_ACPR_IPTP_GB;
				*ptw++ = ptr_acp->domain_id>>8;
				*ptw++ = ptr_acp->domain_id;
				*ptw++ = ptr_acp->gid>>8;
				*ptw++ = ptr_acp->gid;
				break;
			}
		}

		mem_cpy(ptw,ptr_acp->acp_body,ptr_acp->acp_body_len);
		ptw += ptr_acp->acp_body_len;
		apdu_len = (u8)(ptw-apdu);
		*ptw++ = calc_cs(apdu, apdu_len);
		apdu_len++;

		ass.phase = 0;
		ass.apdu = apdu;
		ass.apdu_len = apdu_len;
		ass.protocol = PROTOCOL_PTP;
		if(ptr_acp->target_uid)
		{
			mem_cpy(ass.target_uid, ptr_acp->target_uid, 6);
		}
		else
		{
			mem_clr(ass.target_uid, sizeof(ass.target_uid), 1);	//if in acp_send_struct, uid is NULL, it is an broadcast frame
		}
		
		sid = app_send(&ass);
		OSMemPut(SUPERIOR,apdu);
	}

	return sid;
}


/*******************************************************************************
* Function Name  : acp_transaction
* Description    : send a acp apdu, 

					1. if resp_buffer is not NULL and expected_wsp_res_bytes is not 0,
						wait for just one response (not for mc) , copy return into return buffer
						indicate return bytes in acp struct;

					2. if expected_wsp_res_bytes is not 0, and resp_buffer is NULL
						wait for just one response, not copy return bytes.
						indicate return bytes in acp struct;

					3. if expected_wsp_res_bytes is 0
						just send frame and return, that means this frame is a inform or broadcast no need to response;

* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
STATUS acp_transaction(ACP_SEND_HANDLE acp)
{
	APP_RCV_STRUCT app_rcv_obj;
	ARRAY_HANDLE apdu_rcv;
	u8 apdu_rcv_len;
	u32 time_out;
	SEND_ID_TYPE sid;
	STATUS status = FAIL;
	TIMER_ID_TYPE tid;
	u8 tran_id;
	u8 wsp_command;
	u8 wsp_command_index;

	switch(acp->acp_idtp)
	{
		case(ACP_IDTP_DB):
		{
			apdu_rcv_len = acp->expected_wsp_res_bytes + SEC_ACP_DB_BODY + 1;
			wsp_command_index = SEC_ACP_DB_BODY;
			break;
		}
		case(ACP_IDTP_VC):
		{
			apdu_rcv_len = acp->expected_wsp_res_bytes + SEC_ACP_VC_BODY + 1;
			wsp_command_index = SEC_ACP_VC_BODY;
			break;
		}
		case(ACP_IDTP_MC):
		{
			apdu_rcv_len = acp->expected_wsp_res_bytes + SEC_ACP_MC_BODY + 1;
			wsp_command_index = SEC_ACP_MC_BODY;
			break;
		}
		default:
		{
			apdu_rcv_len = acp->expected_wsp_res_bytes + SEC_ACP_GB_BODY + 1;
			wsp_command_index = SEC_ACP_GB_BODY;
			break;
		}
	}

	if(acp->acp_body_len && acp->expected_wsp_res_bytes)
	{
		acp->acp_body[0] |= BIT_ACP_ACMD_REQ;
	}

	sid = acp_send(acp);
	if(sid != INVALID_RESOURCE_ID)
	{
		if(acp->expected_wsp_res_bytes == 0)		//if wait bytes is zero, it's an inform action (no resp)
		{
			return OKAY;
		}

		apdu_rcv = OSMemGet(SUPERIOR);
		if(apdu_rcv)
		{
			app_rcv_obj.apdu = apdu_rcv;
			time_out = ptp_transmission_sticks(apdu_rcv_len);
			tran_id = (_acp_index[0] - 1) & BIT_ACP_ACPR_TRAN;
			wsp_command = acp->acp_body[0];
			acp->actual_wsp_res_bytes = 0;
			
			wait_until_send_over(sid);

			tid = req_timer();
			if(tid != INVALID_RESOURCE_ID)
			{
				set_timer(tid,time_out);
				my_printf("set timeout:%d ms\r\n", time_out);
				while(!is_timer_over(tid))
				{
					/******************************************
					* response match condition:
					* 1. idtp match
					* 2. tran id match
					* 3. wsp command match
					******************************************/
					if(app_rcv(&app_rcv_obj))
					{
						if((apdu_rcv[1]>>5) == acp->acp_idtp)
						{
							if((apdu_rcv[SEC_ACP_ACPR]&BIT_ACP_ACPR_TRAN) == tran_id)	
							{
								u8 res_wsp_len;
								if(apdu_rcv[wsp_command_index] & BIT_ACP_ACMD_EXCEPT)
								{
									my_printf("exception occurred\r\n");
									res_wsp_len = app_rcv_obj.apdu_len - wsp_command_index - 1;
									acp->actual_wsp_res_bytes = res_wsp_len;
									if(acp->resp_buffer)
									{
										if(acp->expected_wsp_res_bytes>=res_wsp_len)
										{
											mem_cpy(acp->resp_buffer, &apdu_rcv[wsp_command_index], res_wsp_len);
											status = FAIL;
										}
									}
									break;
								}
								else if((wsp_command & BIT_ACP_ACMD_CMD) == (apdu_rcv[wsp_command_index] & BIT_ACP_ACMD_CMD) &&
									(apdu_rcv[wsp_command_index] & BIT_ACP_ACMD_RES))
								{
									res_wsp_len = app_rcv_obj.apdu_len - wsp_command_index - 1;
									acp->actual_wsp_res_bytes = res_wsp_len;
									if(acp->resp_buffer)
									{
										if(acp->expected_wsp_res_bytes>=res_wsp_len)
										{
											mem_cpy(acp->resp_buffer, &apdu_rcv[wsp_command_index], res_wsp_len);
											status = OKAY;
										}
										else
										{
											my_printf("acp rcv is correct, but rec buffer is not enough\r\n");
										}
									}
									break;
								}
								else
								{
									my_printf("irrelative apdu was abandoned\r\n");
									uart_send_asc(app_rcv_obj.apdu,app_rcv_obj.apdu_len);
								}
							}
						}
					}
				}
				if(!status && !acp->actual_wsp_res_bytes)
				{
					my_printf("req time out\r\n");
				}
				delete_timer(tid);
			}
			OSMemPut(SUPERIOR, apdu_rcv);
		}
	}
	return status;
}


/*******************************************************************************
* Function Name  : acp_req_by_uid()
* Description    : request response by specified UID(using idtp = ACP_IDTP_DB domain_id = 0)
					request response was written in resp_buffer[], caller must make sure it is big enough 
					i.e. not smaller than expected_res_bytes.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
u8 acp_req_by_uid(UID_HANDLE target_uid, ARRAY_HANDLE wsp_command, u8 wsp_len, u8 * resp_buffer, u8 expected_res_bytes)
{
	ACP_SEND_STRUCT acp;

	acp.target_uid = target_uid;
	acp.acp_idtp = ACP_IDTP_DB;
	acp.domain_id = 0;
	acp.follow = FALSE;
	acp.acp_body = wsp_command;
	acp.acp_body_len = wsp_len;
	acp.resp_buffer = resp_buffer;
	acp.expected_wsp_res_bytes = expected_res_bytes;

	acp_transaction(&acp);

	return acp.actual_wsp_res_bytes;		//if fail, return 0
}

/*******************************************************************************
* Function Name  : acp_req_by_vid()
* Description    : request response by Domain ID and VID (target UID can be 0xFFFFFFFFFFFF or specified UID)
					request response was written in resp_buffer[], caller must make sure it is big enough 
					i.e. not smaller than expected_res_bytes.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
u8 acp_req_by_vid(UID_HANDLE target_uid, u16 domain_id, u16 vid, ARRAY_HANDLE wsp_command, u8 wsp_len, u8 * resp_buffer, u8 expected_res_bytes)
{
	ACP_SEND_STRUCT acp;

	acp.target_uid = target_uid;
	acp.acp_idtp = ACP_IDTP_VC;
	acp.domain_id = domain_id;
	acp.vid = vid;
	acp.follow = FALSE;
	acp.acp_body = wsp_command;
	acp.acp_body_len = wsp_len;
	acp.resp_buffer = resp_buffer;
	acp.expected_wsp_res_bytes = expected_res_bytes;

	acp_transaction(&acp);

	return acp.actual_wsp_res_bytes;
}

/*******************************************************************************
* Function Name  : acp_domain_broadcast()
* Description    : broadcast to domain.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
STATUS acp_domain_broadcast(u16 domain_id, ARRAY_HANDLE wsp_command, u8 wsp_len)
{
	ACP_SEND_STRUCT acp;

	acp.target_uid = NULL;
	acp.acp_idtp = ACP_IDTP_DB;
	acp.domain_id = domain_id;
	acp.follow = FALSE;
	acp.acp_body = wsp_command;
	acp.acp_body_len = wsp_len;
	acp.resp_buffer = NULL;
	acp.expected_wsp_res_bytes = 0;

	return acp_transaction(&acp);
}

/*******************************************************************************
* Function Name  : acp_domain_broadcast()
* Description    : broadcast to domain.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
STATUS acp_inform_by_vid(u16 domain_id, u16 vid, ARRAY_HANDLE wsp_command, u8 wsp_len)
{
	ACP_SEND_STRUCT acp;

	acp.target_uid = NULL;
	acp.acp_idtp = ACP_IDTP_VC;
	acp.domain_id = domain_id;
	acp.vid = vid;
	acp.follow = FALSE;
	acp.acp_body = wsp_command;
	acp.acp_body_len = wsp_len;
	acp.resp_buffer = NULL;
	acp.expected_wsp_res_bytes = 0;

	return acp_transaction(&acp);
}

/*******************************************************************************
* Function Name  : acp_req_mc()
* Description    : multiple req
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
STATUS acp_req_mc(u16 domain_id, u16 start_vid, u16 end_vid, ARRAY_HANDLE wsp_command, u8 wsp_len, ARRAY_HANDLE resp_buffer, u8 expected_res_bytes)
{
	ACP_SEND_STRUCT acp;

	if(end_vid<=start_vid)
	{
		my_printf("start_vid(%x) must smaller than end_vid(%x)\r\n",start_vid,end_vid);
		return FAIL;
	}

	acp.target_uid = NULL;
	acp.acp_idtp = ACP_IDTP_MC;
	acp.domain_id = domain_id;
	acp.start_vid = start_vid;
	acp.end_vid = end_vid;
	acp.self_vid = _self_vid;
	
	acp.follow = FALSE;
	acp.acp_body = wsp_command;
	acp.acp_body_len = wsp_len;
	acp.resp_buffer = resp_buffer;
	acp.expected_wsp_res_bytes = expected_res_bytes;

	return acp_transaction(&acp);
}




/******************************************************************************
TODO:
	acp_group_broadcast
******************************************************************************/




/*******************************************************************************
* Function Name  : set_se_node_id_by_uid()
* Description    : set domain id, individual id, group id by node UID. if any id (domain id, group id, vid) 
					is not intended to be changed, set it as 0xFFFF
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
STATUS set_se_node_id_by_uid(ARRAY_HANDLE target_uid, u16 domain_id, u16 vid, u16 gid)
{
	u8 body[7];
	u8 rcv_buffer[7];
	u8 return_bytes;

	body[0] = CMD_ACP_SET_ADDR;
	body[1] = domain_id>>8;
	body[2] = domain_id;
	body[3] = vid>>8;
	body[4] = vid;
	body[5] = gid>>8;
	body[6] = gid;

	return_bytes = acp_req_by_uid(target_uid, body, sizeof(body), rcv_buffer, sizeof(rcv_buffer));
	
	if(return_bytes)
	{
		return OKAY;
	}

	return FAIL;
}

/*******************************************************************************
* Function Name  : app_call()
* Description    : 用app协议, 权宜之计
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
STATUS app_call(UID_HANDLE target_uid, ARRAY_HANDLE apdu, u8 apdu_len, ARRAY_HANDLE return_buffer, u8 return_len)
{
	APP_SEND_STRUCT ass;
	APP_RCV_STRUCT ars;
	SEND_ID_TYPE sid = INVALID_RESOURCE_ID;
	u32 time_out;
	STATUS status = FAIL;


	ass.phase = 0;
	ass.protocol = PROTOCOL_PTP;
	mem_cpy(ass.target_uid, target_uid, 6);
	ass.apdu = apdu;
	ass.apdu_len = apdu_len;

	sid = app_send(&ass);

	if(sid==INVALID_RESOURCE_ID)
	{
		return FAIL;
	}
	else
	{
		TIMER_ID_TYPE tid;
		tid = req_timer();
		if(tid==INVALID_RESOURCE_ID)
		{
			return FAIL;
		}
		else
		{
			wait_until_send_over(sid);
			time_out = atp_transaction_sticks(apdu_len, return_len, 1000);
			set_timer(tid,time_out);

			ars.apdu = return_buffer;

			do
			{
				powermesh_event_proc();
				apdu_len = app_rcv(&ars);
				if(apdu_len)
				{
					if(ars.protocol == PROTOCOL_PTP)
					{
						if(check_cs(return_buffer, apdu_len) && (check_uid(0,target_uid) || mem_cmp(target_uid,ars.src_uid,6)))
						{
my_printf("got apdu: ");
uart_send_asc(ars.apdu,apdu_len);
my_printf("\r\n");

							status = OKAY;
							break;
						}
					}
				}
				
			}while(!is_timer_over(tid));
			delete_timer(tid);
		}
	}
	return status;
}


///*******************************************************************************
//* Function Name  : app_direct_send()
//* Description    : 借用dst协议的形式, 实现点对点通信, 
//					dll层给具体的uid作为目的地址, dst禁止洪泛转发和acps等
//					调用之前需要调用方指定使用DST协议, 并正常调用config_dst_flooding
//* Input          : None
//* Output         : None
//* Return         : None
//*******************************************************************************/
//SEND_ID_TYPE app_direct_send(APP_SEND_HANDLE ass)
//{
//	DLL_SEND_STRUCT dss;
//	SEND_ID_TYPE sid = INVALID_RESOURCE_ID;
//	ARRAY_HANDLE lsdu = NULL;

//	if(ass->protocol == PROTOCOL_DST)
//	{
//		lsdu = OSMemGet(SUPERIOR);
//		if(!lsdu)
//		{
//			return INVALID_RESOURCE_ID;
//		}

//		mem_clr((u8*)(&dss),sizeof(DLL_SEND_STRUCT),1);

//		dss.phase = ass->phase;
//		dss.target_uid_handle = ass->target_uid;

//		mem_cpy(lsdu+LEN_DST_NPCI+LEN_MPCI, ass->apdu, ass->apdu_len);	//直接在原apdu上操作有风险

//		dss.lsdu = lsdu;
//		dss.lsdu_len = ass->apdu_len + LEN_DST_NPCI+LEN_MPCI;
//		
//		dss.lsdu[0] = CST_DST_PROTOCOL | get_dst_index(dss.phase);		//dst head[0]: conf
//		dss.lsdu[1] = 0;												//dst head[1]: jump
//		dss.lsdu[2] = 0;												//dst head[2]: forward
//		dss.lsdu[3] = 0;												//dst head[3]: acps
//		dss.lsdu[4] = 0;												//dst head[4]: build_id
//		dss.lsdu[5] = 0;												//dst head[5]: timing_stamp
//		dss.lsdu[6] = 0;												//mpdu head
//		dss.prop |= (dst_config_obj.comm_mode & CHNL_SCAN)?BIT_DLL_SEND_PROP_SCAN:0;
//		dss.xmode = dst_config_obj.comm_mode & 0xF3;
//		dss.delay = 0;
//		sid = dll_send(&dss);

//		OSMemPut(SUPERIOR,lsdu);
//	}
//	return sid;
//}

#endif
