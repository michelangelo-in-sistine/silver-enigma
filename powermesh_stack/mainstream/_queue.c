/******************** (C) COPYRIGHT 2016 ***************************************
* File Name          : queue.c
* Author             : Lv Haifeng
* Version            : V2.0
* Date               : 2016-05-12
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

/* Private define ------------------------------------------------------------*/
u8 code CODE2CH_VAR[] = {0x10,0x20,0x40,0x80,0xf0,0xf0,0xf0,0xf0};		//ch info in SEC_PHPR convert to xmode
u8 code CH2CODE_VAR[] = {0x00,0x10,0x20,0x20,0x30,0x40,0x40,0x40};
#define PHPR_CH_INFO_ENCODE(xmode) (CH2CODE_VAR[(xmode>>5)])
#define CNT_DWA_WAITING	5000											//防死机计数器

/* Private typedef -----------------------------------------------------------*/
typedef struct
{
	u8 phase;
	ARRAY_HANDLE ppdu;
	BASE_LEN_TYPE ppdu_len;			// 原始ppdu数据长度
	u8 xmode;
	u8 prop;
	u32 delay;

	BASE_LEN_TYPE total_bytes;		// 增加编码后的所需发送字节数
	BASE_LEN_TYPE send_cnt;			// 已发送字节数
	u8 scan_order;			// 当前scan模式发送的信道
	SEND_STATUS status;		// 当前状态
#ifdef USE_MAC
	u16 nav_value;					// 需esf广播声明独占信道的NAV值, 每1单位16ms
#endif
}SEND_QUEUE_ITEM_STRUCT;

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
SEND_QUEUE_ITEM_STRUCT xdata _phy_send_queue[CFG_QUEUE_CNT];

#ifdef USE_MAC
u8 xdata _phy_send_state = 0;
#endif
/* Private function prototypes -----------------------------------------------*/
BASE_LEN_TYPE phy_complete_ppci_queue(SEND_QUEUE_ITEM_STRUCT * queue);

/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : init_queue()
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void init_queue()
{
	mem_clr(_phy_send_queue,sizeof(SEND_QUEUE_ITEM_STRUCT),CFG_QUEUE_CNT);
}

#if BRING_USER_DATA == 0
/*******************************************************************************
* Function Name  : check_availablue_queue()
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u8 check_available_queue(void)
{
	u8 i,j;
	
	j=0;
	for(i=0;i<CFG_QUEUE_CNT;i++)
	{
		if(_phy_send_queue[i].status == SEND_STATUS_IDLE || _phy_send_queue[i].status == SEND_STATUS_OKAY || _phy_send_queue[i].status == SEND_STATUS_FAIL)
		{
			j++;
		}
	}
	return j;
}
#endif

/*******************************************************************************
* Function Name  : inquire_send_status()
* Description    : 查询发送状态, 状态为OKAY或FAIL阅后即焚
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
SEND_STATUS inquire_send_status(SEND_ID_TYPE sid)
{
	SEND_STATUS status;

	if((sid < 0) || (sid >=CFG_QUEUE_CNT))
	{
#ifdef DEBUG_DISP_INFO
		my_printf("error sid %bx\r\n",sid);
#endif
		return SEND_STATUS_FAIL;
	}

	status = _phy_send_queue[sid].status;
	if(status == SEND_STATUS_OKAY || status == SEND_STATUS_FAIL)
	{
		_phy_send_queue[sid].status = SEND_STATUS_IDLE;		// burn after read
	}
	return status;
}

/*******************************************************************************
* Function Name  : req_send_queue()
* Description    : 申请send queue, 2013-03-07 modification:
					1. 申请时关闭中断, 防止多线程同时申请时出现问题;
					2. 增加occupied位, 防止多线程同时申请时出现问题;
					3. 优先使用IDLE的queue, 其次才使用success和fail的queue; 尽量减少同一queue刚刚释放就被重新使用的机会;
					4. 分配比较多的queue, 避免"查询机制+多线程+连续使用"造成的问题
* Input          : None
* Output         : None
* Return         : 成功, 返回queue_id, 失败返回-1;
*******************************************************************************/
SEND_ID_TYPE req_send_queue()
{
	u8 i;
	u8 sta;

	ENTER_CRITICAL();
	for(i=0;i<CFG_QUEUE_CNT;i++)
	{
		sta = _phy_send_queue[i].status;
		if(sta == SEND_STATUS_IDLE)
		{
			_phy_send_queue[i].status = SEND_STATUS_RESERVED;
			break;
		}
	}

	if(i>=CFG_QUEUE_CNT)
	{
		for(i=0;i<CFG_QUEUE_CNT;i++)
		{
			sta = _phy_send_queue[i].status;
			if(sta == SEND_STATUS_OKAY || sta == SEND_STATUS_FAIL)
			{
				_phy_send_queue[i].status = SEND_STATUS_RESERVED;
				break;
			}
		}
	}
	EXIT_CRITICAL();

	if(i>=CFG_QUEUE_CNT)
	{
		return INVALID_RESOURCE_ID;
	}
	else
	{
		return i;
	}
}

