/******************** (C) COPYRIGHT 2012 Belling Inc. ********************
* File Name          : mgnt.c
* Author             : Lv Haifeng
* Version            : V2.0
* Date               : 2013-03-20
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
#include "ver_info.h"
//#include "compile_define.h"
//#include "powermesh_datatype.h"
//#include "powermesh_config.h"
//#include "powermesh_spec.h"
//#include "powermesh_timing.h"
//#include "general.h"
//#include "timer.h"
//#include "mem.h"
//#include "uart.h"
//#include "queue.h"
//#include "phy.h"
//#include "dll.h"
//#include "ebc.h"
//#include "addr_db.h"
//#include "psr.h"
//#include "psr_manipulation.h"


/* Private define ------------------------------------------------------------*/
#define DEBUG_INDICATOR DEBUG_APP

/* Private typedef -----------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/
#define GET_MGNT_HANDLE(pt) &_mgnt_rcv_obj[pt->phase]

/* Private variables ---------------------------------------------------------*/
MGNT_RCV_STRUCT xdata _mgnt_rcv_obj[CFG_PHASE_CNT];
APP_STACK_RCV_STRUCT xdata _app_rcv_obj[CFG_PHASE_CNT];
EBC_BROADCAST_STRUCT xdata _ebc_backup;

/* Private function prototypes -----------------------------------------------*/
void app_rcv_resume(APP_STACK_RCV_HANDLE pa) reentrant;
void mgnt_rcv_resume(MGNT_RCV_HANDLE pm) reentrant;

