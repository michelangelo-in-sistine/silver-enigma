/******************** (C) COPYRIGHT 2012 Belling Inc. ********************
* File Name          : dll.c
* Author             : Lv Haifeng
* Version            : V 2.0
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
//#include "powermesh_config.h"
//#include "powermesh_spec.h"
//#include "powermesh_timing.h"
//#include "mem.h"
//#include "hardware.h"
//#include "general.h"
//#include "timer.h"
//#include "uart.h"
//#include "phy.h"
//#include "dll.h"
//#include "ebc.h"

/* Private define ------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
extern u8 code CODE2MODE_VAR[];
#define DIAG_REP_MODE_DECODE(dlct) (CODE2MODE_VAR[(dlct&0x3C)>>2])

/* Private variables ---------------------------------------------------------*/
DLL_RCV_STRUCT xdata _dll_rcv_obj[CFG_PHASE_CNT];
u8 xdata _ebc_response_enable;			// 2014-11-10 ����Ҫ��ģ��������óɲ���Ӧ�㲥

/* Private function prototypes -----------------------------------------------*/
u8 get_dll_dlct(DLL_SEND_HANDLE pdss) reentrant;
void dll_response(PHY_RCV_HANDLE pp);

/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : set_ebc_response_enable
* Description    : set ebc broadcast response enable/disable
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void set_ebc_response_enable(BOOL enable)
{
	_ebc_response_enable = enable;
}


/*******************************************************************************
* Function Name  : init_dll
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void init_dll()
{
	u8 i;
	
	mem_clr(_dll_rcv_obj, sizeof(DLL_RCV_STRUCT), CFG_PHASE_CNT);
	for(i=0;i<3;i++)
	{
		_dll_rcv_obj[i].phase = i;
	}
#ifdef USE_EBC	
	init_ebc();
#if DEVICE_TYPE == DEVICE_MT
	set_ebc_response_enable(TRUE);
#else
	set_ebc_response_enable(FALSE);
#endif
#endif
#ifdef USE_MAC
	init_mac();
#endif
}

/*******************************************************************************
* Function Name  : dll_send
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
SEND_ID_TYPE dll_send(DLL_SEND_HANDLE pdss) reentrant
{
	PHY_SEND_HANDLE ppss;
	ARRAY_HANDLE lpdu;
	SEND_ID_TYPE sid;
	u8 dynamic_mem = FALSE;
	
	ppss = OSMemGet(MINOR);
	if(!ppss)
	{
		return INVALID_RESOURCE_ID;
	}

	if(pdss->lsdu_len==0)				//�������Ľṹ���ﲻ��������, ���豾�������������ݰ�Buffer;
	{
		dynamic_mem = TRUE;
		lpdu = OSMemGet(MINOR);
		if(!lpdu)
		{
			OSMemPut(MINOR,ppss);
			return INVALID_RESOURCE_ID;
		}
	}
	else
	{
		lpdu = pdss->lsdu;
	}
	
	/* Fill DLL SEC */
	if(pdss->prop & BIT_DLL_SEND_PROP_SRF)
	{
		//srf��lsduǰû���κ����
	}
	else
	{
		mem_shift(lpdu,pdss->lsdu_len,LEN_LPCI);				//shift lsdu
		if(pdss->target_uid_handle)
		{
			mem_cpy(&lpdu[SEC_LPDU_DEST],pdss->target_uid_handle,6);		//load dest uid
		}
		else
		{
			u8 i;
			ARRAY_HANDLE ptw;

			ptw = &lpdu[SEC_LPDU_DEST];
			for(i=0;i<6;i++)
			{
				*ptw++ = 0xFF;
			}
		}
		lpdu[SEC_LPDU_DLCT] = get_dll_dlct(pdss);				//fill dlct
		get_uid(pdss->phase,&lpdu[SEC_LPDU_SRC]);				//fill src
	}

	/* send */
	ppss->phase = pdss->phase;
	ppss->xmode = pdss->xmode;
	//ppss->prop = (dll_prop & BIT_DLL_SEND_PROP_SCAN)?(BIT_PHY_SEND_PROP_SCAN):0;
	//ppss->prop |= (dll_prop & BIT_DLL_SEND_PROP_SRF)?(BIT_PHY_SEND_PROP_SRF):0;
	//ppss->prop |= (dll_prop & BIT_DLL_SEND_PROP_ACUPDATE)?(BIT_PHY_SEND_PROP_ACUPDATE):0;
	//������PHY_SEND_PROP��DLL_SEND_PROP������ͬ���ص�
	ppss->prop = pdss->prop & (BIT_DLL_SEND_PROP_SCAN|BIT_DLL_SEND_PROP_SRF|BIT_DLL_SEND_PROP_ACUPDATE);
	
	ppss->psdu = lpdu;
	ppss->psdu_len = pdss->lsdu_len + LEN_LPCI;
	ppss->delay = pdss->delay;

	sid = phy_send(ppss);
	OSMemPut(MINOR,ppss);
	if(dynamic_mem)
	{
		OSMemPut(MINOR,lpdu);
	}
	return sid;
}