/*******************************************************************************
* Function Name  : push_send_queue()
* Description    : 发送数据体加入queue, 检查:
					1. 是否有效send_id;
					2. 是否对应send_id的queue有效;
					3. 是否有效相位;
					4. 是否有效发送模式;
					5. 是否有效发送数据长度;
* Input          : None
* Output         : None
* Return         : SUCCESS/FAIL
*******************************************************************************/
STATUS push_send_queue(SEND_ID_TYPE sid, PHY_SEND_HANDLE pss) reentrant
{
	SEND_QUEUE_ITEM_STRUCT xdata * queue;

	/* check sid validity */
	if(sid == INVALID_RESOURCE_ID || sid >= CFG_QUEUE_CNT)
	{
#ifdef DEBUG_DISP_INFO
		my_printf("invalid send_queue_id!\r\n");
#endif
		return FAIL;
	}

	/* check queue storage availability */
	queue = &_phy_send_queue[sid];
	if(queue->status != SEND_STATUS_RESERVED)
	{
#ifdef DEBUG_DISP_INFO
		my_printf("send queue is in use!\r\n");
#endif
		return FAIL;
	}

	/* check phase validility */
	if(pss->phase > CFG_PHASE_CNT)
	{
#ifdef DEBUG_DISP_INFO
		my_printf("error phase: %bX\r\n",pss->phase);
#endif
		queue->status = SEND_STATUS_IDLE;
		return FAIL;
	}

	/* check xmode validility */
	if((pss->xmode & 0xF0)==0 || (pss->xmode & 0x0F)==0x03)
	{
#ifdef DEBUG_DISP_INFO
		my_printf("error xmode: %bX\r\n",pss->xmode);
#endif
		queue->status = SEND_STATUS_IDLE;
		return FAIL;
	}

	/* check delay validility */
	if(pss->delay > MAX_QUEUE_DELAY)
	{
#ifdef DEBUG_DISP_INFO
		my_printf("too long send delay: %lu\r\n",pss->delay);
#endif	
		queue->status = SEND_STATUS_IDLE;
		return FAIL;
	}

	/* check data length validility */
	if(pss->psdu_len == 0)
	{
#ifdef DEBUG_DISP_INFO
		my_printf("0-byte length!\r\n");
#endif
		queue->status = SEND_STATUS_IDLE;
		return FAIL;
	}
#ifdef USE_RSCODEC	
	if((is_xmode_bpsk(pss->xmode) && (pss->psdu_len > CFG_RS_ENCODE_LENGTH))
		|| (is_xmode_dsss(pss->xmode) && ((pss->psdu_len + LEN_PPCI)> CFG_MEM_SUPERIOR_BLOCK_SIZE)))
#else
	if((pss->psdu_len + LEN_PPCI)> CFG_MEM_SUPERIOR_BLOCK_SIZE)
#endif		
	{
#ifdef DEBUG_DISP_INFO
		my_printf("send too long!\r\n");
#endif
		queue->status = SEND_STATUS_IDLE;
		return FAIL;
	}

	queue->ppdu = OSMemGet(SUPERIOR);
	if(!queue->ppdu)
	{
		queue->status = SEND_STATUS_IDLE;
		return FAIL;
	}
	else
	{
		queue->phase = pss->phase;
		/* 由于SCAN发送方式需要对PHPR重解码, 而且ACUPDATE也需要在发送前确定, 因此PPCI内容的填补放到Queue里完成 */
		if(pss->prop & BIT_PHY_SEND_PROP_SRF)
		{
			mem_cpy(&queue->ppdu[SEC_SRF_PSDU], pss->psdu, 2);
			queue->ppdu_len = pss->psdu_len + LEN_SRF_PPCI;
		}
		else
		{
			mem_cpy(&queue->ppdu[SEC_PSDU], pss->psdu, pss->psdu_len);
			queue->ppdu_len = pss->psdu_len + LEN_PPCI;
		}
		queue->xmode = pss->xmode;
		queue->prop = pss->prop;
		queue->delay = pss->delay;
		queue->send_cnt = 0;
		queue->scan_order = 0;
		queue->status = SEND_STATUS_PENDING;
		return OKAY;
	}
}

