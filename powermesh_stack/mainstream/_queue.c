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
#define CNT_DWA_WAITING	5000											//������������

/* Private typedef -----------------------------------------------------------*/
typedef struct
{
	u8 phase;
	ARRAY_HANDLE ppdu;
	BASE_LEN_TYPE ppdu_len;			// ԭʼppdu���ݳ���
	u8 xmode;
	u8 prop;
	u32 delay;

	BASE_LEN_TYPE total_bytes;		// ���ӱ��������跢���ֽ���
	BASE_LEN_TYPE send_cnt;			// �ѷ����ֽ���
	u8 scan_order;			// ��ǰscanģʽ���͵��ŵ�
	SEND_STATUS status;		// ��ǰ״̬
#ifdef USE_MAC
	u16 nav_value;					// ��esf�㲥������ռ�ŵ���NAVֵ, ÿ1��λ16ms
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
* Description    : ��ѯ����״̬, ״̬ΪOKAY��FAIL�ĺ󼴷�
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
* Description    : ����send queue, 2013-03-07 modification:
					1. ����ʱ�ر��ж�, ��ֹ���߳�ͬʱ����ʱ��������;
					2. ����occupiedλ, ��ֹ���߳�ͬʱ����ʱ��������;
					3. ����ʹ��IDLE��queue, ��β�ʹ��success��fail��queue; ��������ͬһqueue�ո��ͷžͱ�����ʹ�õĻ���;
					4. ����Ƚ϶��queue, ����"��ѯ����+���߳�+����ʹ��"��ɵ�����
* Input          : None
* Output         : None
* Return         : �ɹ�, ����queue_id, ʧ�ܷ���-1;
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
* Description    : �������������queue, ���:
					1. �Ƿ���Чsend_id;
					2. �Ƿ��Ӧsend_id��queue��Ч;
					3. �Ƿ���Ч��λ;
					4. �Ƿ���Ч����ģʽ;
					5. �Ƿ���Ч�������ݳ���;
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
		/* ����SCAN���ͷ�ʽ��Ҫ��PHPR�ؽ���, ����ACUPDATEҲ��Ҫ�ڷ���ǰȷ��, ���PPCI���ݵ���ŵ�Queue����� */
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
* Description    : ����Queue��PPCI��ȫ, �������������:
					1. PHPR
					2. LEN
					3. CS/CRC
					4. AC_UPDATE
					5. if bpsk, rsencode

					PHPR, LEN, CS, CRC��Ϊ������PPCI, ��������������Щ����
					
					psdu_lenΪδ��������ݳ���, �ɵ���ʱ����õ�;
					
* Input          : None
* Output         : None
* Return         : ���ݰ�����
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

	/*�����scan ���ݰ�, �Ҳ��ǵ�һ��, ���Ƚ�RS*/
#ifdef USE_RSCODEC
	if(is_xmode_bpsk(queue->xmode) && !(send_prop & BIT_PHY_SEND_PROP_SRF) \
			&& (send_prop & BIT_PHY_SEND_PROP_SCAN) && (queue->scan_order))
	{
		rsencode_recover(ppdu,rsencoded_len(len));
	}
#endif

	/* PHPR ͳһ���� */
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

	/* ���CS, CRC, AC_UPDATE ��Ϣ*/
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
		ppdu[SEC_PHPR] |= (len>>8)?BIT_PHPR_LENH:0;	//2015-03-10 phpr���λΪlenh
		ppdu[SEC_CS] = 0;
		tmp = calc_cs(ppdu,10);				// csλ���Ƶ���3�ֽ�
		ppdu[SEC_CS] = tmp;

		if(send_prop & BIT_PHY_SEND_PROP_ACUPDATE)
		{
			if(is_zx_valid(queue->phase))	
			{
				ppdu[SEC_NBF_COND] = read_reg(queue->phase,ADR_ACPS);
#ifdef CPU_STM32
				{	// cc����ʱ���mt��,��spiʱ���mt��,��˶�����λ�ͬ,��λ��ΪCFG_CC_ACPS_DELAY(unit:0.1ms),cc�˲���
					u8 tmp2;
					u32 cnt = 0;				//2014-03-21 ���acps����
					
					tmp = ppdu[SEC_NBF_COND];
					do
					{
						tmp2 = read_reg(queue->phase,ADR_ACPS);
						tmp2 = (tmp2>=tmp)?(tmp2-tmp):(tmp2-tmp+200);
						if(cnt++>10000)
						{
							ppdu[SEC_NBF_COND] = 0xFF;		//acps����, ����ѭ��
							break;
						}
					}while(tmp2<CFG_CC_ACPS_DELAY);
				}
#endif
			}
			else
			{
				ppdu[SEC_NBF_COND] = 0xFF;		//FF��ʾ��·����, ������λͳͳ��Ӧ
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
* Description    : ���Ͷ��д���״̬��, �ж�����
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
#define SEND_QUEUE_PROC_IDLE		0		//״̬������, �������пջ�PENDING
#define SEND_QUEUE_PROC_SEND_BODY	1		//������������
#define SEND_QUEUE_PROC_SEND_EOP	2		//���Ͱ�β������־
#define SEND_QUEUE_PROC_SCAN_INTERVAL 3		//����SCAN���ͼ����;

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
					if(!(queue->prop & BIT_PHY_SEND_PROP_MAC_HOLD)) 		//���queue��hold��־
#endif
					{
						if(queue->delay==0)
						{
							queue->total_bytes = phy_complete_ppci_queue(queue);

							if(queue->total_bytes)
							{
								write_reg_all(ADR_WPTD,0xFF);
								write_reg_all(ADR_XMT_RCV,0x80);			//��ֹһ·����ʱ������·�������̬;
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
									_phy_channel_busy_indication[queue->phase] = 0; 	//2016-06-06�����������ͬ�����µ�csma, ��ΪתΪ����̬���䲻�ܱ�0x10�ж�ȡ��
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
					write_reg_all(ADR_XMT_RCV,0x00);				// ��λ�շ�״̬;
					write_reg_all(ADR_XMT_RCV,0x80);				// ���½��뷢��̬
					sending_queue->scan_order++;
					sending_queue->delay=XMT_SCAN_INTERVAL_STICKS;
					state = SEND_QUEUE_PROC_SCAN_INTERVAL;			// SCAN_INTERVAL�����ǵ�����״̬, ������PENDING, ��ֹ���������ڵ�QUEUE SEND����SCAN�����
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
* Description    : ��ѯ����״̬, ���ı䷢��״̬
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
* Description    : ��������queue��delayʱ��, �����߱�֤sid��Ч
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
* Description    : ���������Ƿ��з�������
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
* Description    : �������д����ĳ���
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
* Description    : �������д�������
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
* Description    : queue���Ƿ��з�����mac���Ƶ�֡, ��delay��Ϊ0��֡����
					
* Input          : None
* Output         : None
* Return         : QUEUE����ֵ, ʧ���򷵻�-1
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
* Description    : ����Queue��mac��hold��־, ���øñ�־��֡, ��Ҫcsma��esf�������ܷ���, û���øñ�־��֡����Ӱ��
					
* Input          : None
* Output         : None
* Return         : QUEUE����ֵ, ʧ���򷵻�-1
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
					_phy_send_queue[sid].nav_value++;					//�������nav_timing=fff0, ��1�����
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
* Description    : ��÷���queue����λ, �����߱�֤sid��Ч
					
* Input          : None
* Output         : None
* Return         : ʧ�ܷ���0
*******************************************************************************/
u8 get_queue_phase(SEND_ID_TYPE sid)
{
	return _phy_send_queue[sid].phase;
}

/*******************************************************************************
* Function Name  : get_queue_nav_value()
* Description    : ���queue��nav holdʱ��
					
* Input          : None
* Output         : None
* Return         : ʧ�ܷ���0
*******************************************************************************/
u16 get_queue_nav_value(SEND_ID_TYPE sid)
{
	return _phy_send_queue[sid].nav_value;
}



/*******************************************************************************
* Function Name  : calc_queue_sticks()
* Description    : ��������е���Ŀ�ķ���ʱ��
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