/*******************************************************************************
* Function Name  : check_uid
* Description    : 
* Input          : �׵�ַ
* Output         : 
* Return         : CORRECT/WRONG/BROADCAST/SOMEBODY
*******************************************************************************/
RESULT check_uid(u8 phase, ARRAY_HANDLE target_uid)
{
	u8 i;
	u8 xdata self_uid[6];

	phase = phase;
	for(i=0;i<=5;i++)			//�Ƿ�Ϊȫ0��ַ(�㲥)
	{
		if(target_uid[i]!=0)
		{
			break;
		}
		if(i==5)
		{
			return BROADCAST;
		}
	}
	
	for(i=0;i<=5;i++)			//�Ƿ�ΪȫFF��ַ(Unknown Address)
	{
		if(target_uid[i]!=0xFF)
		{
			break;
		}
		if(i==5)
		{
			return SOMEBODY;
		}
	}

	get_uid(phase,self_uid);
	if(mem_cmp(self_uid,target_uid,6))
	{
		return CORRECT;
	}
	else
	{
		return WRONG;
	}
}

/*******************************************************************************
* Function Name  : dll_rcv()
* Description    : 
* Input          : 
* Output         : DLL_RCV_STRUCT
* Return         : None
*******************************************************************************/
DLL_RCV_HANDLE dll_rcv()
{
	u8 i;
	
	for(i=0;i<CFG_PHASE_CNT;i++)
	{
		if(_dll_rcv_obj[i].dll_rcv_indication)
		{
			return &_dll_rcv_obj[i];
		}
	}
	return NULL;
}


/*******************************************************************************
* Function Name  : dll_rcv_resume()
* Description    : 
* Input          : 
* Output         : 
* Return         : None
*******************************************************************************/
void dll_rcv_resume(DLL_RCV_HANDLE pd) reentrant
{
	phy_rcv_resume(GET_PHY_HANDLE(pd));		//2013-03-13 ���������phy����dll,��������ʱ���жϻ��ǻὫdll��λ; ��phy��ʱ�Ѿ������
	pd->dll_rcv_valid = 0;
	pd->dll_rcv_indication = 0;
}

/*******************************************************************************
* Function Name  : dll_rcv_proc()
* Description    : 
* Input          : PHY_RCV_STRUCT
* Output         : DLL_RCV_STRUCT
* Return         : None
*******************************************************************************/
#ifdef DEBUG_RCV_DISP
extern u8 get_acps_relation(u8 mode, u8 local_ph, u8 remote_ph);
#endif

