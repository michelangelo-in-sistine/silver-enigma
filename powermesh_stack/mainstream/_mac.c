/******************** (C) COPYRIGHT 2012 Belling Inc. ********************
* File Name          : mac.c
* Author             : Lv Haifeng
* Version            : V 1.0
* Date               : 2015-04-03
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

//#define DEBUG_INDICATOR DEBUG_MAC 

#ifdef USE_MAC
/* Private define ------------------------------------------------------------*/
#define DEFAULT_MAC_WINDOW_INDEX	3		//ȱʡ�����Ϊ2^3
#define MAX_MAC_WINDOW_INDEX		6		//��������Ϊ2^6
#define DSSS_ACCESS_DELAY_FACTOR	1.5		//DSSS������Ҫ��ƽ�������ʱ����
#define MAX_NAV_TIMING				0xfff0	//���NAVʱ��, 65520ms, ��һ����
#define MAX_NAV_VALUE				0x0fff	//���NAVֵ, ��С��λ16ms

//#define CSMA_DEADLOCK_LIMIT			400		//CSMA��־ˢ�¼����󲻵ó���400ms(DS63 expire = (24+11)*63/fd)
/* Private typedef -----------------------------------------------------------*/
typedef enum
{
	MAC_STATE_INITIAL=0,
	MAC_STATE_BUSY,
	MAC_STATE_DISF,
	MAC_STATE_TO_SEND,
	MAC_STATE_RANDOM_ACCESS,
	MAC_STATE_SEND,
	MAC_STATE_ESF,
	MAC_STATE_NAV,
	MAC_STATE_ANNEAL				//�����ŵ��������뷢�Ϳ���, ��˷��͹��Ľڵ��ھ�����һ��ʱ������,����һ��"�˻�"״̬
}MAC_STATE;


typedef struct
{
	MAC_STATE mac_state;


	u32 mac_universal_timer;		//��ΪDISF��ANNEAL��ʱ

	SEND_ID_TYPE mac_ctrl_sid;
	SEND_ID_TYPE esf_sid;
	u8  mac_random_access_timer;	//ESF��ʱ������
	u8  mac_random_access_turn;		//ESF�����ִ�

	u32 mac_nav_timer;
	
	//u8	mac_approved;				//mac׼�뼶��
	//u8  mac_access_privilege;		//����׼���־,����csma
	//u8  window_index;
	
	//u32 mac_timing_expire;			//mac�����Ķ�ʱ
	//u32 mac_timing_start;			//mac�������ʼʱ��


	//u32 nav_timing_setting;			//nav�첽����Ľӿ�
	//u32 nav_timing_start;			//nav��ʱ��ʼʱ�̵�ʱ��
	//u32 nav_timing_expire;			//nav��ʱ��С
	
}MEDIA_ACCESS_CTRL_STRUCT;


typedef struct
{
	u8 		phase;
	SEND_ID_TYPE sid_esf;
	SEND_ID_TYPE sid_phy;
	u32 	nav_timing;
	u32		wait_timing_expire;
	u32		wait_timing_cnt;
}MAC_QUEUE_SUPERVISE_STRUCT;		//���Ͷ���mac����


typedef struct
{
	u8		nearby_uid[6];			//���������������ڵ�
	u32		last_timing;			//���һ�μ�������ʱ��
}NEARBY_NODES_STRUCT;





typedef MEDIA_ACCESS_CTRL_STRUCT * MAC_CTRL_HANDLE;
typedef MAC_QUEUE_SUPERVISE_STRUCT * MAC_QUEUE_SUPERVISE_HANDLE;

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
MEDIA_ACCESS_CTRL_STRUCT mac_ctrl;

u8 esf_comm_mode;

NEARBY_NODES_STRUCT _nearby_nodes_db[CFG_MAX_NEARBY_NODES];
u8 _nearby_nodes_db_usage;
u32 _mac_timer = 0;					//ά��һ��mac���ȫ��ʱ��, �����жϸ������ڵ㳬ʱ��
u16 _mac_stick_base;
u16 _mac_stick_anneal;

BOOL _mac_print_log_enable = TRUE;
u32 _mac_channel_monopoly_coverage = 0;		//����һ���������ʱ, ʹ�ö��������Ķ���, ��������, �����ױ����

