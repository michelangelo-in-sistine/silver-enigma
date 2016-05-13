/******************** (C) COPYRIGHT 2012 Belling Inc. ********************
* File Name          : _dst.c
* Author             : Lv Haifeng
* Version            : V 2.0
* Date               : 2013-03-15
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
#include "stdlib.h"

/* Private define ------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/
//typedef struct
//{
//	u8 conf;			// ����app��֮��, ������Ϣ, ���ڷ���ʱ����
//	u8 jumps;
//	u8 forward;
//	u8 stamps;
//	u8 ppdu_len;		// �������ݰ���
//}DST_BACKUP_STRUCT;

//typedef struct
//{
//	RATE_TYPE rate;
//	u8 jumps;
//	u8 forward_prop;	// [0 0 network_ena acps_ena 0 window_index[2:0]]
//
//	//	�ظ�,ת��ʹ��
//	u8 conf;			// ����app��֮��, ������Ϣ, ���ڷ���ʱ����
//	u8 stamps;
//	u8 ppdu_len;		// �������ݰ���
//}DST_CONFIG_STRUCT;		// CC��������������÷���ģʽ, MT��������������ûظ�ģʽ


/* Private macro -------------------------------------------------------------*/


/* Private variables ---------------------------------------------------------*/
DST_STACK_RCV_STRUCT xdata _dst_rcv_obj[CFG_PHASE_CNT];
DST_CONFIG_STRUCT xdata dst_config_obj;

u8 xdata dst_sent_conf[CFG_PHASE_CNT];					// ������dst֡
u8 xdata dst_proceeded_conf[CFG_PHASE_CNT];				// ת������dst֡

#if (DEVICE_TYPE==DEVICE_CC) || (defined DEVICE_READING_CONTROLLER)
//u8 xdata dst_index[CFG_PHASE_CNT];
u8 xdata dst_index;
#else
//DST_BACKUP_STRUCT xdata dst_config_backup;				// MT�Ļظ����������������ͬ
#endif

/* Private function prototypes -----------------------------------------------*/
RESULT check_dst_receivability(DLL_RCV_HANDLE pd);
BOOL check_dst_forward_necessity(DLL_RCV_HANDLE pd);
ARRAY_HANDLE check_dst_integraty(DLL_RCV_HANDLE pd);
void dst_forward(DLL_RCV_HANDLE pd, u8 search);

/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : init_dst
* Description    : Initialize dst
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void init_dst(void)
{
	mem_clr(&dst_sent_conf,sizeof(dst_sent_conf),1);
	mem_clr(&dst_proceeded_conf,sizeof(dst_proceeded_conf),1);	//��ֹ��һ֡��indexΪ0���ܱ�ת��, ��������Ϊ�������ܳ��ֵ�״̬
	
#if DEVICE_TYPE==DEVICE_CC
	//mem_clr(dst_index,sizeof(dst_index),1);
	dst_index = 0;
#endif
	//dst_config_obj.rate = RATE_BPSK;
	//dst_config_obj.jumps = 0x22;
	//dst_config_obj.forward_prop = 0x13;
	config_dst_flooding(0,0,0,0,0);
	dst_config_obj.forward_prop = 0;				//����һ��config_dst_flooding,ʡ�ĳ�����,ͬʱ��ʹ��config_lock������
	//����MT����ʹ��;
//#if DEVICE_TYPE==DEVICE_MT	
//	dst_config_obj.conf = CST_DST_PROTOCOL|BIT_DST_CONF_INDEX|BIT_DST_CONF_UPLINK;
//	dst_config_obj.ppdu_len = 0x20;
//#endif
}