/*******************************************************************************
* Function Name  : phy_complete_ppci_queue()
* Description    : 基于Queue的PPCI补全, 包括填补以下内容:
					1. PHPR
					2. LEN
					3. CS/CRC
					4. AC_UPDATE
					5. if bpsk, rsencode

					PHPR, LEN, CS, CRC视为物理层的PPCI, 本函数仅处理这些内容
					
					psdu_len为未编码的数据长度, 由调用时计算得到;
					
* Input          : None
* Output         : None
* Return         : 数据包长度
*******************************************************************************/
BASE_LEN_TYPE phy_complete_ppci_queue(SEND_QUEUE_ITEM_STRUCT * queue)
{
	u8 send_prop;
	ARRAY_HANDLE ppdu;
	u16 tmp;
	BASE_LEN_TYPE len;
	send_prop = queue->prop;
	ppdu = queue->ppdu;
	len = queue->ppdu_len;

	/*如果是scan 数据包, 且不是第一包, 首先解RS*/
#ifdef USE_RSCODEC
	if(is_xmode_bpsk(queue->xmode) && !(send_prop & BIT_PHY_SEND_PROP_SRF) \
			&& (send_prop & BIT_PHY_SEND_PROP_SCAN) && (queue->scan_order))
	{
		rsencode_recover(ppdu,rsencoded_len(len));
	}
#endif

	/* PHPR 统一处理 */
	ppdu[SEC_SRF_PHPR] = FIRM_VER;
	if(send_prop & BIT_PHY_SEND_PROP_SCAN)
	{
		ppdu[SEC_SRF_PHPR] |= PHY_FLAG_SCAN;
		ppdu[SEC_SRF_PHPR] |= PHPR_CH_INFO_ENCODE(0x10<<(queue->scan_order));
	}
	else
	{
		ppdu[SEC_SRF_PHPR] |= PHPR_CH_INFO_ENCODE(queue->xmode);
	}

	/* 填充CS, CRC, AC_UPDATE 信息*/
	if(send_prop & BIT_PHY_SEND_PROP_SRF)
	{
#ifdef USE_MAC
		if(send_prop & BIT_PHY_SEND_PROP_ESF)
		{
			ppdu[SEC_SRF_PHPR] |= PHY_FLAG_NAV;
		}
#endif
		ppdu[SEC_SRF_PHPR] |= PHY_FLAG_SRF;
		tmp = calc_cs(ppdu,3);
		ppdu[SEC_SRF_CS] = tmp;
		len = 4;
	}
	else
	{
		ppdu[SEC_LENL] = (u8)len;
		ppdu[SEC_PHPR] |= (len>>8)?BIT_PHPR_LENH:0;	//2015-03-10 phpr最高位为lenh
		ppdu[SEC_CS] = 0;
		tmp = calc_cs(ppdu,10);				// cs位置移到第3字节
		ppdu[SEC_CS] = tmp;

		if(send_prop & BIT_PHY_SEND_PROP_ACUPDATE)
		{
			if(is_zx_valid(queue->phase))	
			{
				ppdu[SEC_NBF_COND] = read_reg(queue->phase,ADR_ACPS);
#ifdef CPU_STM32
				{	// cc处理时间比mt快,而spi时间比mt慢,因此二者相位差不同,相位差为CFG_CC_ACPS_DELAY(unit:0.1ms),cc端补足
					u8 tmp2;
					u32 cnt = 0;				//2014-03-21 需防acps锁死
					
					tmp = ppdu[SEC_NBF_COND];
					do
					{
						tmp2 = read_reg(queue->phase,ADR_ACPS);
						tmp2 = (tmp2>=tmp)?(tmp2-tmp):(tmp2-tmp+200);
						if(cnt++>10000)
						{
							ppdu[SEC_NBF_COND] = 0xFF;		//acps锁死, 跳出循环
							break;
						}
					}while(tmp2<CFG_CC_ACPS_DELAY);
				}
#endif
			}
			else
			{
				ppdu[SEC_NBF_COND] = 0xFF;		//FF表示电路坏损, 所有相位统统回应
			}
		}

		tmp = calc_crc(ppdu,len-2);
		ppdu[len-2] = tmp>>8;
		ppdu[len-1] = tmp;
#ifdef USE_RSCODEC
		if(is_xmode_bpsk(queue->xmode))
		{
			len = rsencode_vec(ppdu, len);
		}
#endif
	}

	return len;
}

