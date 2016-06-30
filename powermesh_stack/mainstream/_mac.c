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
#define DEFAULT_MAC_WINDOW_INDEX	3		//缺省随机窗为2^3
#define MAX_MAC_WINDOW_INDEX		6		//最大随机窗为2^6
#define DSSS_ACCESS_DELAY_FACTOR	1.5		//DSSS接入需要的平均随机延时更长
#define MAX_NAV_TIMING				0xfff0	//最大NAV时间, 65520ms, 即一分钟
#define MAX_NAV_VALUE				0x0fff	//最大NAV值, 最小单位16ms

//#define CSMA_DEADLOCK_LIMIT			400		//CSMA标志刷新间隔最大不得超过400ms(DS63 expire = (24+11)*63/fd)
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
	MAC_STATE_ANNEAL				//由于信道检测落后与发送控制, 因此发送过的节点在竞争下一轮时有优势,增加一个"退火"状态
}MAC_STATE;


typedef struct
{
	MAC_STATE mac_state;


	u32 mac_universal_timer;		//作为DISF和ANNEAL定时

	SEND_ID_TYPE mac_ctrl_sid;
	SEND_ID_TYPE esf_sid;
	u8  mac_random_access_timer;	//ESF定时器接入
	u8  mac_random_access_turn;		//ESF竞争轮次

	u32 mac_nav_timer;
	
	//u8	mac_approved;				//mac准入级别
	//u8  mac_access_privilege;		//立即准入标志,不需csma
	//u8  window_index;
	
	//u32 mac_timing_expire;			//mac层接入的定时
	//u32 mac_timing_start;			//mac层接入起始时间


	//u32 nav_timing_setting;			//nav异步设入的接口
	//u32 nav_timing_start;			//nav定时起始时刻的时间
	//u32 nav_timing_expire;			//nav定时大小
	
}MEDIA_ACCESS_CTRL_STRUCT;


typedef struct
{
	u8 		phase;
	SEND_ID_TYPE sid_esf;
	SEND_ID_TYPE sid_phy;
	u32 	nav_timing;
	u32		wait_timing_expire;
	u32		wait_timing_cnt;
}MAC_QUEUE_SUPERVISE_STRUCT;		//发送队列mac控制


typedef struct
{
	u8		nearby_uid[6];			//附近监听到的主节点
	u32		last_timing;			//最后一次监听到的时间
}NEARBY_NODES_STRUCT;





typedef MEDIA_ACCESS_CTRL_STRUCT * MAC_CTRL_HANDLE;
typedef MAC_QUEUE_SUPERVISE_STRUCT * MAC_QUEUE_SUPERVISE_HANDLE;

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
MEDIA_ACCESS_CTRL_STRUCT mac_ctrl;

u8 esf_comm_mode;

NEARBY_NODES_STRUCT _nearby_nodes_db[CFG_MAX_NEARBY_NODES];
u8 _nearby_nodes_db_usage;
u32 _mac_timer = 0;					//维护一个mac层的全局时钟, 用于判断附近主节点超时用
u16 _mac_stick_base;
u16 _mac_stick_anneal;

BOOL _mac_print_log_enable = TRUE;
u32 _mac_channel_monopoly_coverage = 0;		//定义一个额外的延时, 使得对于连续的动作, 例如组网, 不容易被打断

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
* Description    : 将监听到的附近的节点UID保存到数据库中
					1. 若库中已有该节点地址, 更新其监听时间
					2. 若库中没有该节点, 若库还有空间, 在尾部增加该地址, 并更新库条目数
					3. 若库中已没有该节点, 若库没有空间, 则将最早监听到的节点排除, 将新监听的节点存入该地址
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
STATUS update_nearby_active_nodes(UID_HANDLE src_uid)
{
	u8 i;

	/**************************************************************************
	* 判断是否是主动节点, 现在没有更好的办法, 现在是使用DLCT_ACK标志位判断
	* 但MT在受控Diag, EBC的时候, 也会发出没有DLCT_ACK标志位
	* 因此暂时使用UID特征判断, 目前6810两版的UID号码分别为5E1D... 0B9A...
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

	//运行到这里, 说明库中没有找到该节点
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
* Description    : 获得当前的周边节点数
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
* Description    : 取出周边节点UID号集合
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
* Description    : 根据周边节点数,得到竞争窗口大小, 根据仿真得到(仿真计算的是总节点数, 这里需要抛去自身)
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
u8 get_random_access_window(u8 nearby_nodes)
{
	switch(nearby_nodes)
	{
		case 0:
			return 1;		//如果周围无节点, 直接发送即可
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
* Description    : 根据周边节点数,得到竞争轮次, 如果只有自己一个节点, 无需竞争
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
* Description    : esf帧使用的格式
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

	if((comm_mode & 0xF0)==0)			//2015-04-08 当没指定频率时, 默认为CH3单频点方式
	{
		comm_mode |= CHNL_CH3;
	}
	esf_comm_mode = comm_mode;
	return OKAY;
}

/*******************************************************************************
* Function Name  : send_esf(u8 phase, u32 esf_timing)
* Description    : 发送信道独占声明, esf_timing是独占信道的毫秒数
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
	if((esf_comm_mode & CHNL_SALVO) == 0)	//如果没指定频率, 默认为CH3
	{
		esf_comm_mode = CHNL_CH3;
	}
	pss.prop |= (esf_comm_mode&CHNL_SCAN)?BIT_PHY_SEND_PROP_SCAN:0;
	pss.xmode = esf_comm_mode&0xF3;
	pss.delay = 0;							//ESF必须立刻发出
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
* Description    : 2016-06-02 mac层不分相处理, 三相任意收到的信号都做csma统一处理
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
		mac_ctrl.mac_state = MAC_STATE_NAV;		//NAV状态优先级最高与其他都无关
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
		case MAC_STATE_DISF:				//DISF是留给接收后的一小端空隙时间,如果没有要发送的帧,一直停留在DISF状态;
		{
			if(mac_ctrl.mac_universal_timer)
			{
				mac_ctrl.mac_universal_timer--;
			}
			else
			{
				// 如果发送队列中有mac受控的数据帧(delay=0, mac_hold=1), 转入随机接入态
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

					/* 如果检查到esf_sid为无效, 说明send_queue已经没有空间了, 处理方法是将其撤销, 发送状态为FAIL */
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
				/* 如果转到MAC_STATE_RANDOM_ACCESS发现turn为0, 说明竞争胜利可以直接发送 
					也可能是nearby node为0, 得到的turn为0 */
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
					//pending和sending都只需等待即可
				}
			}
			break;
		}
		case MAC_STATE_SEND:
		{
			SEND_STATUS send_status;

			send_status = get_send_status(mac_ctrl.mac_ctrl_sid);	//此处使用不改变send状态的get_send_status()
			switch(send_status)
			{
				case(SEND_STATUS_OKAY):
				case(SEND_STATUS_FAIL):						//可能由于顶层使用require_send_status()查询后已经改变了状态
				case(SEND_STATUS_IDLE):
				case(SEND_STATUS_RESERVED):
				{
					mac_ctrl.mac_state = MAC_STATE_ANNEAL;
					mac_ctrl.mac_universal_timer = _mac_stick_anneal;
					break;
				}
				default:
				{
					//pending和sending都只需等待即可
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