/*******************************************************************************
* Function Name  : dst_send
* Description    : Send a DST protocol frame, 
					CC������������
					MTֻ�ǻظ�(SEARCH,�鷺����),��������CC�����������ͬ
					2014-11-17
					����������ҪMTҲ������������DST(��Ե�),��˻����������޸�:
					* ȡ��CC��app_send��ʱ�������flooding������, ��flooding������ȡ��
					* ͳһʹ��dst_send��Ϊ���
					* CC��MT������config_flooding�Ĺ���,ÿ��configһ��֮��,��һ��app_send�Ͷ���Ϊ��������
					* ��һ��app_sendִ����֮ǰ,MT��dst_config��������պ鷺���仯;
					* Ŀǰֻ����MT�޸�����,����֮��Ļ���0;
					
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
SEND_ID_TYPE dst_send(DST_SEND_HANDLE dst_handle) reentrant
{
	DLL_SEND_HANDLE dss_handle;
	ARRAY_HANDLE pt;
	SEND_ID_TYPE sid;
	u8 conf;
	u8 comm_mode;
	u8 xmode;
	u8 scan;
	u8 total_jumps;
	u8 left_jumps;
	u8 is_forward;
	u8 is_config_lock;
	u32 delay;
	u8 left_window;
	u8 next_window;
	u8 window_index;
	u8 total_window;
	
	if(!dst_handle->nsdu)
	{
		return INVALID_RESOURCE_ID;
	}

	/* prepare, req resources */
	dss_handle = (DLL_SEND_HANDLE)(OSMemGet(MINOR));
	if(!dss_handle)
	{
		return INVALID_RESOURCE_ID;
	}
	mem_shift(dst_handle->nsdu, dst_handle->nsdu_len, LEN_DST_NPCI);
	pt = dst_handle->nsdu;
	is_forward = dst_handle->forward;
	comm_mode = dst_config_obj.comm_mode;
	xmode = comm_mode & 0xF3;
	scan = (comm_mode & CHNL_SCAN)?1:0;
	total_jumps = dst_config_obj.jumps>>4;
	left_jumps = dst_config_obj.jumps&0x0F;
	is_config_lock = dst_config_obj.forward_prop & BIT_DST_FORW_CONFIG_LOCK;
	dst_config_obj.forward_prop &= (~BIT_DST_FORW_CONFIG_LOCK);

	if(!is_forward)
	{
		if(is_config_lock)
		{
#if DEVICE_TYPE == DEVICE_CC		
			conf = CST_DST_PROTOCOL |(dst_handle->search?BIT_DST_CONF_SEARCH:0) | get_dst_index(dst_handle->phase);	//��ʱֻ����CC����DST Index, MT�����Ķ�����index��ֱͨ���ݰ�
#else
			conf = CST_DST_PROTOCOL |(dst_handle->search?BIT_DST_CONF_SEARCH:0);
#endif
			
		}
		else
		{
			conf = CST_DST_PROTOCOL | dst_config_obj.conf|BIT_DST_CONF_UPLINK;
		}
		*pt++ = conf;
		*pt++ = (total_jumps<<4)|total_jumps;
	}
	else
	{
		conf = dst_config_obj.conf;					//ת��conf����
		*pt++ = conf;
		*pt++ = dst_config_obj.jumps-1;				//ת��jump��1
	}
	
#if DEVICE_TYPE==DEVICE_CC
	*pt++ = dst_config_obj.forward_prop | ((dst_handle->search && dst_handle->search_mid)?BIT_DST_FORW_MID:0);
#else
	*pt++ = dst_config_obj.forward_prop;
