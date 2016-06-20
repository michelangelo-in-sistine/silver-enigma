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


extern DST_CONFIG_STRUCT xdata dst_config_obj;

/* private variables ---------------------------------------------------------*/
u16 xdata _self_domain; 
u16 xdata _self_vid;
u16 xdata _self_gid;

SEND_ID_TYPE xdata ass_send_id;

/* Private Functions Prototype ---------------------------------------*/
u8 wsp_cmd_proc(ARRAY_HANDLE body, u8 body_len, ARRAY_HANDLE return_buffer);
SEND_ID_TYPE app_direct_send(APP_SEND_HANDLE ass);
STATUS app_call(UID_HANDLE target_uid, ARRAY_HANDLE apdu, u8 apdu_len, ARRAY_HANDLE return_buffer, u8 return_len);
u8 get_acp_index(u8 phase);
STATUS acp_call_by_uid(UID_HANDLE target_uid, ARRAY_HANDLE cmd_body, u8 cmd_body_len, u8* return_buffer, u8 return_cmd_body_len);

/* Functions ---------------------------------------------------------*/
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
		_self_domain = 0;
		_self_vid = 0;
		_self_gid = 0;
	}
}

STATUS save_id_info_into_app_nvr(void)
{
	set_app_nvr_data_domain_id(_self_domain);
	set_app_nvr_data_vid(_self_vid);
	set_app_nvr_data_gid(_self_gid);

	return save_app_nvr_data();
}


void acp_rcv_proc(APP_RCV_HANDLE pt_app_rcv)
{

	ARRAY_HANDLE apdu;
	u8 apdu_len;

	ARRAY_HANDLE return_buffer;
	u8 ret_len;

	u8 head_len;

	// proc

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

			//if((target_domain == _self_domain) || (target_domain == 0))
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
						req = apdu[SEC_ACP_DB_BODY] & BIT_ACP_ACMD_REQ;
						id_match = TRUE;
						pt_body = &apdu[SEC_ACP_DB_BODY];
						head_len = SEC_ACP_DB_BODY;
						break;
					}
					case(EXP_ACP_ACPR_IPTP_VC):								//single node
					{
						if((target_xid == _self_vid) || (target_xid == 0))
						{
							req = apdu[SEC_ACP_VC_BODY] & BIT_ACP_ACMD_REQ;
							id_match = TRUE;
							pt_body = &apdu[SEC_ACP_VC_BODY];
							head_len = SEC_ACP_VC_BODY;
							break;
						}
						else
						{
							goto ACP_RCV_PROC_QUIT;
						}
					}
					case(EXP_ACP_ACPR_IPTP_MC):								//multiple nodes
					{
						u16 end_vid;

						end_vid = (apdu[SEC_ACP_MC_ENDH] << 8) + apdu[SEC_ACP_MC_ENDL];
						caller_vid = (apdu[SEC_ACP_MC_SELH] << 8) + apdu[SEC_ACP_MC_SELL];
						if(_self_vid>=target_xid && _self_vid<end_vid)		//target_xid is start_vid now; selected range is [start_vid,end_vid), like Python.
						{
							req = apdu[SEC_ACP_MC_BODY] & (BIT_ACP_ACMD_REQ|BIT_ACP_ACMD_RES);	// in MC type, req frame and res frame can all invoke response
							id_match = TRUE;
							pt_body = &apdu[SEC_ACP_MC_BODY];
							head_len = SEC_ACP_MC_BODY;
							break;
						}
						else
						{
							goto ACP_RCV_PROC_QUIT;
						}
					}
					case(EXP_ACP_ACPR_IPTP_GB):
					{
						if((target_xid == _self_gid) || (target_xid == 0))
						{
							req = apdu[SEC_ACP_GB_BODY] & BIT_ACP_ACMD_REQ;
							id_match = TRUE;
							pt_body = &apdu[SEC_ACP_GB_BODY];
							head_len = SEC_ACP_GB_BODY;
							break;
						}
						else
						{
							goto ACP_RCV_PROC_QUIT;
						}
					}
					default:
					{
						goto ACP_RCV_PROC_QUIT;
					}
				}

				if(id_match)
				{
					APP_SEND_STRUCT xdata ass;

					ret_len = wsp_cmd_proc(pt_body, apdu_len - head_len, &return_buffer[head_len]);		// 包括了cs, 无所谓了

					if(req)			//need req
					{
						if(idtp == EXP_ACP_ACPR_IPTP_MC)
						{
							new_pending_sticks = calc_pending_sticks();
						}

					
						if(idtp == EXP_ACP_ACPR_IPTP_MC && req_send_queue(ass_send_id)==SEND_STATUS_PENDING)
						{
							adjust_queue_pending(ass_send_id, new_pending_sticks);
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
							if(idtp == EXP_ACP_ACPR_IPTP_MC)
							{
								adjust_queue_pending(ass_send_id, new_pending_sticks);
							}
						}
					}
				}
			}
		}
	}