void dll_rcv_proc(DLL_RCV_HANDLE pd)
{
	PHY_RCV_HANDLE pp;
	ARRAY_HANDLE ppdu;

	if(!pd->dll_rcv_valid)
	{
		pp = GET_PHY_HANDLE(pd);
		if(pp->phy_rcv_valid)
		{
			u8 phpr;

			ppdu = pp->phy_rcv_data;
			phpr = ppdu[SEC_PHPR];

			if((phpr & 0x03) == FIRM_VER)
			{
				ppdu = pp->phy_rcv_data;
#ifdef USE_MAC
				if((phpr & PHY_FLAG_NAV) == PHY_FLAG_NAV)
				{
					set_mac_nav_timing_from_esf(pp);
					phy_rcv_resume(pp);
				}
				else
				{
#endif
				if(phpr & PHY_FLAG_SRF)
				{
					pd->dll_rcv_valid = BIT_DLL_VALID | BIT_DLL_ACK | BIT_DLL_SRF;	//SRF is a special ack frame.
					pd->dll_rcv_indication = 1;
				}
				else
				{
					u8 dlct = ppdu[SEC_DLCT];
#ifdef USE_MAC
					if((dlct & BIT_DLCT_ACK) == 0)
					{
						update_nearby_active_nodes(&ppdu[SEC_SRC]);					// 2016-06-02 ����һ�����ܱ߽ڵ�
					}
#endif
		
					if(check_uid(pp->phase,&ppdu[SEC_DEST]) != WRONG)				// CORRECT/BROADCAST/SOMEBODY ALL PASS!
					{
#ifdef DEBUG_DISP_DLL_RCV
						print_phy_rcv(pp);
//			my_printf("RCV PHASE:%bX %bX %bX %bX\r\n",pp->plc_rcv_acps[0],pp->plc_rcv_acps[1],pp->plc_rcv_acps[2],pp->plc_rcv_acps[3]);
//			my_printf("RELATION:%bX",get_acps_relation(pp->phy_rcv_valid&0x03, pp->plc_rcv_acps[0], pp->phy_rcv_data[SEC_NBF_COND]));
#endif		
						
						if(dlct & BIT_DLCT_DIAG)						// Diag��Ӧ����, �����history
						{
							if(dlct & BIT_DLCT_ACK)						// �����ݰ���һ��DIAG��ACK
							{
								pd->dll_rcv_valid = BIT_DLL_VALID | BIT_DLL_DIAG | BIT_DLL_ACK | (dlct & BIT_DLL_IDX);
								pd->dll_rcv_indication = 1;				// diag ack, �����̴߳���
							}
							else										// Diag request, ack and over;
							{
								dll_response(pp);
								dll_rcv_resume(pd);
							}
						}
						else
						{
							if(dlct & BIT_DLCT_ACK)						// Got a Normal Ack, for Dll AckSend Func.
							{
								pd->dll_rcv_valid = BIT_DLL_VALID | BIT_DLL_ACK | (dlct & BIT_DLL_IDX);
								pd->dll_rcv_indication = 1;				// normal ack, �����̴߳���
							}
							else
							{
								if(dlct & BIT_DLCT_REQ_ACK) 			// Ack Request, for higher layer proc
								{
#ifdef DEBUG_DLL
									my_printf("req_ack packet received.");
#endif
									phy_rcv_resume(pp);
									//dll_response(pp);
									//pd->dll_rcv_valid = BIT_DLL_VALID | BIT_DLL_REQ_ACK | (dlct & BIT_DLL_IDX);
									//pd->lpdu = &ppdu[SEC_LPDU];
									//pd->lpdu_len = pp->phy_rcv_len-LEN_PPCI;
								}
								else									// Non-Ack Request, for higher layer proc
								{
									pd->dll_rcv_valid = BIT_DLL_VALID | (dlct & BIT_DLL_IDX);
								}
							}
						}
					}
					else
					{
						phy_rcv_resume(pp);
					}
				}
				if(pd->dll_rcv_valid & BIT_DLL_VALID)
				{
					pd->lpdu = &ppdu[SEC_LPDU];
					pd->lpdu_len = pp->phy_rcv_len-LEN_PPCI;
				}
#ifdef USE_MAC
				}
#endif
			}
			else
			{
#ifdef DEBUG_DLL
				my_printf("invalid version.");
				print_phy_rcv(pp);
#endif
				phy_rcv_resume(pp);
			}
		}
	}
}