#endif
	pt++;											//acps����
	*pt++ = get_build_id(dst_handle->phase);		//network id


	/* ʱ�����׼�� */
	window_index = dst_config_obj.forward_prop&BIT_DST_FORW_WINDOW;
	total_window = 0x01<<window_index;


	/* ��ʱʱ���߼�:
	* ��ʱʱ�������������: 1. ���շ��ִ�ʣ�ര��. ��total_window - window_stamp - 1. ����:��total_jumps==left_jumps, ˵������Դ��һ�η���, �ȴ�ʱ�����Ϊ0;
	2. ת��������, ���Լ���Ҫת��, �˴���Ϊ��������ܴ�����ȡ��. ����: ��ظ�,�ҷ���left_jumps==0, ˵�����������һ�ν���, û����ת��, �ȴ�ʱ�����Ϊ0;
		����ת���򲻻ᷢ��left_jumps==0, ��Ϊ������forward_necessary()��һ��;
	*/

	if(!is_forward && is_config_lock)
	{
		left_window = 0;
		next_window = 0;
		*pt++ = 0;
		delay = DLL_SEND_DELAY_STICKS;
	}
	else
	{
		left_window = (total_jumps>left_jumps)?(total_window - 1 - dst_config_obj.stamps):0;

		if(!is_forward)
		{
			next_window = (left_jumps)?total_window:0;
			*pt++ = 0;
		}
		else
		{
			next_window = rand()&(~(0xFF<<window_index));
			*pt++ = next_window;
		}

		delay = windows_delay_sticks(dst_config_obj.ppdu_len,xmode,scan,0,left_window);
		if(dst_handle->search)
		{
			delay += windows_delay_sticks(dst_config_obj.ppdu_len+7,xmode,scan,0,next_window);
		}
		else
		{
			delay += windows_delay_sticks(dst_config_obj.ppdu_len,xmode,scan,0,next_window);
		}
#ifdef DEBUG_DST
		my_printf("left:%bu,next:%bu,delay:%lu\r\n",left_window,next_window,delay);
#endif
	}

	dst_sent_conf[dst_handle->phase] = conf;		//��¼���͵�dst֡
	
	dss_handle->phase = dst_handle->phase;
	dss_handle->target_uid_handle = NULL;
	dss_handle->lsdu = dst_handle->nsdu;
	dss_handle->lsdu_len = dst_handle->nsdu_len + LEN_DST_NPCI;
#if DEVICE_TYPE == DEVICE_CC
	dss_handle->prop = (dst_handle->forward&BIT_DST_FORW_ACPS)?BIT_DLL_SEND_PROP_ACUPDATE:0;	//cc�����ָ����λ,����������,����ȴ���λ����ռ��̫��ʱ��
#else
	dss_handle->prop = BIT_DLL_SEND_PROP_ACUPDATE;				//mtһ�ɰ�acps����
#endif
	dss_handle->prop |= scan?BIT_DLL_SEND_PROP_SCAN:0;
	dss_handle->xmode = xmode;
	dss_handle->delay = delay;
	sid = dll_send(dss_handle);
	OSMemPut(MINOR,dss_handle);
	return sid;
}

/*******************************************************************************
* Function Name  : dst_rcv_proc
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void dst_rcv_proc(DLL_RCV_HANDLE pd)
{
	DST_STACK_RCV_HANDLE pdst;
	u8 conf;
	u8 jumps;
	u8 forward;
	ARRAY_HANDLE nsdu;

	nsdu = check_dst_integraty(pd);
	if(nsdu)
	{
		if(check_dst_receivability(pd))						//��������
		{
			conf = pd->lpdu[SEC_LPDU_DST_CONF];
			jumps = pd->lpdu[SEC_LPDU_DST_JUMPS];
			forward = pd->lpdu[SEC_LPDU_DST_FORW];

			dst_proceeded_conf[pd->phase] = conf;			//��־�Ѵ���
			dst_sent_conf[pd->phase] = 0;					//��־��һ����ʼ�����Լ����͵�, ������һ�κ����ͬindex�İ����Ӳ���

			/* ��������DSTȫ�ֱ���������ÿһ�ν��յ��ĺ鷺���ö��ı�, Ŀǰ����MT��Ч */
#if DEVICE_TYPE==DEVICE_MT
			if(!(dst_config_obj.forward_prop & BIT_DST_FORW_CONFIG_LOCK))
			{
				PHY_RCV_HANDLE pp;
	
				pp = GET_PHY_HANDLE(pd);
	
				dst_config_obj.comm_mode = (pp->phy_rcv_supposed_ch) | ((pp->phy_rcv_valid)&0x0B);
				dst_config_obj.jumps = jumps;
				dst_config_obj.forward_prop = forward;
	
				dst_config_obj.conf = conf;
				dst_config_obj.stamps = pd->lpdu[SEC_LPDU_DST_WINDOW_STAMP];
				dst_config_obj.ppdu_len = pd->lpdu_len+LEN_PPCI;
			}