/* Private function -----------------------------------------------*/
/*******************************************************************************
* Function Name  : init_mgnt_app()
* Description    : 应用层发送接口
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void init_mgnt_app(void)
{
	mem_clr(_mgnt_rcv_obj,sizeof(MGNT_RCV_STRUCT),CFG_PHASE_CNT);	//2013-08-08 初始化mgnt数据体
	mem_clr(_app_rcv_obj,sizeof(APP_STACK_RCV_STRUCT),CFG_PHASE_CNT);
}


/*******************************************************************************
* Function Name  : app_send()
* Description    : 应用层发送接口
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
SEND_ID_TYPE app_send(APP_SEND_HANDLE pas)
{
	ARRAY_HANDLE send_struct;
	ARRAY_HANDLE send_buffer;
	SEND_ID_TYPE sid = INVALID_RESOURCE_ID;
	ARRAY_HANDLE ptw;

	// 检查数据体有效性	
	if((pas->apdu_len==0) || (pas->phase>=CFG_PHASE_CNT) || (pas->apdu_len>CFG_APDU_MAX_LENGTH))
	{
#ifdef DEBUG_APP
		my_printf("app_send(): phase %bd error or apdu_len %bd error\r\n",pas->phase,pas->apdu_len);
#endif
		return INVALID_RESOURCE_ID;
	}

	// 检查协议有效性
	switch(pas->protocol)
	{
#ifdef USE_PSR
		case(PROTOCOL_PSR):
		{
			if(!is_pipe_endpoint(pas->pipe_id))
			{
#ifdef DEBUG_APP
				my_printf("app_send(): invalid pipe_id %x\r\n",pas->pipe_id);
#endif
				return INVALID_RESOURCE_ID;
			}
			else
			{
				break;
			}
		}
#endif
		case(PROTOCOL_DST):
		{
//			if((pas->rate) > RATE_DS63)
//			{
//				return INVALID_RESOURCE_ID;
//			}
//			else
//			{
//				break;
//			}
			break;
		}
		default:
		{
#ifdef DEBUG_APP
			my_printf("app_send(): unknown protocol %bx\r\n",pas->protocol);
#endif

			return INVALID_RESOURCE_ID;		// unknown protocol
		}
	}
	
	// 申请资源
	send_buffer = OSMemGet(SUPERIOR);
	if(!send_buffer)
	{
		return INVALID_RESOURCE_ID;
	}

	send_struct = OSMemGet(MINOR);
	if(!send_struct)
	{
		OSMemPut(SUPERIOR,send_buffer);
		return INVALID_RESOURCE_ID;
	}

	// 对发送数据体清零, 对发送缓冲区增加MPDU_HEAD
	mem_clr(send_struct,CFG_MEM_MINOR_BLOCK_SIZE,1);
	ptw = send_buffer;
	*ptw++ = 0;											//APDU Head

	// 分协议类型启动发送
	switch(pas->protocol)
	{
#ifdef USE_PSR
		case(PROTOCOL_PSR):
		{
			PSR_SEND_HANDLE pss;

			// 装载数据体
			pss = (PSR_SEND_HANDLE)(send_struct);
			mem_cpy(ptw, pas->apdu, pas->apdu_len);			//ASDU Body
			pss->phase = pas->phase;
			pss->pipe_id = pas->pipe_id;
			pss->prop = 0;
			pss->index = get_psr_index(pas->phase);
			pss->uplink = is_pipe_endpoint_uplink(pas->pipe_id);
			pss->nsdu = send_buffer;
			pss->nsdu_len = pas->apdu_len + 1;
			sid = psr_send(pss);
			break;
		}
#endif
		case(PROTOCOL_DST):
		{
			DST_SEND_HANDLE dst;

			dst = (DST_SEND_HANDLE)(send_struct);
			mem_cpy(ptw,pas->apdu,pas->apdu_len);					//ASDU Body
			dst->phase = pas->phase;
			dst->forward = 0;										//根据dst_config 判断发送上下行
			dst->search = 0;
			dst->nsdu = send_buffer;
			dst->nsdu_len = pas->apdu_len + 1;
			sid = dst_send(dst);
			break;
		}
	}
	
	OSMemPut(MINOR,send_struct);
	OSMemPut(SUPERIOR,send_buffer);
	return sid;
}

#if DEVICE_TYPE == DEVICE_CC
/*******************************************************************************
* Function Name  : app_uid_send()
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
SEND_ID_TYPE app_uid_send(APP_SEND_HANDLE pas)
{
	pas->pipe_id = uid2pipeid(pas->target_uid);

	if(pas->pipe_id)
	{
		return app_send(pas);
	}
	else
	{
		return INVALID_RESOURCE_ID;
	}
}
#endif

/*******************************************************************************
* Function Name  : app_rcv()
* Description    : 应用层接收接口, 将接收到的用户层数据复制给外部提供的数据缓存;
					用户程序必须保证外部有足够的接收缓存
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
BASE_LEN_TYPE app_rcv(APP_RCV_HANDLE pa)
{
	u8 i;

#if DEVICE_TYPE==DEVICE_CC || DEVICE_TYPE==DEVICE_CV
	powermesh_main_thread_useless_resp_clear();
#endif

	for(i=0;i<CFG_PHASE_CNT;i++)
	{
		DST_STACK_RCV_HANDLE pdst;
#ifdef USE_PSR
		APP_STACK_RCV_HANDLE papp;
	
		papp = &_app_rcv_obj[i];				//用指针取代数组取址,可减小代码体积
		if(papp->app_rcv_indication)
		{
			pa->phase = papp->phase;
			pa->protocol = PROTOCOL_PSR;
			pa->pipe_id = papp->pipe_id;
			if(pa->apdu)
			{
				if(papp->apdu_len>CFG_APDU_MAX_LENGTH)			//防止写越界
				{
					papp->apdu_len = CFG_APDU_MAX_LENGTH;
				}
				mem_cpy(pa->apdu,papp->apdu,papp->apdu_len);
			}
			pa->apdu_len = papp->apdu_len;
			mem_clr(pa->src_uid,6,1);
			pa->comm_mode = 0;
#if APP_RCV_SS_SNR == 1
			pa->ss = get_phy_ss(GET_PHY_HANDLE(papp));
			pa->snr = get_phy_snr(GET_PHY_HANDLE(papp));
#endif
			app_rcv_resume(papp);
			return pa->apdu_len;
		}
#endif
		pdst = &_dst_rcv_obj[i];
		if(pdst->dst_rcv_indication)
		{
			pa->phase = pdst->phase;
			pa->protocol = PROTOCOL_DST;
			pa->pipe_id = 0;
			if(pa->apdu)
			{
				if(pdst->apdu_len>CFG_APDU_MAX_LENGTH)			//防止写越界
				{
					pdst->apdu_len = CFG_APDU_MAX_LENGTH;
				}
				mem_cpy(pa->apdu,pdst->apdu,pdst->apdu_len);
			}

			pa->apdu_len = pdst->apdu_len;
			mem_cpy(pa->src_uid,pdst->src_uid,6);
			pa->comm_mode = pdst->comm_mode;
#if APP_RCV_SS_SNR == 1
			pa->ss = get_phy_ss(GET_PHY_HANDLE(pdst));
			pa->snr = get_phy_snr(GET_PHY_HANDLE(pdst));
#endif
			dst_rcv_resume(pdst);
			return pa->apdu_len;
		}
	}
	return 0;
}

/* Private functions ---------------------------------------------------------*/
#ifdef USE_PSR
/*******************************************************************************
* Function Name  : app_rcv_resume
* Description    : 接收机继续接收
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void app_rcv_resume(APP_STACK_RCV_HANDLE pa) reentrant
{
	psr_rcv_resume(GET_NW_HANDLE(pa));
	pa->app_rcv_indication = 0;
}

/*******************************************************************************
* Function Name  : mgnt_rcv
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
MGNT_RCV_HANDLE mgnt_rcv()
{
	u8 i;

	for(i=0;i<CFG_PHASE_CNT;i++)
	{
		if(_mgnt_rcv_obj[i].mgnt_rcv_indication)
		{
			return &_mgnt_rcv_obj[i];
		}
	}
	return NULL;
}


/*******************************************************************************
* Function Name  : mgnt_rcv_resume()
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void mgnt_rcv_resume(MGNT_RCV_HANDLE pm) reentrant
{
	psr_rcv_resume(GET_NW_HANDLE(pm));
	pm->mgnt_rcv_valid = 0;
	pm->mgnt_rcv_indication = 0;
}

/*******************************************************************************
* Function Name  : mgnt_proc_mpdu_send_common_fill()
* Description    : 填充管理层返回的mpdu基本信息
					1. 命令; 2. uid; 不填0x80, 因为执行命令未必成功;
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
ARRAY_HANDLE mgnt_proc_mpdu_send_common_fill(u8 phase, ARRAY_HANDLE mpdu_send, u8 mpdu_cmd)
{
	phase = phase;
	*(mpdu_send++) = mpdu_cmd|0xC0;
	get_uid(phase,mpdu_send);
	mpdu_send += 6;
	return mpdu_send;
}

/*******************************************************************************
* Function Name  : mgnt_proc_psr_send_struct_common_fill()
* Description    : 填充psr发送结构体
					1.相位; 2. pipe_id; 3.index. 4.方向; 5.nsdu地址
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void mgnt_proc_psr_send_struct_common_fill(PSR_SEND_HANDLE pt_psr_send, PSR_RCV_HANDLE pt_psr_rcv, ARRAY_HANDLE mpdu_send)
{
	pt_psr_send->phase = pt_psr_rcv->phase;
	pt_psr_send->pipe_id = pt_psr_rcv->pipe_id;
	pt_psr_send->prop = 0;
	pt_psr_send->index = pt_psr_rcv->psr_rcv_valid & BIT_PSR_RCV_INDEX;
	pt_psr_send->uplink = !(pt_psr_rcv->psr_rcv_valid & BIT_PSR_RCV_UPLINK);	//方向取反
	pt_psr_send->nsdu = mpdu_send;
}


/*******************************************************************************
* Function Name  : mgnt_proc()
* Description    : 管理层事务处理, 管理命令执行, 主循环中执行
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void mgnt_proc(MGNT_RCV_HANDLE pm)
{
	ARRAY_HANDLE mpdu;
	ARRAY_HANDLE mpdu_send;
	PSR_RCV_HANDLE pn;

	u8 xdata mpdu_cmd;
	PSR_SEND_STRUCT xdata pss;
	
	if(pm->mgnt_rcv_valid)
	{
		pn = GET_NW_HANDLE(pm);

		mpdu = pm->mpdu;
		mpdu_cmd = mpdu[0];
		if(!(mpdu_cmd & 0x40))			// not answer
		{
			mpdu_send = OSMemGet(SUPERIOR);
			if(mpdu_send)
			{
				ARRAY_HANDLE ptr;
				ARRAY_HANDLE ptw;

				mgnt_proc_psr_send_struct_common_fill(&pss,pn,mpdu_send);	//返回信息必须在释放前添加,否则释放后信息丢失
				switch(mpdu_cmd & 0x7F)
				{
					case(EXP_MGNT_PING):
					{
						check_and_clear_send_status();
						
						ptw = mgnt_proc_mpdu_send_common_fill(pn->phase,mpdu_send,EXP_MGNT_PING);
						*ptw++ = 0x80;

						/*
							2014-09-24 林洋要求加入组网时携带用户信息(DLMS16字节表号)
							决定在PING中携带此信息
							同时修改PING分组定义
							原定义如下:
							[is_zx_valid YEAR MONTH DAY HOUR check_available_timer check_available_mem check_available_mem check_available_queue build_id is_vhh_low]
						*/