/* Extern variables ---------------------------------------------------------*/
extern u8 xdata _phy_channel_busy_indication[CFG_PHASE_CNT];
extern u8 xdata _phy_send_state;

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
void set_mac_debug_print(BOOL value)
{
	_mac_print_log_enable = value;
}

/*******************************************************************************
* Function Name  : update_nearby_active_nodes()
* Description    : ���������ĸ����Ľڵ�UID���浽���ݿ���
					1. ���������иýڵ��ַ, ���������ʱ��
					2. ������û�иýڵ�, ���⻹�пռ�, ��β�����Ӹõ�ַ, �����¿���Ŀ��
					3. ��������û�иýڵ�, ����û�пռ�, ������������Ľڵ��ų�, ���¼����Ľڵ����õ�ַ
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
STATUS update_nearby_active_nodes(UID_HANDLE src_uid)
{
	u8 i;

	/**************************************************************************
	* �ж��Ƿ��������ڵ�, ����û�и��õİ취, ������ʹ��DLCT_ACK��־λ�ж�
	* ��MT���ܿ�Diag, EBC��ʱ��, Ҳ�ᷢ��û��DLCT_ACK��־λ
	* �����ʱʹ��UID�����ж�, Ŀǰ6810�����UID����ֱ�Ϊ5E1D... 0B9A...
	**************************************************************************/
	if (((src_uid[0] == 0x5E) && (src_uid[1] == 0x1D)) ||
		((src_uid[0] == 0x0B) && (src_uid[1] == 0x9A)))
	{
			return FAIL;
	}

	
	for(i=0;i<_nearby_nodes_db_usage;i++)
	{
		if(mem_cmp_reverse(src_uid,_nearby_nodes_db[i].nearby_uid,6))
		{
			_nearby_nodes_db[i].last_timing = _mac_timer;
			return OKAY;
		}
	}

	//���е�����, ˵������û���ҵ��ýڵ�
	if(_nearby_nodes_db_usage < CFG_MAX_NEARBY_NODES)
	{
		mem_cpy(_nearby_nodes_db[_nearby_nodes_db_usage].nearby_uid, src_uid, 6);
		_nearby_nodes_db[_nearby_nodes_db_usage].last_timing = _mac_timer;
		_nearby_nodes_db_usage++;
	}
	else
	{
		u32 min_timer = _nearby_nodes_db[0].last_timing;
		u8 min_index = 0;
		
		for(i=1;i<_nearby_nodes_db_usage;i++)
		{
			if(_nearby_nodes_db[i].last_timing < min_timer)
			{
				min_timer = _nearby_nodes_db[i].last_timing;
				min_index = i;
			}
		}
		mem_cpy(_nearby_nodes_db[min_index].nearby_uid, src_uid, 6);
		_nearby_nodes_db[min_index].last_timing = _mac_timer;
	}
	return OKAY;
}