#endif
			
			if(conf & BIT_DST_CONF_SEARCH)
			{
#if DEVICE_TYPE!=DEVICE_CC														//CC���ܱ�Search����

				DST_SEND_STRUCT xdata dst;

				if(((forward&BIT_DST_FORW_MID)&&(check_meter_id(nsdu))) || 
					(!(forward&BIT_DST_FORW_MID)&&(check_uid(pd->phase,nsdu))))
				{
					dst.phase = pd->phase;
					dst.search = 1;
					dst.forward = 0;
					*(pd->lpdu + (pd->lpdu_len)) = conf;							//׷���յ���conf
					get_uid(pd->phase, pd->lpdu + (pd->lpdu_len) + 1);			//׷��uid
					dst.nsdu = &pd->lpdu[SEC_LPDU_DST_NSDU];
					dst.nsdu_len = pd->lpdu_len - LEN_LPCI - LEN_DST_NPCI + 7;
					
					dst_send(&dst);
				}
				else
				{
					if(check_dst_forward_necessity(pd))
					{
						dst_forward(pd,1);
					}
					else
					{
#ifdef DEBUG_DST
						my_printf("unforwardable\r\n");
#endif
					}
				}
				dll_rcv_resume(pd);

				/* CC search procedure*/
#else
				forward = forward;
				pdst = &_dst_rcv_obj[pd->phase];
				if(conf & BIT_DST_CONF_UPLINK)
				{
					if(!pdst->dst_rcv_indication)
					{
						pdst->phase = pd->phase;
						pdst->uplink = 1;
						pdst->apdu = nsdu;					//Search���ؽ��û��MPDU Head
						pdst->apdu_len = pd->lpdu_len-(LEN_LPCI+LEN_DST_NPCI);
						pdst->dst_rcv_indication = 0x01;
					}
				}
				else
				{
#ifdef DEBUG_DST
					my_printf("cc is configured to be not searchable.\r\n");
#endif
					dll_rcv_resume(pd);
				}
#endif
			}
			else
			{
				if(check_dst_forward_necessity(pd))
				{
					dst_forward(pd,0);
				}
				pdst = &_dst_rcv_obj[pd->phase];
				if(!pdst->dst_rcv_indication)				//����Ӧ�ò�
				{
					pdst->phase = pd->phase;
					mem_cpy(pdst->src_uid,&pd->lpdu[SEC_LPDU_SRC],6);
					pdst->comm_mode = dst_config_obj.comm_mode;
					pdst->uplink = (conf & BIT_DST_CONF_UPLINK)?1:0;
					pdst->apdu = nsdu+1;					//ȥ��MPDU Head
					pdst->apdu_len = pd->lpdu_len-(LEN_LPCI+LEN_DST_NPCI+1);
					pdst->dst_rcv_indication = 0x01;
#ifdef DEBUG_DST
					my_printf("got apdu,conf:%bx,jumps:%bx\r\n",conf,jumps);
#else
					jumps = jumps;
#endif					
				}				
			}
		}
		else
		{
#ifdef DEBUG_DST
			my_printf("unreceivable\r\n");
#endif
			dll_rcv_resume(pd);
		}
	}
	else
	{
//#ifdef DEBUG_DST
//		my_printf("error dst\r\n");
//#endif	
		dll_rcv_resume(pd);
	}
}

/*******************************************************************************
* Function Name  : dst_rcv_resume
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void dst_rcv_resume(DST_STACK_RCV_HANDLE pdst)
{
	dll_rcv_resume(GET_DLL_HANDLE(pdst));
	pdst->dst_rcv_indication = 0;
}

/*******************************************************************************
* Function Name  : check_dst_integraty
* Description    : �жϵ�ǰdst֡������
* Input          : 
* Output         : 
* Return         : �ɹ�����NSDU��ַ, ʧ�ܷ���NULL
*******************************************************************************/
ARRAY_HANDLE check_dst_integraty(DLL_RCV_HANDLE pd)
{
	if(pd->lpdu_len<=(LEN_LPCI+LEN_DST_NPCI))
	{
		return NULL;
	}
	else
	{
		return &pd->lpdu[SEC_LPDU_DST_NSDU];
	}
}