#if !(defined BRING_USER_DATA) || (BRING_USER_DATA == 0)
						*ptw++ = is_zx_valid(pn->phase);					//返回过零点是否有效;
						*ptw++ = VER_YEAR;
						*ptw++ = VER_MONTH;
						*ptw++ = VER_DAY;
						*ptw++ = VER_HOUR;
						*ptw++ = check_available_timer();
						*ptw++ = check_available_mem(MINOR);
						*ptw++ = check_available_mem(SUPERIOR);
						*ptw++ = check_available_queue();
						*ptw++ = get_build_id(0);
#ifdef AMP_CONTROL						
						*ptw++ = !PIN_LOW_POWER;
#else
						*ptw++ = 0;
#endif
						pss.nsdu_len = 18;

						//改成如下定义
						//[YEAR MONTH DAY HOUR {1'b0, 1'b0, 1'b0, 1'b0, 1'b0, 1'b0, is_vhh_low, is_zx_valid} build_id user_data[20] ]
						//路由器根据新的版本号位置判断如何判断

#else						
						*ptw++ = VER_YEAR;
						*ptw++ = VER_MONTH;
						*ptw++ = VER_DAY;
						*ptw++ = VER_HOUR;
						*ptw = is_zx_valid(pn->phase) ? 0x01 : 0x00;		//zx有效