/*******************************************************************************
* Function Name  : send_queue_proc()
* Description    : 发送队列处理状态机, 中断驱动
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
#define SEND_QUEUE_PROC_IDLE		0		//状态机空闲, 包括队列空或PENDING
#define SEND_QUEUE_PROC_SEND_BODY	1		//发送数据体中
#define SEND_QUEUE_PROC_SEND_EOP	2		//发送包尾结束标志
#define SEND_QUEUE_PROC_SCAN_INTERVAL 3		//发送SCAN类型间隔中;

void send_queue_proc()
{
	static u8 state = SEND_QUEUE_PROC_IDLE;
	static SEND_QUEUE_ITEM_STRUCT xdata * sending_queue;
	static u16 xdata dwa_cnt;				//dead waiting avoid counter;

	u8 i;
	u8 sta;

	/* delay proc 
	* delay proc are not rely on the machine state, in every state, the pending queue timing will count down.
	*/
	for(i=0;i<CFG_QUEUE_CNT;i++)
	{
		SEND_QUEUE_ITEM_STRUCT xdata * queue;

		queue = &_phy_send_queue[i];
		if(queue->status == SEND_STATUS_PENDING || queue->status == SEND_QUEUE_PROC_SCAN_INTERVAL)
		{
			if(queue->delay>0)
			{
				queue->delay--;
			}
		}
	}

	switch(state)
	{
		case(SEND_QUEUE_PROC_IDLE):
		{
			for(i=0;i<CFG_QUEUE_CNT;i++)
			{
				SEND_QUEUE_ITEM_STRUCT xdata * queue;
			
				queue = &_phy_send_queue[i];
				if(queue->status == SEND_STATUS_PENDING)
				{
#ifdef USE_MAC
					if(!(queue->prop & BIT_PHY_SEND_PROP_MAC_HOLD)) 		//检查queue的hold标志
#endif
					{
						if(queue->delay==0)
						{
							queue->total_bytes = phy_complete_ppci_queue(queue);

							if(queue->total_bytes)
							{
								write_reg_all(ADR_WPTD,0xFF);
								write_reg_all(ADR_XMT_RCV,0x80);			//防止一路发送时其他两路进入接收态;
#ifdef AMP_CONTROL							
								write_reg_all(ADR_XMT_AMP,CFG_XMT_AMP_DEFAULT_VALUE);
#endif
								write_reg(queue->phase,ADR_ADDA,0x00);
								if(queue->prop & BIT_PHY_SEND_PROP_SCAN)
								{
									write_reg(queue->phase,ADR_XMT_CTRL,(queue->xmode&0x0F)|0x10);
								}
								else
								{
									write_reg(queue->phase,ADR_XMT_CTRL,queue->xmode);
								}
								tx_on(queue->phase);
								send_buf(queue->phase,queue->ppdu[0]);
								queue->status = SEND_STATUS_SENDING;
								queue->send_cnt = 1;
								sending_queue = queue;
								state = SEND_QUEUE_PROC_SEND_BODY;
								dwa_cnt = 0;
#ifdef USE_MAC
								{
									extern u8 _phy_channel_busy_indication[];
									_phy_channel_busy_indication[queue->phase] = 0; 	//2016-06-06清除掉由于误同步导致的csma, 因为转为发送态后其不能被0x10中断取消
								}
#endif
#ifdef DEBUG_PHY
								if(!(sending_queue->prop & BIT_PHY_SEND_PROP_ESF))
								{
									my_printf("PHY:@%bx@%bx|",sending_queue->phase,read_reg(sending_queue->phase,ADR_XMT_CTRL));
									uart_send_asc(&queue->ppdu[0],1);
								}
#endif
								break;
							}
							else
							{
								/* if completement returns 0, abort sending */
								OSMemPut(SUPERIOR, queue->ppdu);
								queue->status = SEND_STATUS_FAIL;
								state = SEND_QUEUE_PROC_IDLE;
								break;
							}
						}
					}
				}
			}
			break;
		}
		case(SEND_QUEUE_PROC_SEND_BODY):
		{
			sta = read_reg(sending_queue->phase,ADR_XMT_STATUS);
			/* check buf_expire first */
			if((sta & 0x02) || (dwa_cnt++>CNT_DWA_WAITING))
			{
#ifdef DEBUG_DISP_INFO
				my_printf("XMT HARDWARE TIMEOUT!\r\n");
#endif
				write_reg(sending_queue->phase,ADR_XMT_STATUS,0x00);
				write_reg(sending_queue->phase,ADR_ADDA,0x00);
				write_reg_all(ADR_XMT_RCV,0x00);
				tx_off(0);
				tx_off(1);
				tx_off(2);
				OSMemPut(SUPERIOR, sending_queue->ppdu);
				sending_queue->status = SEND_STATUS_FAIL;
				state = SEND_QUEUE_PROC_IDLE;

				if(dwa_cnt>CNT_DWA_WAITING)
				{
#ifdef DEBUG_DISP_INFO
					my_printf("DWA CNT OVERFLOW!\r\n");
					//reset_system();
#endif
				}
			}
			else if(sta & 0x01)
			{
				if(sending_queue->send_cnt < sending_queue->total_bytes)
				{
					send_buf(sending_queue->phase, sending_queue->ppdu[sending_queue->send_cnt]);
#ifdef DEBUG_PHY
					if(!(sending_queue->prop & BIT_PHY_SEND_PROP_ESF))
					{
						uart_send_asc(&sending_queue->ppdu[sending_queue->send_cnt],1);
					}
#endif
					sending_queue->send_cnt++;
				}
				else
				{
					write_reg(sending_queue->phase,ADR_XMT_STATUS,0x80);
					state = SEND_QUEUE_PROC_SEND_EOP;
				}

				dwa_cnt = 0;
			}
			break;
		}
		case(SEND_QUEUE_PROC_SEND_EOP):
		{
			sta = read_reg(sending_queue->phase,ADR_XMT_STATUS);
			if(sta & 0x04)
			{
				write_reg(sending_queue->phase,ADR_XMT_STATUS,0x00);
				if(!(sending_queue->prop & BIT_PHY_SEND_PROP_SCAN)
					|| ((sending_queue->prop & BIT_PHY_SEND_PROP_SCAN) && sending_queue->scan_order >= 3) )
				{
					write_reg(sending_queue->phase,ADR_XMT_STATUS,0x00);
					write_reg(sending_queue->phase,ADR_ADDA,0x00);
					write_reg_all(ADR_XMT_RCV,0x00);
					tx_off(0);
					tx_off(1);
					tx_off(2);
					OSMemPut(SUPERIOR, sending_queue->ppdu);
					sending_queue->status = SEND_STATUS_OKAY;
					state = SEND_QUEUE_PROC_IDLE;
				
				}
				else
				{
					write_reg(sending_queue->phase,ADR_XMT_STATUS,0x00);
					write_reg_all(ADR_XMT_RCV,0x00);				// 复位收发状态;
					write_reg_all(ADR_XMT_RCV,0x80);				// 重新进入发送态
					sending_queue->scan_order++;
					sending_queue->delay=XMT_SCAN_INTERVAL_STICKS;
					state = SEND_QUEUE_PROC_SCAN_INTERVAL;			// SCAN_INTERVAL必须是单独的状态, 不能是PENDING, 防止被其他到期的QUEUE SEND任务将SCAN包打断
				}
#ifdef DEBUG_PHY
				if(!(sending_queue->prop & BIT_PHY_SEND_PROP_ESF))
				{
					my_printf("\r\n");
				}
#endif
			}
			else
			{
				if(dwa_cnt++>CNT_DWA_WAITING)
				{
#ifdef DEBUG_DISP_INFO
					my_printf("XMT HARDWARE FAULT!\r\n");
#endif			
					write_reg(sending_queue->phase,ADR_XMT_STATUS,0x00);
					write_reg_all(ADR_XMT_RCV,0x00);
					tx_off(0);
					tx_off(1);
					tx_off(2);
					OSMemPut(SUPERIOR, sending_queue->ppdu);
					sending_queue->status = SEND_STATUS_FAIL;
					state = SEND_QUEUE_PROC_IDLE;			
				}
			}
			break;
		}
		case(SEND_QUEUE_PROC_SCAN_INTERVAL):
		{
			if(sending_queue->delay==0)
			{
				write_reg(sending_queue->phase,ADR_XMT_CTRL,(sending_queue->xmode&0x0F) | (0x10<<sending_queue->scan_order));
				phy_complete_ppci_queue(sending_queue);
				write_reg(sending_queue->phase,0,0);	//time padding, make delay as same as the first package
				write_reg(sending_queue->phase,0,0);	//must be write operation, otherwise, it will be optimized by compiler
				write_reg(sending_queue->phase,0,0);
				write_reg(sending_queue->phase,0,0);
#ifdef AMP_CONTROL							
				write_reg_all(ADR_XMT_AMP,CFG_XMT_AMP_DEFAULT_VALUE);
#endif
				send_buf(sending_queue->phase,sending_queue->ppdu[0]);
				sending_queue->send_cnt=1;
				state = SEND_QUEUE_PROC_SEND_BODY;
#ifdef DEBUG_PHY
				if(!(sending_queue->prop & BIT_PHY_SEND_PROP_ESF))
				{
					my_printf("PHY:@%bx@%bx|",sending_queue->phase,read_reg(sending_queue->phase,ADR_XMT_CTRL));
					uart_send_asc(&sending_queue->ppdu[0],1);
				}
#endif
			}
			break;
		}
		default:
		{
#ifdef DEBUG_DISP_INFO
			my_printf("invalid proc state: %bu", state);
#endif
			state = SEND_QUEUE_PROC_IDLE;
		}
		
	}