/*******************************************************************************
* Function Name  : check_dst_receivability
* Description    : �жϵ�ǰdst֡�Ƿ����
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
RESULT check_dst_receivability(DLL_RCV_HANDLE pd)
{
	u8 conf;
	u8 forward;
	u8 jump;								//2014-03-21 ԭʼ����Ϊ0��dst֡��������к�

	u8 i;

	conf = pd->lpdu[SEC_LPDU_DST_CONF];
	forward = pd->lpdu[SEC_LPDU_DST_FORW];
	jump = pd->lpdu[SEC_LPDU_DST_JUMPS];


	//����Ƿ����Լ��շ�����, ����һ��
	if(jump&0xF0)							//2014-03-21 ����Ϊ0��֡��������к�,��Ϊ0�����ظ���
	{
		for(i=0;i<CFG_PHASE_CNT;i++)
		{
			if(conf==dst_sent_conf[i])
			{
#ifdef DEBUG_DST
				my_printf("repeated forward frame\r\n");
#endif
				return WRONG;
			}
		}

		//����Ƿ����Լ��մ������, һ��һ�࿴
		if(conf==dst_proceeded_conf[pd->phase])
		{
#ifdef DEBUG_DST
			my_printf("repeated receiving\r\n");
#endif
			return WRONG;
		}
	}

	//�����λ��ϵ, ������ź���Ч�Ҳ�ͬ��, ������; �������Ч, ����(����ʹ�����𻵵Ľڵ��Ϊ�µ�)
	if(forward & BIT_DST_FORW_ACPS)
	{
#if DEVICE_TYPE==DEVICE_CC
		if(!(is_zx_valid(pd->phase)) || (phy_acps_compare(GET_PHY_HANDLE(pd))!=0))	//CC�����й����ź�
#else
		if((is_zx_valid(pd->phase)) && (phy_acps_compare(GET_PHY_HANDLE(pd))!=0))	//�޹����ź�, ����
#endif
		{
#ifdef DEBUG_DST
			my_printf("diff phase\r\n");
#endif
			return WRONG;
		}
	}

	/* ��������ϵ, ��������Ҳ�ͬ����, ������; ����Ч�����, ����(����ʹδ�����Ľڵ��Ϊ�µ�) */
	if(forward & BIT_DST_FORW_NETWORK)
	{
		u8 network_id;

		network_id = get_build_id(pd->phase);
		if(network_id && (network_id != pd->lpdu[SEC_LPDU_DST_NETWORK_ID]))
		{
//#ifdef DEBUG_DST
//			my_printf("diff network\r\n");
//#endif
			return WRONG;
		}
	}
	
	return CORRECT;
}

/*******************************************************************************
* Function Name  : check_dst_forward_necessity
* Description    : �жϵ�ǰdst֡�Ƿ���Ҫת��, ������������֮һ�߼�����ת��
					* Դ������ʣ������Ϊ0;
					* ֡�б궨ͬ��λת��,���ع�����ź���Ч���ж�Ϊ��ͬ��;
					* ֡�б궨ͬ����ת��,�����ж�����Ų�һ��;
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
BOOL check_dst_forward_necessity(DLL_RCV_HANDLE pd)
{
#ifdef FLOODING_NO_FORWARD
	return FALSE;					//��������ת���鷺֡
#else

	u8 jumps;
	u8 forward;

	jumps = pd->lpdu[SEC_LPDU_DST_JUMPS];
	forward = pd->lpdu[SEC_LPDU_DST_FORW];


	/* ������� */
	if(!(jumps&0xF0) || !(jumps&0x0F))
	{
//#ifdef DEBUG_DST
//		my_printf("no jumps\r\n");
//#endif
		return FALSE;
	}

	/* �����λ��ϵ, ����ȷ��ͬ���ת��, ��ֹ�������� */
	if(forward & BIT_DST_FORW_ACPS)
	{
		if(!is_zx_valid(pd->phase) || (phy_acps_compare(GET_PHY_HANDLE(pd))!=0))	//�޹����ź�,��ת��
		{
//#ifdef DEBUG_DST
//			my_printf("diff phase\r\n");
//#endif
			return FALSE;
		}
	}

	/* ��������ϵ, ����ȷ��ͬ�����ת��, ��ֹ���������� */
	if(forward & BIT_DST_FORW_NETWORK)
	{
		u8 network_id;

		network_id = get_build_id(pd->phase);
		if(!network_id || (network_id != pd->lpdu[SEC_LPDU_DST_NETWORK_ID]))
		{
//#ifdef DEBUG_DST
//			my_printf("diff net\r\n");
//#endif
			return FALSE;
		}
	}

	/* ͨ���������м���֡, ȷ��Ϊ��Ҫת�� */
	return TRUE;