/*******************************************************************************
* Function Name  : get_nearby_active_nodes_num()
* Description    : ��õ�ǰ���ܱ߽ڵ���
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
u8 get_nearby_active_nodes_num(void)
{
	return _nearby_nodes_db_usage;
}


/*******************************************************************************
* Function Name  : acquire_nearby_active_nodes_set()
* Description    : ȡ���ܱ߽ڵ�UID�ż���
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
u8 acquire_nearby_active_nodes_set(u8 * buffer)
{
	u16 i;

	for(i=0;i<_nearby_nodes_db_usage;i++)
	{
		mem_cpy(buffer, _nearby_nodes_db[i].nearby_uid, 6);
		buffer += 6;
	}
	return _nearby_nodes_db_usage * 6;
}

/*******************************************************************************
* Function Name  : get_random_access_window()
* Description    : �����ܱ߽ڵ���,�õ��������ڴ�С, ���ݷ���õ�(�����������ܽڵ���, ������Ҫ��ȥ����)
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
u8 get_random_access_window(u8 nearby_nodes)
{
	switch(nearby_nodes)
	{
		case 0:
			return 1;		//�����Χ�޽ڵ�, ֱ�ӷ��ͼ���
		case 1:
			return 6;
		case 2:
			return 7;
		case 3:
			return 8;
		case 4:
			return 9;
		case 5:
			return 9;
		case 6:
			return 9;
		case 7:
			return 9;
		case 8:
			return 10;
		default:
			return nearby_nodes+1;		
	}
}

/*******************************************************************************
* Function Name  : get_random_access_turn()
* Description    : �����ܱ߽ڵ���,�õ������ִ�, ���ֻ���Լ�һ���ڵ�, ���辺��
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
u8 get_random_access_turn(u8 nearby_nodes)
{
	switch(nearby_nodes)
	{
		case 0:
			return 0;
		default:
			return 3;
	}
}

void set_mac_parameter(u16 mac_stick_base_value, u16 mac_stick_anneal_value)
{
	_mac_stick_base = mac_stick_base_value;
	_mac_stick_anneal = mac_stick_anneal_value;
}

/*******************************************************************************
* Function Name  : init_mac()
* Description    : check phy channel
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void init_mac(void)
{	
	mem_clr(&mac_ctrl,sizeof(MEDIA_ACCESS_CTRL_STRUCT),1);
	esf_comm_mode = RATE_BPSK|CHNL_CH3;

	_nearby_nodes_db_usage = 0;
	mem_clr(_nearby_nodes_db,sizeof(NEARBY_NODES_STRUCT),CFG_MAX_NEARBY_NODES);

	set_mac_parameter(CFG_DEFAULT_MAC_STICK_BASE, CFG_DEFAULT_MAC_STICK_ANNEAL);
}

/*******************************************************************************
* Function Name  : is_channel_busy(u8 phase)
* Description    : check phy channel
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
BOOL is_channel_busy(void)
{
	u8 i;

///*for debug*/
//{
//	static u8 last;
//	if(_phy_channel_busy_indication[2]!=last)
//	{
//		my_printf("M%bx\n",_phy_channel_busy_indication[2]);
//		last = _phy_channel_busy_indication[2];
//	}
//}


	for(i=0;i<CFG_PHASE_CNT;i++)
	{
		if(_phy_channel_busy_indication[i])	
		{
//led_t_off();
			return TRUE;
		}
	}
	return FALSE;
}