#ifdef AMP_CONTROL
						*ptw |= (PIN_LOW_POWER ? 0x00 : 0x02);			//PIN=0, 对应位为1
#endif
						ptw++;
						*ptw++ = get_build_id(0);
						mem_cpy(ptw,_user_data,_user_data_len);
						ptw += _user_data_len;
						pss.nsdu_len = 14 + _user_data_len;
#endif
						mgnt_rcv_resume(pm);
						break;
					}
					case(EXP_MGNT_DIAG):
					{
						DLL_SEND_STRUCT xdata ds;
						u8 xdata target_uid[6];
						STATUS status;
						u8 return_len;
						
						ds.phase = pn->phase;
						mem_cpy(target_uid,&mpdu[1],6);		//发送diag之前需释放底层, 因此不能以接收缓冲作为地址指向的实际地址
						ds.target_uid_handle = target_uid;
						ds.xmode = mpdu[7];
						ds.rmode = mpdu[8];
						ds.prop = BIT_DLL_SEND_PROP_DIAG | BIT_DLL_SEND_PROP_REQ_ACK;
						ds.prop |= mpdu[9]?(BIT_DLL_SEND_PROP_SCAN):0;
						ptw = mgnt_proc_mpdu_send_common_fill(pn->phase,mpdu_send,EXP_MGNT_DIAG);
						mgnt_rcv_resume(pm);
						status = dll_diag(&ds,ptw);
						if(status)
						{
							return_len = *ptw;					//写入首字节为数据长度
							*ptw = 0x80;
							pss.nsdu_len = 8 + return_len;
						}
						else
						{
							*ptw = DIAG_TIMEOUT;
							pss.nsdu_len = 8;
						}
						break;
					}
					case(EXP_MGNT_EBC_BROADCAST):
					{
						EBC_BROADCAST_STRUCT xdata ebc;
						u8 num_bsrf;

						ptr = &mpdu[1];

						mem_clr(&ebc,sizeof(EBC_BROADCAST_STRUCT),1);
						ebc.phase = pn->phase;
						ebc.bid = *ptr++ & 0x0F;
						ebc.xmode = *ptr++;
						ebc.rmode = *ptr++;
						ebc.scan = *ptr++;
						ebc.window = *ptr++;
						ebc.mask = *ptr++;

						
						//if(ebc.mask & BIT_NBF_MASK_ACPHASE)
						//if acps enable, ebc_broadcast should set the mask bit
						
						//... fill other conditions
						if(ebc.mask & BIT_NBF_MASK_UID)
						{
							mem_cpy(ebc.uid,ptr,6);
							ptr += 6;
						}
						if(ebc.mask & BIT_NBF_MASK_METERID)
						{
							mem_cpy(ebc.mid,ptr,6);
							ptr += 6;
						}
						if(ebc.mask & BIT_NBF_MASK_BUILDID)
						{
							ebc.build_id = *(ptr++);
						}
						
						mem_cpy((ARRAY_HANDLE)(&_ebc_backup),(ARRAY_HANDLE)(&ebc),sizeof(EBC_BROADCAST_STRUCT));	// backup ebc setting;

						ptw = mgnt_proc_mpdu_send_common_fill(pn->phase,mpdu_send,EXP_MGNT_EBC_BROADCAST);
						mgnt_rcv_resume(pm);
						num_bsrf = ebc_broadcast(&ebc);

						*ptw++ = 0x80;
						*ptw++ = ebc.bid;
						*ptw++ = num_bsrf;
						pss.nsdu_len = 10;
						break;
					}
					case(EXP_MGNT_EBC_IDENTIFY):
					{
						u8 xdata num_nodes;
//						u8 xdata ebc_id;

//						ebc_id = *(mpdu++);		//// 不利用传来的ebc_id
						num_nodes = mpdu[2];
						_ebc_backup.max_identify_try = mpdu[3];

						ptw = mgnt_proc_mpdu_send_common_fill(pn->phase,mpdu_send,EXP_MGNT_EBC_IDENTIFY);
						mgnt_rcv_resume(pm);
						num_nodes = ebc_identify_transaction(&_ebc_backup,num_nodes);

						*ptw++ = 0x80;
						*ptw++ = _ebc_backup.bid;
						*ptw++ = num_nodes;
						pss.nsdu_len = 10;
						break;
					}
					case(EXP_MGNT_EBC_ACQUIRE):
					{
						u8 node_index;
						u8 node_num;
						u8 i;

						node_index = mpdu[2];
						node_num = mpdu[3];

						ptw = mgnt_proc_mpdu_send_common_fill(pn->phase,mpdu_send,EXP_MGNT_EBC_ACQUIRE);
						mgnt_rcv_resume(pm);

						*ptw++ = 0x80;
						*ptw++ = _ebc_backup.bid;
						for(i = node_index;i<node_index+node_num;i++)
						{
							if(inquire_neighbor_uid_db(i,ptw))
							{
								ptw += 6;
							}
						}
						pss.nsdu_len = ptw-mpdu_send;
						break;
	
					}
					case(EXP_MGNT_SET_BUILD_ID):
					{
						u8 new_build_id;
						u8 phase;

						phase = pm->phase;
						new_build_id = mpdu[1];
						ptw = mgnt_proc_mpdu_send_common_fill(pn->phase,mpdu_send,EXP_MGNT_SET_BUILD_ID);
						mgnt_rcv_resume(pm);

						set_build_id(phase, new_build_id);
						*ptw++ = 0x80;
						*ptw++ = get_build_id(phase);
						

						pss.nsdu_len = ptw-mpdu_send;
						break;
					}	
					default:
					{
#ifdef DEBUG_MGNT
						my_printf("UNKNOWN MGNT CMD %bX, NPDU LEN %bX\r\n", (mpdu_cmd & 0x7F),(pn->npdu_len));
						print_phy_rcv(GET_PHY_HANDLE(pm));
#endif
						mgnt_rcv_resume(pm);
						break;
					}
				}
				psr_send(&pss);
				
				OSMemPut(SUPERIOR,mpdu_send);
			}//如申请不到buffer, 可能是底层通信占用了, 等待;
		}
		else
		{
			
		}
	}
}