#endif
}

/*******************************************************************************
* Function Name  : dst_forward
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void dst_forward(DLL_RCV_HANDLE pd, u8 search)
{
	DST_SEND_STRUCT xdata dst;
	ARRAY_HANDLE send_buffer;
	ARRAY_HANDLE nsdu;
	BASE_LEN_TYPE nsdu_len;

	/* prepare */
	nsdu = &pd->lpdu[SEC_LPDU_DST_NSDU];
	nsdu_len = pd->lpdu_len - LEN_LPCI - LEN_DST_NPCI;
	send_buffer = OSMemGet(SUPERIOR);
	if(!send_buffer)					//dst_forward������һ���µ�buffer,��Ϊdst_send��۸�buffer����
	{
		return;
	}
	mem_cpy(send_buffer,nsdu,nsdu_len);

	/* append self uid if search */
	if(search)
	{
		*(send_buffer+nsdu_len) = pd->lpdu[SEC_LPDU_LSDU];			//append conf as the 
		get_uid(pd->phase,send_buffer+nsdu_len+1);	//append self uid
		nsdu_len += 7;
	}

	dst.phase = pd->phase;
	dst.search = search;
	dst.forward = 1;
	dst.nsdu = send_buffer;
	dst.nsdu_len = nsdu_len;
	dst_send(&dst);
	OSMemPut(SUPERIOR,send_buffer);
}