#ifdef USE_MAC
	_phy_send_state = state;
#endif
	
}

/*******************************************************************************
* Function Name  : get_send_status()
* Description    : 查询发送状态, 不改变发送状态
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
SEND_STATUS get_send_status(SEND_ID_TYPE sid)
{
	return _phy_send_queue[sid].status;
}


/*******************************************************************************
* Function Name  : set_queue_delay()
* Description    : 重新设置queue的delay时间, 调用者保证sid有效
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void set_queue_delay(SEND_ID_TYPE sid, u32 delay)
{
	ENTER_CRITICAL();
	_phy_send_queue[sid].delay = delay;
my_printf("queue:%bu,set delay:%u\r\n",sid,delay);
	EXIT_CRITICAL();
}

#ifdef USE_MAC
/*******************************************************************************
* Function Name  : is_queue_pending()
* Description    : 检查物理层是否有发送在列
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
BOOL is_queue_pending(void)
{
	u8 i;
	for(i=0;i<CFG_QUEUE_CNT;i++)
	{
		if((_phy_send_queue[i].status == SEND_STATUS_PENDING))
		{
			return TRUE;
		}
	}
	return FALSE;
}

/*******************************************************************************
* Function Name  : cancel_queue_pending(SEND_ID_TYPE)
* Description    : 将队列中待发的撤销
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
STATUS cancel_pending_queue(SEND_ID_TYPE sid)
{
	if(_phy_send_queue[sid].status != SEND_STATUS_PENDING)
	{
		return FAIL;
	}
	else
	{
		_phy_send_queue[sid].status = SEND_STATUS_FAIL;
		OSMemPut(SUPERIOR,_phy_send_queue[sid].ppdu);
		return OKAY;
	}
}

/*******************************************************************************
* Function Name  : cancel_all_pending_queue(void)
* Description    : 撤销所有待发队列
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
void cancel_all_pending_queue(void)
{
	u8 i;
	for(i=0;i<CFG_QUEUE_CNT;i++)
	{
		cancel_pending_queue(i);
	}
}

/*******************************************************************************
* Function Name  : is_queue_holding()
* Description    : queue中是否有发送受mac控制的帧, 且delay已为0的帧存在
					
* Input          : None
* Output         : None
* Return         : QUEUE索引值, 失败则返回-1
*******************************************************************************/
SEND_ID_TYPE is_queue_holding(void)
{
	u8 i;
	
	for(i=0;i<CFG_QUEUE_CNT;i++)
	{
		if((_phy_send_queue[i].status == SEND_STATUS_PENDING)&&(_phy_send_queue[i].delay == 0)&&(_phy_send_queue[i].prop&BIT_PHY_SEND_PROP_MAC_HOLD))
		{
			return i;
		}
	}
	return INVALID_RESOURCE_ID;
}

