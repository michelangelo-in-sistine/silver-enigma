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
//	u8 conf;			// 交给app层之后, 保存信息, 用于返回时调用
//	u8 jumps;
//	u8 forward;
//	u8 stamps;
//	u8 ppdu_len;		// 本次数据包长
//}DST_BACKUP_STRUCT;

//typedef struct
//{
//	RATE_TYPE rate;
//	u8 jumps;
//	u8 forward_prop;	// [0 0 network_ena acps_ena 0 window_index[2:0]]
//
//	//	回复,转发使用
//	u8 conf;			// 交给app层之后, 保存信息, 用于返回时调用
//	u8 stamps;
//	u8 ppdu_len;		// 本次数据包长
//}DST_CONFIG_STRUCT;		// CC用这个数据体配置发送模式, MT用这个数据体设置回复模式


/* Private macro -------------------------------------------------------------*/


/* Private variables ---------------------------------------------------------*/
DST_STACK_RCV_STRUCT xdata _dst_rcv_obj[CFG_PHASE_CNT];
DST_CONFIG_STRUCT xdata dst_config_obj;

u8 xdata dst_sent_conf[CFG_PHASE_CNT];					// 发出的dst帧
u8 xdata dst_proceeded_conf[CFG_PHASE_CNT];				// 转发过的dst帧

#if (DEVICE_TYPE==DEVICE_CC) || (defined DEVICE_READING_CONTROLLER)
//u8 xdata dst_index[CFG_PHASE_CNT];
u8 xdata dst_index;
#else
//DST_BACKUP_STRUCT xdata dst_config_backup;				// MT的回复条件与接收条件相同
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
	mem_clr(&dst_proceeded_conf,sizeof(dst_proceeded_conf),1);	//防止第一帧因index为0不能被转发, 将其设置为绝不可能出现的状态
	
#if DEVICE_TYPE==DEVICE_CC
	//mem_clr(dst_index,sizeof(dst_index),1);
	dst_index = 0;
#endif
	//dst_config_obj.rate = RATE_BPSK;
	//dst_config_obj.jumps = 0x22;
	//dst_config_obj.forward_prop = 0x13;
	config_dst_flooding(0,0,0,0,0);
	dst_config_obj.forward_prop = 0;				//调用一次config_dst_flooding,省的出报警,同时又使得config_lock不启动
	//用于MT返回使用;
//#if DEVICE_TYPE==DEVICE_MT	
//	dst_config_obj.conf = CST_DST_PROTOCOL|BIT_DST_CONF_INDEX|BIT_DST_CONF_UPLINK;
//	dst_config_obj.ppdu_len = 0x20;
//#endif
}