/*******************************************************************************
* Function Name  : set_mac_nav_timing_from_esf()
* Description    : set nav timer after got a NAV frame.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void set_mac_nav_timing_from_esf(PHY_RCV_HANDLE pp)
{
	u8 navt_h;
	u8 navt_l;
	u32 nav_timing;

	navt_h = (pp->phy_rcv_data[1]&0x0F);
	navt_l = (pp->phy_rcv_data[2]);

	nav_timing = (u32)(navt_h<<8) + navt_l;
	nav_timing <<= 4;

	if(nav_timing > CFG_MAX_NAV_TIMING)
	{
#ifdef DEBUG_MAC
		my_printf("set_mac_nav_timing_from_esf():error nav value %X\r\n",nav_timing);
#endif
		nav_timing = CFG_MAX_NAV_TIMING;
	}
#ifdef DEBUG_MAC
	if(_mac_print_log_enable)
	{
		my_printf("NAV hold %dms\r\n",nav_timing);
	}
#endif

	mac_ctrl.mac_nav_timer = nav_timing;
}

/*******************************************************************************
* Function Name  : config_esf_xmode(COMM_MODE_TYPE comm_mode)
* Description    : esf֡ʹ�õĸ�ʽ
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
STATUS config_esf_xmode(COMM_MODE_TYPE comm_mode)
{
	if((comm_mode&0x03)>RATE_DS63)
	{
#ifdef DEBUG_MAC
		my_printf("error comm_mode\r\n");
#endif
		return FAIL;
	}

	if((comm_mode & 0xF0)==0)			//2015-04-08 ��ûָ��Ƶ��ʱ, Ĭ��ΪCH3��Ƶ�㷽ʽ
	{
		comm_mode |= CHNL_CH3;
	}
	esf_comm_mode = comm_mode;
	return OKAY;
}

/*******************************************************************************
* Function Name  : send_esf(u8 phase, u32 esf_timing)
* Description    : �����ŵ���ռ����, esf_timing�Ƕ�ռ�ŵ��ĺ�����
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
SEND_ID_TYPE send_esf(u8 phase, u16 nav_value)
{
	PHY_SEND_STRUCT pss;
	u8 psdu[2];
	SEND_ID_TYPE sid;

	if(phase>CFG_PHASE_CNT)
	{
		return INVALID_RESOURCE_ID;
	}

	if(nav_value > (CFG_MAX_NAV_VALUE))
	{
#ifdef DEBUG_MAC
		my_printf("nav_value limited\r\n");
#endif
		nav_value = CFG_MAX_NAV_VALUE;
	}

	pss.phase = phase;
	pss.prop = BIT_PHY_SEND_PROP_SRF|BIT_PHY_SEND_PROP_ESF;
	if((esf_comm_mode & CHNL_SALVO) == 0)	//���ûָ��Ƶ��, Ĭ��ΪCH3
	{
		esf_comm_mode = CHNL_CH3;
	}
	pss.prop |= (esf_comm_mode&CHNL_SCAN)?BIT_PHY_SEND_PROP_SCAN:0;
	pss.xmode = esf_comm_mode&0xF3;
	pss.delay = 0;							//ESF�������̷���
	psdu[0] = (u8)(nav_value>>8) & 0x0F;
	psdu[1] = (u8)(nav_value);
	pss.psdu = psdu;
	pss.psdu_len = 2;
	sid = phy_send(&pss);

	return sid;
}

/*******************************************************************************
* Function Name  : declare_channel_occupation(SEND_ID_TYPE sid, u32 nav_timing)
* Description    : ;
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
//STATUS declare_channel_occupation(SEND_ID_TYPE sid, u32 nav_timing)
//{
//	return set_queue_mac_hold(sid, TRUE, nav_timing);
//}


void declare_channel_monopoly(u32 over_time_stick)
{
	_mac_channel_monopoly_coverage = over_time_stick;
}


void cancel_channel_monopoly(void)
{
	_mac_channel_monopoly_coverage = 0;
}


/*******************************************************************************
* Function Name  : declare_channel_occupation(SEND_ID_TYPE sid, u32 nav_timing)
* Description    : ;
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
STATUS declare_channel_occupation(SEND_ID_TYPE dll_sid, u32 wait_time_after_sending)
{
	u32 nav_timing;

	nav_timing = calc_queue_sticks(dll_sid) + wait_time_after_sending + _mac_channel_monopoly_coverage;
	return set_queue_mac_hold(dll_sid, TRUE, nav_timing);
}




/*******************************************************************************
* Function Name  : mac_layer_proc()
* Description    : 2016-06-02 mac�㲻���ദ��, ���������յ����źŶ���csmaͳһ����
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void mac_layer_proc()
{
///* for debug */
//mac_ctrl.mac_nav_timer = 99;
///* */

	_mac_timer++;

	if(mac_ctrl.mac_nav_timer)
	{
		mac_ctrl.mac_state = MAC_STATE_NAV;		//NAV״̬���ȼ�������������޹�
	}
	else if(is_channel_busy())
	{
		mac_ctrl.mac_state = MAC_STATE_BUSY;
	}