/*******************************************************************************
* Function Name  : mgnt_app_rcv_proc()
* Description    : 管理层/APP层区分, 汇报, 中断中执行
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void mgnt_app_rcv_proc(APP_STACK_RCV_HANDLE papp)
{
	PSR_RCV_HANDLE ppsr;
	MGNT_RCV_HANDLE pm;

	if(!papp->app_rcv_indication)
	{
		ppsr = GET_NW_HANDLE(papp);
		if((ppsr->psr_rcv_valid) && (ppsr->npdu_len > LEN_NPCI) && !(ppsr->psr_rcv_valid & BIT_PSR_RCV_PATROL))
		{
			ARRAY_HANDLE nsdu = &ppsr->npdu[SEC_NPDU_PSR_NSDU];
			u8 mpdu_head = nsdu[0];
			
			if(!(mpdu_head & 0x80))				//首字节为0, 数据为APDU, 接收并向上汇报
			{
				papp->phase = ppsr->phase;
				papp->pipe_id = ppsr->pipe_id;
				papp->apdu = &nsdu[1];
				papp->apdu_len = ppsr->npdu_len-LEN_NPCI-LEN_MPCI;
				if(papp->apdu_len)
				{
					papp->app_rcv_indication = 1;
				}
				else
				{
					app_rcv_resume(papp);
				}
			}
			else
			{
				pm = GET_MGNT_HANDLE(papp);
				pm->phase = ppsr->phase;
				pm->pipe_id = ppsr->pipe_id;
				pm->mpdu = nsdu;
				pm->mpdu_len = ppsr->npdu_len-LEN_NPCI;
				if(pm->mpdu_len>=LEN_MPCI)
				{
					pm->mgnt_rcv_valid = mpdu_head & 0xC0;
					if(mpdu_head & 0x40)			
					{
						pm->mgnt_rcv_indication = 1;	//不管是不是mgnt层的响应, 都需主线程处理;
														//但普通命令, 在event_proc中处理, indication的属于回应, 也在垃圾处理中用到, 故单独分开;
					}
				}
				else
				{
					mgnt_rcv_resume(pm);
				}
			}
		}
	}
}
#endif
#if APP_RCV_SS_SNR == 1
/*******************************************************************************
* Function Name  : get_phy_ss
* Description    : 获得底层接收的信号强度, 获得的是四个频率接收的信号强度中最大的值
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
s8 get_phy_ss(PHY_RCV_HANDLE pp)
{
	s8 ss = 0x80;
	u8 i;
	//
	for(i=0;i<4;i++)
	{
		if(ss < pp->phy_rcv_ss[i])
		{
			ss = pp->phy_rcv_ss[i];
		}
	}
	return ss;
}

/*******************************************************************************
* Function Name  : get_phy_snr
* Description    : 获得底层接收的信噪比, 获得的是四个频率接收的信号信噪比中最大的值
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
s8 get_phy_snr(PHY_RCV_HANDLE pp)
{
	s8 snr = 0x80;
	u8 i;
	//
	for(i=0;i<4;i++)
	{
		if(snr < pp->phy_rcv_ebn0[i])
		{
			snr = pp->phy_rcv_ebn0[i];
		}
	}
	return snr;
}
#endif

#if DEVICE_TYPE==DEVICE_CC || DEVICE_TYPE==DEVICE_CV
/*******************************************************************************
* Function Name  : app_transaction()
* Description    : 阻塞方式实现的app层一次应答
* Input          : app_send_struct
* Output         : app_rcv_struct
* Return         : 状态
*******************************************************************************/
STATUS app_transaction(APP_SEND_HANDLE pas, APP_RCV_HANDLE pv, BASE_LEN_TYPE asdu_uplink_len, u16 app_delay)
{
	SEND_ID_TYPE sid;
	TIMER_ID_TYPE tid;
	u32 wait_sticks;
	STATUS status;
//	u8 nsdu_buffer[256];
//	u8 return_buffer[256];
//	ERROR_STRUCT psr_err;
	u8 i;
	u8 max_retry;

	status = FAIL;

	// validality check

	if(pas->protocol == PROTOCOL_PSR)
	{
		wait_sticks = app_transaction_sticks(pas->pipe_id, pas->apdu_len, asdu_uplink_len, app_delay);
		max_retry = CFG_MAX_PSR_TRANSACTION_TRY;

	}
	else if(pas->protocol == PROTOCOL_DST)
	{
		wait_sticks = dst_transaction_sticks(pas->apdu_len, asdu_uplink_len, app_delay);
		max_retry = CFG_MAX_DST_TRANSACTION_TRY;
	}
	else
	{
		my_printf("error protocol\r\n");
		return status;
	}

	// req timer
	tid = req_timer();
	if(tid==INVALID_RESOURCE_ID)
	{
		return status;
	}
	
	// transaction
	for(i=0;i<max_retry;i++)
	{
		SET_BREAK_THROUGH("quit app_transaction()\r\n");

#ifdef USE_MAC
		declare_channel_occupation(pas->phase,wait_sticks);		//transaction sticks包括了发送
#endif

		sid = app_send(pas);
		if(sid != INVALID_RESOURCE_ID)
		{
			wait_until_send_over(sid);
			set_timer(tid, wait_sticks);
#ifdef DEBUG_INDICATOR
			my_printf("wait_sticks:%d\r\n",wait_sticks);
#endif
			do
			{
				SET_BREAK_THROUGH("quit app_transaction()\r\n");
				FEED_DOG();
				if(app_rcv(pv))
				{
					if((pv->phase == pas->phase)&&(pv->protocol==pas->protocol))
					{
						status = OKAY;
					}
				}
				
				if(status == OKAY)						//洪泛必须等上行帧都转发完成才结束
				{
					if(pv->protocol != PROTOCOL_DST)
					{
						break;
					}
				}
			}while(!is_timer_over(tid));
		}
	}

	delete_timer(tid);
	return status;
}