/*******************************************************************************
* Function Name  : dst_send
* Description    : Send a DST protocol frame, 
					CC具有主动发起
					MT只是回复(SEARCH,洪泛读表),其属性与CC发起的属性相同
					2014-11-17
					由于林洋需要MT也可以主动发起DST(点对点),借此机会做如下修改:
					* 取消CC在app_send的时候对设置flooding的限制, 但flooding的设置取消
					* 统一使用dst_send作为入口
					* CC和MT都具有config_flooding的功能,每次config一次之后,下一次app_send就定义为主动发送
					* 在一次app_send执行完之前,MT的dst_config不会随接收洪泛包变化;
					* 目前只允许MT修改速率,跳数之类的还是0;
					
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
			conf = CST_DST_PROTOCOL |(dst_handle->search?BIT_DST_CONF_SEARCH:0) | get_dst_index(dst_handle->phase);	//暂时只允许CC管理DST Index, MT发出的都是无index的直通数据包
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
		conf = dst_config_obj.conf;					//转发conf不变
		*pt++ = conf;
		*pt++ = dst_config_obj.jumps-1;				//转发jump减1
	}
	
#if DEVICE_TYPE==DEVICE_CC
	*pt++ = dst_config_obj.forward_prop | ((dst_handle->search && dst_handle->search_mid)?BIT_DST_FORW_MID:0);
#else
	*pt++ = dst_config_obj.forward_prop;
#endif
	pt++;											//acps留空
	*pt++ = get_build_id(dst_handle->phase);		//network id


	/* 时间计算准备 */
	window_index = dst_config_obj.forward_prop&BIT_DST_FORW_WINDOW;
	total_window = 0x01<<window_index;


	/* 延时时间逻辑:
	* 延时时间由两部分组成: 1. 接收方轮次剩余窗口. 即total_window - window_stamp - 1. 例外:如total_jumps==left_jumps, 说明这是源第一次发出, 等待时间可以为0;
	2. 转发方窗口, 如自己需要转发, 此窗口为随机数对总窗口数取余. 例外: 如回复,且发现left_jumps==0, 说明本轮是最后一次接收, 没有再转发, 等待时间可以为0;
		至于转发则不会发现left_jumps==0, 因为过不了forward_necessary()这一关;
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

	dst_sent_conf[dst_handle->phase] = conf;		//记录发送的dst帧
	
	dss_handle->phase = dst_handle->phase;
	dss_handle->target_uid_handle = NULL;
	dss_handle->lsdu = dst_handle->nsdu;
	dss_handle->lsdu_len = dst_handle->nsdu_len + LEN_DST_NPCI;
#if DEVICE_TYPE == DEVICE_CC
	dss_handle->prop = (dst_handle->forward&BIT_DST_FORW_ACPS)?BIT_DLL_SEND_PROP_ACUPDATE:0;	//cc如果不指定相位,则正常发送,否则等待相位发送占据太长时间
#else
	dss_handle->prop = BIT_DLL_SEND_PROP_ACUPDATE;				//mt一律按acps发送
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
		if(check_dst_receivability(pd))						//检查接收性
		{
			conf = pd->lpdu[SEC_LPDU_DST_CONF];
			jumps = pd->lpdu[SEC_LPDU_DST_JUMPS];
			forward = pd->lpdu[SEC_LPDU_DST_FORW];

			dst_proceeded_conf[pd->phase] = conf;			//标志已处理
			dst_sent_conf[pd->phase] = 0;					//标志这一包开始不是自己发送的, 否则发送一次后可能同index的包将接不到

			/* 如无锁定DST全局变量将根据每一次接收到的洪泛设置而改变, 目前仅对MT有效 */
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
#if DEVICE_TYPE!=DEVICE_CC														//CC不能被Search搜索

				DST_SEND_STRUCT xdata dst;

				if(((forward&BIT_DST_FORW_MID)&&(check_meter_id(nsdu))) || 
					(!(forward&BIT_DST_FORW_MID)&&(check_uid(pd->phase,nsdu))))
				{
					dst.phase = pd->phase;
					dst.search = 1;
					dst.forward = 0;
					*(pd->lpdu + (pd->lpdu_len)) = conf;							//追加收到的conf
					get_uid(pd->phase, pd->lpdu + (pd->lpdu_len) + 1);			//追加uid
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
						pdst->apdu = nsdu;					//Search返回结果没有MPDU Head
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
				if(!pdst->dst_rcv_indication)				//返给应用层
				{
					pdst->phase = pd->phase;
					mem_cpy(pdst->src_uid,&pd->lpdu[SEC_LPDU_SRC],6);
					pdst->comm_mode = dst_config_obj.comm_mode;
					pdst->uplink = (conf & BIT_DST_CONF_UPLINK)?1:0;
					pdst->apdu = nsdu+1;					//去掉MPDU Head
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
* Description    : 判断当前dst帧完整性
* Input          : 
* Output         : 
* Return         : 成功返回NSDU地址, 失败返回NULL
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
* Description    : 判断当前dst帧是否处理过
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
RESULT check_dst_receivability(DLL_RCV_HANDLE pd)
{
	u8 conf;
	u8 forward;
	u8 jump;								//2014-03-21 原始跳数为0的dst帧不检查序列号

	u8 i;

	conf = pd->lpdu[SEC_LPDU_DST_CONF];
	forward = pd->lpdu[SEC_LPDU_DST_FORW];
	jump = pd->lpdu[SEC_LPDU_DST_JUMPS];


	//检查是否是自己刚发过的, 三相一起看
	if(jump&0xF0)							//2014-03-21 跳数为0的帧不检查序列号,不为0则检查重复性
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

		//检查是否是自己刚处理过的, 一相一相看
		if(conf==dst_proceeded_conf[pd->phase])
		{
#ifdef DEBUG_DST
			my_printf("repeated receiving\r\n");
#endif
			return WRONG;
		}
	}

	//检查相位关系, 过零点信号有效且不同相, 不接收; 过零点无效, 接收(不能使过零损坏的节点成为孤岛)
	if(forward & BIT_DST_FORW_ACPS)
	{
#if DEVICE_TYPE==DEVICE_CC
		if(!(is_zx_valid(pd->phase)) || (phy_acps_compare(GET_PHY_HANDLE(pd))!=0))	//CC必须有过零信号
#else
		if((is_zx_valid(pd->phase)) && (phy_acps_compare(GET_PHY_HANDLE(pd))!=0))	//无过零信号, 接收
#endif
		{
#ifdef DEBUG_DST
			my_printf("diff phase\r\n");
#endif
			return WRONG;
		}
	}

	/* 检查网络关系, 有网络号且不同网络, 不接收; 无有效网络号, 接收(不能使未组网的节点成为孤岛) */
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
* Description    : 判断当前dst帧是否需要转发, 如有以下条件之一者即不做转发
					* 源跳数或剩余跳数为0;
					* 帧中标定同相位转发,本地过零点信号有效且判断为非同相;
					* 帧中标定同网络转发,本地判断网络号不一致;
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
BOOL check_dst_forward_necessity(DLL_RCV_HANDLE pd)
{
#ifdef FLOODING_NO_FORWARD
	return FALSE;					//集中器不转发洪泛帧
#else

	u8 jumps;
	u8 forward;

	jumps = pd->lpdu[SEC_LPDU_DST_JUMPS];
	forward = pd->lpdu[SEC_LPDU_DST_FORW];


	/* 检查跳数 */
	if(!(jumps&0xF0) || !(jumps&0x0F))
	{
//#ifdef DEBUG_DST
//		my_printf("no jumps\r\n");
//#endif
		return FALSE;
	}

	/* 检查相位关系, 必须确定同相才转发, 防止跨相蔓延 */
	if(forward & BIT_DST_FORW_ACPS)
	{
		if(!is_zx_valid(pd->phase) || (phy_acps_compare(GET_PHY_HANDLE(pd))!=0))	//无过零信号,不转发
		{
//#ifdef DEBUG_DST
//			my_printf("diff phase\r\n");
//#endif
			return FALSE;
		}
	}

	/* 检查网络关系, 必须确定同网络才转发, 防止跨网络蔓延 */
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

	/* 通过上述所有检查的帧, 确认为需要转发 */
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
	if(!send_buffer)					//dst_forward必须用一段新的buffer,因为dst_send会篡改buffer内容
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
* 2015-03-12 瑞银需要点对点功能, 希望除了SCAN方式外还能有其他的控制方式
* 因此扩充原DST协议:从只能SCAN方式扩充成支持DLL层的所有通信类型,即
* (CH0+BPSK /CH1+BPSK /CH2+BPSK /CH3+BPSK /SCAN+BPSK /SALVO+BPSK)
* (CH0+DS15 /CH1+DS15 /CH2+DS15 /CH3+DS15 /SCAN+DS15 /SALVO+DS15)
* (CH0+DS63 /CH1+DS63 /CH2+DS63 /CH3+DS63 /SCAN+DS63 /SALVO+DS63)
* 方式为原仅表示速率的参数rate,扩展为comm_type,同时兼容原rate用法,用户将选项需要调用
* 例如config_dst_flooding(CHNL_CH2|RATE_63,0,0,0,0);

* STATUS config_dst_flooding(COMM_MODE_TYPE comm_mode, u8 max_jumps, u8 max_window_index, u8 acps_constrained, u8 network_constrained)
* 参数comm_mode可选择的频率参数: CHNL_CH0, CHNL_CH1, CHNL_CH2, CHNL_CH3, CHNL_SALVO, CHNL_SCAN; 可选择的速率参数: RATE_BPSK, RATE_DS15, RATE_DS63
* 当节点主动以DST方式使用app_send()通信执行下行请求前, 需要执行一次本函数, 以指定通信方式, 洪泛跳数, 窗口等
* 当节点需要上行答复调用app_send()时, 不需要调用本函数, 直接使用app_send()回复即可, 底层将直接以已接收的下行通行方式回复

* MT主动发起下行请求时, 请在调用本函数时把后四个参数设为0, 即MT仅可以主动点对点请求, 和做洪泛上行应答, 但不可以主动发起洪泛下行请求

* e.g.:
* config_dst_flooding(RATE_BPSK,0,0,0,0); 				//默认为SCAN+BPSK通信方式;
* config_dst_flooding(CHNL_CH0 | RATE_BPSK,0,0,0,0);	//使用CH0单频率BPSK
* config_dst_flooding(CHNL_CH3 | RATE_DS15,0,0,0,0);	//使用CH3单频率DS15
* config_dst_flooding(CHNL_SALVO | RATE_DS15,0,0,0,0);	//使用salvo模式(四频率齐发)DS15

* 注1: RATE不指定则默认为BPSK; CHNL不指定则默认为CHNL_SCAN;
* 注2: CHNL_SCAN将覆盖其余频率指定, 即CHNL_CH3 | CHNL_SCAN|RATE_DS15 的效果就等于CHNL_SCAN|RATE_DS15;
* 注3: 建议不要使用CHNL_CH0 | CHNL_CH1这种两个单频率的方式, 底层不对该模式进行检查;
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

	if((comm_mode & 0xFC)==0)			//2015-03-13 兼容原设计:当即不指定SCAN又不指定频率时,默认为SCAN方式
	{
		comm_mode |= (CHNL_CH0 | CHNL_SCAN);
	}
	else if((comm_mode & 0xF0)==0)		//2015-03-13 如果设置了SCAN,没设置频率,自动加一位频率位
	{
		comm_mode |= CHNL_CH0;
	}
	
	dst_config_obj.comm_mode = comm_mode;
	/***************************************
	* 2014-11-17 允许MT主动发送,但仅可以修改速率,仅点对点
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
	* 2014-11-17 CC允许修改洪泛其他设置
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