//if(mac_ctrl.mac_state == MAC_STATE_ANNEAL)
//{
////		my_printf("%d\n",mac_ctrl.mac_universal_timer);
//	led_t_flash();
//}
//else
//{
//	led_t_off();
//}

	
	switch(mac_ctrl.mac_state)
	{
		case MAC_STATE_INITIAL:
		{
			mac_ctrl.mac_universal_timer = 2 * _mac_stick_base;
			mac_ctrl.mac_random_access_timer = 0;
			mac_ctrl.mac_random_access_turn = 0;
			mac_ctrl.mac_state = MAC_STATE_DISF;
			break;
		}
		case MAC_STATE_DISF:				//DISF���������պ��һС�˿�϶ʱ��,���û��Ҫ���͵�֡,һֱͣ����DISF״̬;
		{
			if(mac_ctrl.mac_universal_timer)
			{
				mac_ctrl.mac_universal_timer--;
			}
			else
			{
				// ������Ͷ�������mac�ܿص�����֡(delay=0, mac_hold=1), ת���������̬
				mac_ctrl.mac_ctrl_sid = is_queue_holding();
				if(mac_ctrl.mac_ctrl_sid != INVALID_RESOURCE_ID)
				{
					mac_ctrl.mac_random_access_timer = (rand() % get_random_access_window(get_nearby_active_nodes_num())) * _mac_stick_base;
					mac_ctrl.mac_random_access_turn = get_random_access_turn(get_nearby_active_nodes_num());
					mac_ctrl.mac_state = MAC_STATE_RANDOM_ACCESS;
				}
			}
			break;
		}
		case MAC_STATE_RANDOM_ACCESS:
		{
			if(mac_ctrl.mac_random_access_turn)
			{
				if(mac_ctrl.mac_random_access_timer)
				{
					mac_ctrl.mac_random_access_timer--;
				}
				else
				{
					u8 phase;
					u16 nav_value;

					phase = get_queue_phase(mac_ctrl.mac_ctrl_sid);
					nav_value = get_queue_nav_value(mac_ctrl.mac_ctrl_sid);
					mac_ctrl.esf_sid = send_esf(phase, nav_value);

					/* �����鵽esf_sidΪ��Ч, ˵��send_queue�Ѿ�û�пռ���, �������ǽ��䳷��, ����״̬ΪFAIL */
					if(mac_ctrl.esf_sid == INVALID_RESOURCE_ID)
					{
#ifdef DEBUG_MAC
						my_printf("mac_layer_proc():no resource for mac competition, send queue aborted.\r\n");
#endif
						cancel_pending_queue(mac_ctrl.mac_ctrl_sid);
						mac_ctrl.mac_state = MAC_STATE_INITIAL;
					}
					else
					{
						mac_ctrl.mac_state = MAC_STATE_ESF;
					}
				}
			}
			else
			{
				/* ���ת��MAC_STATE_RANDOM_ACCESS����turnΪ0, ˵������ʤ������ֱ�ӷ��� 
					Ҳ������nearby nodeΪ0, �õ���turnΪ0 */
				set_queue_mac_hold(mac_ctrl.mac_ctrl_sid, FALSE, 0);
				mac_ctrl.mac_state = MAC_STATE_SEND;
			}
			break;
		}
		case MAC_STATE_ESF:
		{
			SEND_STATUS send_status;
			
			send_status = inquire_send_status(mac_ctrl.esf_sid);
			switch(send_status)
			{
				case(SEND_STATUS_OKAY):
				{
					mac_ctrl.mac_random_access_timer = (rand() % get_random_access_window(get_nearby_active_nodes_num())) * _mac_stick_base;
					mac_ctrl.mac_random_access_turn--;
					mac_ctrl.mac_state = MAC_STATE_RANDOM_ACCESS;
					break;
				}
				case(SEND_STATUS_FAIL):
				case(SEND_STATUS_IDLE):
				case(SEND_STATUS_RESERVED):
				{
#ifdef DEBUG_MAC
					my_printf("mac_layer_proc():error esf send status, abort sending\r\n");
#endif
					cancel_pending_queue(mac_ctrl.mac_ctrl_sid);
					mac_ctrl.mac_state = MAC_STATE_INITIAL;
					break;
				}
				default:
				{
					//pending��sending��ֻ��ȴ�����
				}
			}
			break;
		}
		case MAC_STATE_SEND:
		{
			SEND_STATUS send_status;

			send_status = get_send_status(mac_ctrl.mac_ctrl_sid);	//�˴�ʹ�ò��ı�send״̬��get_send_status()
			switch(send_status)
			{
				case(SEND_STATUS_OKAY):
				case(SEND_STATUS_FAIL):						//�������ڶ���ʹ��require_send_status()��ѯ���Ѿ��ı���״̬
				case(SEND_STATUS_IDLE):
				case(SEND_STATUS_RESERVED):
				{
					mac_ctrl.mac_state = MAC_STATE_ANNEAL;
					mac_ctrl.mac_universal_timer = _mac_stick_anneal;
					break;
				}
				default:
				{
					//pending��sending��ֻ��ȴ�����
				}
			}
			break;
		}
		case MAC_STATE_BUSY:
		{
			if(!is_channel_busy())
			{
				mac_ctrl.mac_state = MAC_STATE_INITIAL;
			}
			break;
		}
		case MAC_STATE_NAV:
		{
			if(mac_ctrl.mac_nav_timer)
			{
				if(--mac_ctrl.mac_nav_timer == 0)
				{
					mac_ctrl.mac_state = MAC_STATE_INITIAL;
				}
			}
			break;
		}
		case MAC_STATE_ANNEAL:
		{
			if(mac_ctrl.mac_universal_timer)
			{
				if(--mac_ctrl.mac_universal_timer == 0)
				{
					mac_ctrl.mac_state = MAC_STATE_INITIAL;
				}
			}
			break;
		}
	}
}

#endif