/*******************************************************************************
* Function Name  : set_queue_mac_hold()
* Description    : 设置Queue的mac层hold标志, 设置该标志的帧, 需要csma和esf竞争才能发出, 没设置该标志的帧不受影响
					
* Input          : None
* Output         : None
* Return         : QUEUE索引值, 失败则返回-1
*******************************************************************************/
STATUS set_queue_mac_hold(SEND_ID_TYPE sid, BOOL enable, u32 nav_timing)
{
	if((sid>=CFG_QUEUE_CNT) || (_phy_send_queue[sid].status != SEND_STATUS_PENDING))
	{
#ifdef DEBUG_DISP_INFO
		my_printf("release_queue_mac_hold():error send id %bu\r\n",sid);
#endif
		return FAIL;
	}
	else
	{
		if(enable)
		{
			_phy_send_queue[sid].prop |= BIT_PHY_SEND_PROP_MAC_HOLD;
			if(nav_timing > CFG_MAX_NAV_TIMING)
			{
#ifdef DEBUG_DISP_INFO
				my_printf("release_queue_mac_hold():error nav_timing 0x%X, max 0x%X\r\n",nav_timing, CFG_MAX_NAV_TIMING);
#endif
				_phy_send_queue[sid].nav_value = CFG_MAX_NAV_VALUE;
			}
			else
			{
				_phy_send_queue[sid].nav_value = (u16)(nav_timing>>4);
				if((nav_timing & 0x0F) && (_phy_send_queue[sid].nav_value < CFG_MAX_NAV_VALUE))
				{
					_phy_send_queue[sid].nav_value++;					//如果设置nav_timing=fff0, 加1会溢出
				}
			}
			
		}
		else
		{
#ifdef DEBUG_MAC
			extern BOOL _mac_print_log_enable;
			if(_mac_print_log_enable)
			{
				if(_phy_send_queue[sid].prop & BIT_PHY_SEND_PROP_MAC_HOLD)
				{
					my_printf("NAV declare %X\r\n",_phy_send_queue[sid].nav_value);
				}
			}
#endif
			_phy_send_queue[sid].prop &= (~BIT_PHY_SEND_PROP_MAC_HOLD);
		}
	}
	return OKAY;
}

/*******************************************************************************
* Function Name  : get_queue_phase()
* Description    : 获得发送queue的相位, 调用者保证sid有效
					
* Input          : None
* Output         : None
* Return         : 失败返回0
*******************************************************************************/
u8 get_queue_phase(SEND_ID_TYPE sid)
{
	return _phy_send_queue[sid].phase;
}

/*******************************************************************************
* Function Name  : get_queue_nav_value()
* Description    : 获得queue的nav hold时间
					
* Input          : None
* Output         : None
* Return         : 失败返回0
*******************************************************************************/
u16 get_queue_nav_value(SEND_ID_TYPE sid)
{
	return _phy_send_queue[sid].nav_value;
}



/*******************************************************************************
* Function Name  : calc_queue_sticks()
* Description    : 计算队列中的项目的发送时间
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
u32 calc_queue_sticks(SEND_ID_TYPE sid)
{
	return phy_trans_sticks(_phy_send_queue[sid].ppdu_len, _phy_send_queue[sid].xmode & 0x03, _phy_send_queue[sid].prop & BIT_PHY_SEND_PROP_SCAN);
}



#endif


/******************* (C) COPYRIGHT 2016 *****END OF FILE****/

