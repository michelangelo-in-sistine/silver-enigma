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
#define BIT_ACP_ACMD_CMD		0x20




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

/* Private Functions ---------------------------------------------------------*/
u8 wsp_cmd_proc(ARRAY_HANDLE body, u8 body_len, ARRAY_HANDLE return_buffer);

/* Datatype ------------------------------------------------------------------*/
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
					case(EXP_ACP_ACPR_IPTP_DB):
					{
						req = apdu[SEC_ACP_DB_BODY] & BIT_ACP_ACMD_REQ;
						id_match = TRUE;
						pt_body = &apdu[SEC_ACP_DB_BODY];
						head_len = SEC_ACP_DB_BODY;
						break;
					}
					case(EXP_ACP_ACPR_IPTP_VC):
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
					case(EXP_ACP_ACPR_IPTP_MC):
					{
						//{
						//
						//	wsp_cmd_proc(pt_body, head_len, );
						//}
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
	
	switch(cmd)
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
			*ptw = EXCEPTION_ERRORCMD;
			ret_len = 1;
			break;
		}
	}
	return ret_len;
}