ACP_RCV_PROC_QUIT:
	OSMemPut(SUPERIOR,return_buffer);
	return;
}


u8 wsp_cmd_proc(ARRAY_HANDLE body, u8 body_len, ARRAY_HANDLE return_buffer)
{
	u8 cmd;
	u8 ret_len = 0;
	u8 mask;
	ARRAY_HANDLE ptw;
	s16 para;

	cmd = *body++;
	ptw = return_buffer;
	
	*ptw++ = cmd & (~BIT_ACP_ACMD_REQ) | BIT_ACP_ACMD_RES;
	ret_len = 1;

	switch(cmd & BIT_ACP_ACMD_CMD)
	{
		case(CMD_ACP_READ_CURR_PARA):
		{
			mask = *body++;
			//if(mask & BIT_ACP_CMD_MASK_T)
			//{
			//	para = measure_current_t();
			//	*ptw++ = (u8)(para>>8);
			//	*ptw++ = (u8)(para);
			//	ret_len += 2;
			//}
			if(mask & BIT_ACP_CMD_MASK_U)
			{
				para = measure_current_v();
				*ptw++ = (u8)(para>>8);
				*ptw++ = (u8)(para);
				ret_len += 2;
			}
			if(mask & BIT_ACP_CMD_MASK_I)
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

u8 _comm_mode = CHNL_CH3;

void set_comm_mode(u8 comm_mode)
{
	_comm_mode = comm_mode;
}

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
	static u8 index[CFG_PHASE_CNT];
	return ((index[phase]++) & BIT_ACP_ACPR_TRAN);
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
	ass.protocol = PROTOCOL_DST;
	mem_cpy(ass.target_uid, target_uid, 6);
	ass.apdu = apdu;
	ass.apdu_len = apdu_len;

	config_dst_flooding(_comm_mode,0,0,0,0);
	sid = app_direct_send(&ass);

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
			time_out = dst_transaction_sticks(apdu_len, return_len, 1000);
			set_timer(tid,time_out);

			ars.apdu = return_buffer;

			do
			{
				powermesh_event_proc();
				apdu_len = app_rcv(&ars);
				if(apdu_len)
				{
					if(ars.protocol == PROTOCOL_DST)
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


/*******************************************************************************
* Function Name  : app_direct_send()
* Description    : 借用dst协议的形式, 实现点对点通信, 
					dll层给具体的uid作为目的地址, dst禁止洪泛转发和acps等
					调用之前需要调用方指定使用DST协议, 并正常调用config_dst_flooding
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
SEND_ID_TYPE app_direct_send(APP_SEND_HANDLE ass)
{
	DLL_SEND_STRUCT dss;
	SEND_ID_TYPE sid = INVALID_RESOURCE_ID;
	ARRAY_HANDLE lsdu = NULL;

	if(ass->protocol == PROTOCOL_DST)
	{
		lsdu = OSMemGet(SUPERIOR);
		if(!lsdu)
		{
			return INVALID_RESOURCE_ID;
		}

		mem_clr((u8*)(&dss),sizeof(DLL_SEND_STRUCT),1);

		dss.phase = ass->phase;
		dss.target_uid_handle = ass->target_uid;

		mem_cpy(lsdu+LEN_DST_NPCI+LEN_MPCI, ass->apdu, ass->apdu_len);	//直接在原apdu上操作有风险

		dss.lsdu = lsdu;
		dss.lsdu_len = ass->apdu_len + LEN_DST_NPCI+LEN_MPCI;
		
		dss.lsdu[0] = CST_DST_PROTOCOL | get_dst_index(dss.phase);		//dst head[0]: conf
		dss.lsdu[1] = 0;												//dst head[1]: jump
		dss.lsdu[2] = 0;												//dst head[2]: forward
		dss.lsdu[3] = 0;												//dst head[3]: acps
		dss.lsdu[4] = 0;												//dst head[4]: build_id
		dss.lsdu[5] = 0;												//dst head[5]: timing_stamp
		dss.lsdu[6] = 0;												//mpdu head
		dss.prop |= (dst_config_obj.comm_mode & CHNL_SCAN)?BIT_DLL_SEND_PROP_SCAN:0;
		dss.xmode = dst_config_obj.comm_mode & 0xF3;
		dss.delay = 0;
		sid = dll_send(&dss);

		OSMemPut(SUPERIOR,lsdu);
	}
	return sid;
}

#endif