#if (DEVICE_TYPE==DEVICE_CC) || (defined DEVICE_READING_CONTROLLER)
/*******************************************************************************
* Function Name  : get_dst_index()
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
s8 get_dst_index(u8 phase)
{
	if(phase>=CFG_PHASE_CNT)
	{
		return INVALID_RESOURCE_ID;
	}
	//return dst_index[phase]++ & BIT_DST_CONF_INDEX;
	return dst_index++ & BIT_DST_CONF_INDEX;
}
#endif

/*******************************************************************************
* Function Name  : config_dst_flooding()
* Description    : 
* 2015-03-12 ������Ҫ��Ե㹦��, ϣ������SCAN��ʽ�⻹���������Ŀ��Ʒ�ʽ
* �������ԭDSTЭ��:��ֻ��SCAN��ʽ�����֧��DLL�������ͨ������,��
* (CH0+BPSK /CH1+BPSK /CH2+BPSK /CH3+BPSK /SCAN+BPSK /SALVO+BPSK)
* (CH0+DS15 /CH1+DS15 /CH2+DS15 /CH3+DS15 /SCAN+DS15 /SALVO+DS15)
* (CH0+DS63 /CH1+DS63 /CH2+DS63 /CH3+DS63 /SCAN+DS63 /SALVO+DS63)
* ��ʽΪԭ����ʾ���ʵĲ���rate,��չΪcomm_type,ͬʱ����ԭrate�÷�,�û���ѡ����Ҫ����
* ����config_dst_flooding(CHNL_CH2|RATE_63,0,0,0,0);

* STATUS config_dst_flooding(COMM_MODE_TYPE comm_mode, u8 max_jumps, u8 max_window_index, u8 acps_constrained, u8 network_constrained)
* ����comm_mode��ѡ���Ƶ�ʲ���: CHNL_CH0, CHNL_CH1, CHNL_CH2, CHNL_CH3, CHNL_SALVO, CHNL_SCAN; ��ѡ������ʲ���: RATE_BPSK, RATE_DS15, RATE_DS63
* ���ڵ�������DST��ʽʹ��app_send()ͨ��ִ����������ǰ, ��Ҫִ��һ�α�����, ��ָ��ͨ�ŷ�ʽ, �鷺����, ���ڵ�
* ���ڵ���Ҫ���д𸴵���app_send()ʱ, ����Ҫ���ñ�����, ֱ��ʹ��app_send()�ظ�����, �ײ㽫ֱ�����ѽ��յ�����ͨ�з�ʽ�ظ�

* MT����������������ʱ, ���ڵ��ñ�����ʱ�Ѻ��ĸ�������Ϊ0, ��MT������������Ե�����, �����鷺����Ӧ��, ����������������鷺��������

* e.g.:
* config_dst_flooding(RATE_BPSK,0,0,0,0); 				//Ĭ��ΪSCAN+BPSKͨ�ŷ�ʽ;
* config_dst_flooding(CHNL_CH0 | RATE_BPSK,0,0,0,0);	//ʹ��CH0��Ƶ��BPSK
* config_dst_flooding(CHNL_CH3 | RATE_DS15,0,0,0,0);	//ʹ��CH3��Ƶ��DS15
* config_dst_flooding(CHNL_SALVO | RATE_DS15,0,0,0,0);	//ʹ��salvoģʽ(��Ƶ���뷢)DS15

* ע1: RATE��ָ����Ĭ��ΪBPSK; CHNL��ָ����Ĭ��ΪCHNL_SCAN;
* ע2: CHNL_SCAN����������Ƶ��ָ��, ��CHNL_CH3 | CHNL_SCAN|RATE_DS15 ��Ч���͵���CHNL_SCAN|RATE_DS15;
* ע3: ���鲻Ҫʹ��CHNL_CH0 | CHNL_CH1����������Ƶ�ʵķ�ʽ, �ײ㲻�Ը�ģʽ���м��;
*
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
STATUS config_dst_flooding(COMM_MODE_TYPE comm_mode, u8 max_jumps, u8 max_window_index, u8 acps_constrained, u8 network_constrained)
{
	if((comm_mode&0x03)>RATE_DS63)
	{
#ifdef DEBUG_DST
		my_printf("error comm_mode\r\n");
#endif
		return FAIL;
	}

	if((comm_mode & 0xFC)==0)			//2015-03-13 ����ԭ���:������ָ��SCAN�ֲ�ָ��Ƶ��ʱ,Ĭ��ΪSCAN��ʽ
	{
		comm_mode |= (CHNL_CH0 | CHNL_SCAN);
	}
	else if((comm_mode & 0xF0)==0)		//2015-03-13 ���������SCAN,û����Ƶ��,�Զ���һλƵ��λ
	{
		comm_mode |= CHNL_CH0;
	}
	
	dst_config_obj.comm_mode = comm_mode;
	/***************************************
	* 2014-11-17 ����MT��������,���������޸�����,����Ե�
	***************************************/
#if DEVICE_TYPE == DEVICE_MT
	dst_config_obj.forward_prop = 0x80;
	dst_config_obj.jumps = 0;
	max_jumps = max_jumps;
	max_window_index = max_window_index;
	acps_constrained = acps_constrained;
	network_constrained = network_constrained;
#else
	/***************************************
	* 2014-11-17 CC�����޸ĺ鷺��������
	***************************************/
	if(max_jumps>CFG_DST_MAX_JUMPS)
	{
#ifdef DEBUG_DST
		my_printf("too big jumps\r\n");
#endif
		return FAIL;
	}
	else if(max_window_index>CFG_DST_MAX_WINDOW_INDEX)
	{
#ifdef DEBUG_DST
		my_printf("too big window\r\n");
#endif
		return FAIL;
	}
	
	dst_config_obj.jumps = (max_jumps<<4)|(max_jumps);
	dst_config_obj.forward_prop = max_window_index;
	dst_config_obj.forward_prop |= acps_constrained?BIT_DST_FORW_ACPS:0;
	dst_config_obj.forward_prop |= network_constrained?BIT_DST_FORW_NETWORK:0;
#endif

	dst_config_obj.forward_prop |= BIT_DST_FORW_CONFIG_LOCK;
	return OKAY;

}