/*******************************************************************************
* Function Name  : get_dll_dlct()
* Description    : �ڱ��汾��, 
					ֻ��Diag��Ҫ��Ӧ
					����ͨ�ž�ΪUNACK��ʽ, ��˲���Ҫindex
					2013-03-14 �����EDP��֧��
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u8 get_dll_dlct(DLL_SEND_HANDLE pdss) reentrant
{
	u8 dlct;

	if(pdss->prop & BIT_DLL_SEND_PROP_EDP)
	{
		dlct = EXP_DLCT_EDP;
		dlct |= (pdss->prop & BIT_DLL_SEND_PROP_ACK)? BIT_DLCT_ACK:0;
	}
	else if(pdss->prop & BIT_DLL_SEND_PROP_DIAG)
	{
		dlct = BIT_DLCT_DIAG;
		dlct |= (pdss->prop & BIT_DLL_SEND_PROP_ACK)\
					? BIT_DLCT_ACK \
					:(encode_xmode(pdss->rmode,pdss->prop&BIT_DLL_SEND_PROP_SCAN)<<2);
	}
	else //����ͨ�Ű�
	{
		dlct = 0;
		dlct |= (pdss->prop & BIT_DLL_SEND_PROP_ACK)? BIT_DLCT_ACK:0;
	}
	return dlct;
}

/*******************************************************************************
* Function Name  : dll_response
* Description    : Dll ������Ӧ, ����diag��Ӧ����ͨ����֡��Ӧ
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void dll_response(PHY_RCV_HANDLE pp)
{
	DLL_SEND_STRUCT xdata dss;
	ARRAY_HANDLE lsdu;
	BASE_LEN_TYPE lsdu_len;
	ARRAY_HANDLE ppdu = pp->phy_rcv_data;
	u8 xdata phpr = ppdu[SEC_PHPR];
	u8 xdata dlct = ppdu[SEC_DLCT];

	mem_clr(&dss,sizeof(PHY_SEND_STRUCT),1);

	if(dlct & BIT_DLCT_DIAG)
	{
#ifdef USE_EBC
		if((dlct & 0x3C) == 0x3C)					// dlct = [x x 1 1 1 1 x x] means extended diag protocol
		{
			if(_ebc_response_enable)					// 2014-11-10 ��������Ҫ��, ���ӹ㲥��Ӧ����
			{
				ext_diag_protocol_response(pp);
			}
		}
		else
#endif
		{
			lsdu = OSMemGet(MINOR);					// apply mem block here, release in phy_xmt_proc;
			lsdu_len = 0;
			if(lsdu != NULL)
			{
				u8 i;
				u8 valid_ch;

				if(phpr & PHY_FLAG_SCAN)
				{
					valid_ch = pp->phy_rcv_valid;
					dss.prop = BIT_DLL_SEND_PROP_SCAN;
				}
				else
				{
					valid_ch = pp->phy_rcv_supposed_ch;
				}

				for(i=0;i<=3;i++)
				{
					if(valid_ch & (0x10<<i))
					{
						lsdu[lsdu_len++] = pp->phy_rcv_ss[i];
						lsdu[lsdu_len++] = pp->phy_rcv_ebn0[i];
					}
					else
					{
						if(phpr & PHY_FLAG_SCAN)
						{
							lsdu[lsdu_len++] = 0x80;				// receive fail ch: -128
							lsdu[lsdu_len++] = 0x80;
						}
					}
				}					

				dss.phase = pp->phase;
				dss.xmode = DIAG_REP_MODE_DECODE(ppdu[SEC_DLCT]);
				dss.target_uid_handle = &ppdu[SEC_SRC];
				dss.prop = BIT_DLL_SEND_PROP_DIAG | BIT_DLL_SEND_PROP_ACK | (ppdu[SEC_DLCT]&BIT_DLCT_IDX);
				dss.prop |= (phpr & PHY_FLAG_SCAN)?(BIT_DLL_SEND_PROP_SCAN):0;
				dss.lsdu = lsdu;
				dss.lsdu_len = lsdu_len;
				dss.delay = ACK_DELAY_STICKS;
				dll_send(&dss);
				OSMemPut(MINOR,lsdu);
			}
		}
	}
	else
	{
#ifdef DEBUG_DLL
		my_printf("NO NORMAL ACK RESPONSE IN THIS VERSION\r\n");
#endif
	}
}

#ifdef USE_DIAG
/*******************************************************************************
* Function Name  : dll_diag()
* Description    : �������ڵ�����һ���ڵ����·����, DLL_SEND׼������
* Input          : struct diag
* Output         : 
* Return         : �ɹ�/ʧ�� (д���ֽ�������buffer��һ����Ч, ��˲�����Ϊ������Ϣ, ���÷�����ʹ�õ�ch�����м���)
					2013-03-28 ����Ч, ���ֽ�Ϊbufferд���ֽ���
*******************************************************************************/
STATUS dll_diag(DLL_SEND_HANDLE pdss, ARRAY_HANDLE buffer)		// 2012-12-26 modify dll diag interface
{
	TIMER_ID_TYPE tid;
	SEND_ID_TYPE sid;
	u32 xdata expiring_sticks;
	STATUS status;
	DLL_RCV_HANDLE pd;
	ARRAY_HANDLE ptw;

	/* make sure */
	pdss->lsdu = NULL;
	pdss->lsdu_len = 0;
	pdss->delay = DLL_SEND_DELAY_STICKS;
	pdss->prop |= (BIT_DLL_SEND_PROP_DIAG | BIT_DLL_SEND_PROP_REQ_ACK);
	if(pdss->prop & BIT_DLL_SEND_PROP_SCAN)
	{
		pdss->xmode |= 0xF0;
		pdss->rmode |= 0xF0;
	}
	

	/* req resources */
	tid = req_timer();
	if(tid==INVALID_RESOURCE_ID)
	{
		return FAIL;
	}

#ifdef DEBUG_DLL
	my_printf("dll_diag():\r\n");
#endif

	expiring_sticks = diag_expiring_sticks(pdss);
	sid = dll_send(pdss);
	if(sid==INVALID_RESOURCE_ID)
	{
		delete_timer(tid);
		return FAIL;
	}

#ifdef USE_MAC
	declare_channel_occupation(sid,expiring_sticks);
#endif

	/* wait send over */
	status = wait_until_send_over(sid);
	if(!status)
	{
		delete_timer(tid);
		return FAIL;
	}

	/******************* ������Ӧ��ʱ�ȴ�ʱ�� ********************/
#ifdef DEBUG_DLL
//	my_printf("SET EXPIRE:%ld.\r\n",expiring_sticks);
#endif
	set_timer(tid,expiring_sticks);

	status = FAIL;
	do
	{
#if NODE_TYPE==NODE_MASTER
		SET_BREAK_THROUGH("quit dll_diag()\r\n");
#endif
		FEED_DOG();
		pd = dll_rcv();
		if(pd)
		{
			if((check_ack_src(&pd->lpdu[SEC_LPDU_SRC],pdss->target_uid_handle) != CORRECT)
			||(pd->dll_rcv_valid & BIT_DLL_SRF))
			{
#ifdef DEBUG_DLL
				my_printf("ack src wrong\r\n");
				print_phy_rcv(GET_PHY_HANDLE(pd));
#endif
			}
			else
			{
				if(pd->dll_rcv_valid & BIT_DLL_ACK)
				{
					PHY_RCV_HANDLE pp;
					u8 i;
					
					pause_timer(tid);
					status = OKAY;
					if(buffer)
					{
						ptw = buffer + 1;
						pp = GET_PHY_HANDLE(pd);
						mem_cpy(ptw,&pd->lpdu[SEC_LPDU_LSDU],pd->lpdu_len-LEN_LPCI);	// copy downlink info in ack packet
						ptw += pd->lpdu_len-LEN_LPCI;							// fill uplink info
						for(i=0;i<=3;i++)
						{
							if(pdss->rmode & ((0x10)<<i))
							{
								*(ptw++) = pp->phy_rcv_ss[i];
								*(ptw++) = pp->phy_rcv_ebn0[i];
							}
						}
						*buffer = (u8)(ptw-buffer-1);
					}
#ifdef DEBUG_DLL
//					my_printf("GOT ACK.EXPECT:%ld,ACTUAL:%ld,DIFF:%ld\r\n",expiring_sticks,(expiring_sticks - get_timer_val(tid)),get_timer_val(tid));
					pp = GET_PHY_HANDLE(pd);
//					my_printf("GOT DIAG RMODE:%bX\r\n",pp->phy_rcv_valid);
					my_printf("DL S&R:");
					uart_send_asc(&pd->lpdu[SEC_LPDU_LSDU],pd->lpdu_len-LEN_LPCI);
					my_printf("\r\nUL S&R:");
					for(i=0;i<=3;i++)
					{
						if(pdss->rmode & ((0x10)<<i))
						{
							my_printf("%bX%bX",pp->phy_rcv_ss[i],pp->phy_rcv_ebn0[i]);
						}
					}
					my_printf("\r\n");
#endif
				}
				else
				{
#ifdef DEBUG_DLL
					my_printf("NON-ACK PACKET\r\n");
#endif
				}
			}
			dll_rcv_resume(pd);
		}
	}while(!is_timer_over(tid) && !status);
	delete_timer(tid);
	return status;
}
#endif

/******************* (C) COPYRIGHT 2012 Belling *****END OF FILE****/