#ifdef USE_MAC
/*******************************************************************************
* Function Name  : SEND_ID_TYPE app_send_ca(APP_SEND_HANDLE app_send_handle, u32 nav_timing, u32 max_wait_timing)
* Description    : 以csma-ca发送独占信道声明,app_send
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
SEND_ID_TYPE app_send_ca(APP_SEND_HANDLE app_send_handle, u32 transaction_timing, u32 max_send_hold_timing)
{
	SEND_ID_TYPE sid_esf, sid_app;
	u8 phase;

	phase = app_send_handle->phase;
	if(phase > CFG_PHASE_CNT)
	{
		return INVALID_RESOURCE_ID;
	}

	sid_esf = INVALID_RESOURCE_ID;							//set default value
	sid_app = INVALID_RESOURCE_ID;

	sid_esf = send_esf(app_send_handle->phase, transaction_timing);

	if(sid_esf != INVALID_RESOURCE_ID)
	{
		sid_app = app_send(app_send_handle);
		if(sid_app != INVALID_RESOURCE_ID)
		{
			req_mac_queue_supervise(app_send_handle->phase,sid_esf,sid_app,transaction_timing,max_send_hold_timing);
		}
	}
	return sid_app;				//返回esf, esf 发送结束的时候启动定时, 原来的计算transaction的时间函数就不用改了
}

#endif

#endif
/******************* (C) COPYRIGHT 2012 Belling *****END OF FILE****/

