/******************** (C) COPYRIGHT 2016 ***************************************
* File Name          : phy.c
* Author             : Lv Haifeng
* Version            : V 2.0
* Date               : 2013-03-08
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
//#include "powermesh_timing.h"9
//#include "hardware.h"
//#include "general.h"
//#include "rscodec.h"
//#include "timer.h"
//#include "uart.h"
//#include "phy.h"
//#include "queue.h"


/* Private macros -----------------------------------------------------------*/
#define PARITY_ERR_TOL 3
#define PHPR_CH_INFO_DECODE(code)  (CODE2CH_VAR[code])

/* Private typedef -----------------------------------------------------------*/


/* Extern variables ---------------------------------------------------------*/
extern u8 code CODE2CH_VAR[];		//ch info in SEC_PHPR convert to xmode
extern u8 code CH2CODE_VAR[];

/* Private variables ---------------------------------------------------------*/
PHY_RCV_STRUCT xdata _phy_rcv_obj[CFG_PHASE_CNT];

#ifdef USE_MAC
u8 xdata _phy_channel_busy_indication[CFG_PHASE_CNT];
#endif

/*******************************************************************************
* Function Name  : init_bl6810_plc()
* Description    : Initialize BL6810 PLC Setting
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void init_bl6810_plc(void)
{
	//Initialize BL6810
	write_reg_all(ADR_WPTD,0xFF);			// make it always writable;
	write_reg_all(ADR_XMT_RCV,0x04);
	write_reg_all(ADR_XMT_CTRL,0x10);
	write_reg_all(ADR_SYNC_TRHD,0xC8);
	write_reg_all(ADR_SYNC_ENDR,0x64);
//	write_reg_all(ADR_AGC_SAGLINE,0x02);
//	write_reg_all(ADR_AGC_SAGLINE2,0x08);
//	write_reg_all(ADR_AGC_OVERLINE,0x40);
//	write_reg_all(ADR_AGC_SAGGATE,0x04);
//	write_reg_all(ADR_AGC_OVERGATE,0x04);
	write_reg_all(ADR_AGC_SAGLINE,0x02);
	write_reg_all(ADR_AGC_SAGLINE2,0x08);
	write_reg_all(ADR_AGC_OVERLINE,0x20);
	write_reg_all(ADR_AGC_SAGGATE,0x08);
	write_reg_all(ADR_AGC_OVERGATE,0x04);

#ifdef DISABLE_IFP	
	write_reg_all(ADR_ENABLE,0x00);			// FIREUPDATE DISABLE
#endif

}

/*******************************************************************************
* Function Name  : init_phy()
* Description    : 
* Input          : None
* Output         : None
* Return         : ���ݰ�����
*******************************************************************************/
void init_phy(void)
{
	u8 i,j;
	
	//Initialize TXON GPIO, EXTI, etc...
	init_phy_hardware();
	
	//Initialize PLC Registers Setting;
	init_bl6810_plc();

	//Initialize Phy Send
	init_queue();

	//Initialize Phy Rcv
	mem_clr(_phy_rcv_obj, sizeof(PHY_RCV_STRUCT), CFG_PHASE_CNT);

	for(i=0;i<CFG_PHASE_CNT;i++)
	{
		TIMER_ID_TYPE tid1,tid2;
		
		tid1 = req_timer();
		tid2 = req_timer();
		_phy_rcv_obj[i].phy_tid = req_timer();
		delete_timer(tid1);
		delete_timer(tid2);

		for(j=0;j<4;j++)
		{
			_phy_rcv_obj[i].phy_rcv_pkg_len[j] = 0;
			_phy_rcv_obj[i].phy_rcv_ebn0[j] = -128;
			_phy_rcv_obj[i].phy_rcv_ss[j] = -128;
		}
	}

#ifdef USE_MAC
	 mem_clr(_phy_channel_busy_indication,sizeof(_phy_channel_busy_indication),1);
#endif
}

/*******************************************************************************
* Function Name  : phy_rcv_int_svr()
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void phy_rcv_int_svr(PHY_RCV_HANDLE pp)
{
	u8 xdata rcv_info;
	u8 xdata rcv_msg[4];
	BASE_LEN_TYPE xdata rec_byte;
	u8 xdata rec_parity;
	u8 xdata phase=phase;
	u8 temp;
	u8 i;
	BASE_LEN_TYPE xdata * pt_plc_rcv_len;
	u8 xdata * pt_plc_rcv_buffer;
	u8 xdata * pt_plc_rcv_parity_err;

#ifdef DEBUG
#if DEVICE_TYPE == DEVICE_CC
	led_r_on();
#elif DEVICE_TYPE == DEVICE_SE
	led_phy_int_on();
#endif
#endif 
	
	phase = pp->phase;
	rcv_info = read_reg(phase,ADR_RCV_INFO);
	for(i=0;i<=3;i++)
	{
		pt_plc_rcv_len = &pp->plc_rcv_len[i];
		pt_plc_rcv_buffer = &pp->plc_rcv_buffer[i][0];
		pt_plc_rcv_parity_err = &pp->plc_rcv_parity_err[i];
	
		if((rcv_info & (0x01<<i)))
		{
			rcv_msg[i] = read_reg(phase, ADR_RCV_MSG0 + i * 4);
			switch(rcv_msg[i])
			{
				case 0x02:
					if((!pp->phy_rcv_valid) && !(pp->plc_rcv_valid & (0x10<<i))) // phyRcvValid��Ч��ʾ�ϴν�������δ������, ��ʱ�����ٽ��մ���������
					{
						*pt_plc_rcv_len = 0;
						*pt_plc_rcv_parity_err = 0;
						if(!(pp->phy_rcv_protect & (0x10<<i)))
						{
							pp->plc_rcv_we |= (0x10<<i);
						}
						write_reg(phase,ADR_SNR_CTRL,0x00);
#ifdef FIX_AGC_WHEN_RCV
						write_reg(phase,ADR_AGC_CWORD,(read_reg(phase,ADR_AGC_CWORD)|0x80));
#endif
					}
#ifdef USE_MAC
					_phy_channel_busy_indication[phase] |= (0x10<<i);
#endif
					break;
				case 0x04:
					if(pp->plc_rcv_we & (0x10<<i))
					{
						rec_byte = read_reg(phase,ADR_RCV_BYTE0 + i * 4);
						rec_parity = read_reg(phase,ADR_RCV_BYTE0 + i * 4 + 1);

						if(check_parity(rec_byte,rec_parity) == WRONG)
						{
							*pt_plc_rcv_parity_err += 1;
						}

#ifdef USE_RSCODEC
						if(((pp->plc_rcv_valid & 0x03)!=0x00) && (*pt_plc_rcv_parity_err > PARITY_ERR_TOL))	//2013-10-10 ����BPSK�о�����������, �����䲻���ǽ��ս׶εĴ����ֽ���, �ܴ����߳����ĳɹ���
#else
						if(*pt_plc_rcv_parity_err > PARITY_ERR_TOL)
#endif
						{
							temp = read_reg(phase,ADR_RCV_STA_MSK);
							write_reg(phase,ADR_RCV_STA_MSK,temp | (0x01<<i));		//reset corresponding rx ch by mask
							write_reg(phase,ADR_RCV_STA_MSK,temp & (~(0x01<<i)));
							pp->plc_rcv_valid &= ~(0x10<<i);
							pp->plc_rcv_we &= ~(0x10<<i);
							*pt_plc_rcv_len = 0;
#ifdef USE_MAC
							_phy_channel_busy_indication[phase] &= ~(0x10<<i);
#endif							
						}
						else
						{
							if(*pt_plc_rcv_len < CFG_PHY_RCV_BUFFER_SIZE)
							{
								pt_plc_rcv_buffer[*pt_plc_rcv_len] = rec_byte;
								*pt_plc_rcv_len += 1;
							}
							else
							{
								*pt_plc_rcv_len = (CFG_PHY_RCV_BUFFER_SIZE);
							}

							/* Below block is based on specific phy standard */
							if((pp->phy_rcv_pkg_len[i]>0)&&(*pt_plc_rcv_len > (pp->phy_rcv_pkg_len[i]+1))) 		//��������ֽڳ�����֡ͷָʾ��ʵ�ʳ���, 2015-03-12
							{
								*pt_plc_rcv_len -= 2;
								pp->phy_rcv_pkg_len[i] = 0;
								pp->plc_rcv_valid = (pp->plc_rcv_valid & 0xFC) + (rcv_info >> 5);
								pp->plc_rcv_valid |= (0x10<<i);
								pp->plc_rcv_we &= ~(0x10<<i);
								
								// POS PON Storage
								//write_reg(phase,ADR_WPTD,0xFF);						//2013-10-29 ���д����,������ѭ��, û����, û�д��ڽ����ֽ�̬
								write_reg(phase,ADR_SNR_CTRL,0x80);
#if DEVICE_TYPE==DEVICE_MT
								while(!(read_reg(phase,ADR_SNR_CTRL)&0x0F));		//2013-10-30 CC�Ķ�д�ٶ���,����ȴ�(һ��SPI��57.2us,һ��SNR����12.8us,��������޹�)
#endif
								write_reg(phase,ADR_SNR_CTRL,(i<<4));
								pp->plc_rcv_pos[i] = read_reg(phase,ADR_SNR_POSH);
								pp->plc_rcv_pos[i] <<= 8;
								pp->plc_rcv_pos[i] += read_reg(phase,ADR_SNR_POSL);

								pp->plc_rcv_pon[i] = read_reg(phase,ADR_SNR_PONH);
								pp->plc_rcv_pon[i] <<= 8;
								pp->plc_rcv_pon[i] += read_reg(phase,ADR_SNR_PONL);

								pp->plc_rcv_acps[i] = read_reg(phase,ADR_RCV_ACPS0 + i*4);
								pp->plc_rcv_agc_word = read_reg(phase,ADR_AGCWORD) & 0x7F;
								
								// Reset Ch
								temp = read_reg(phase,ADR_RCV_STA_MSK);
								write_reg(phase,ADR_RCV_STA_MSK,(temp | (0x01<<i)));
								write_reg(phase,ADR_RCV_STA_MSK,(temp & (~(0x01<<i))));
#ifdef USE_MAC
								_phy_channel_busy_indication[phase] &= ~(0x10<<i);
#endif
								
								/* BPSK RS����*/
#ifdef USE_RSCODEC
								if(((pp->plc_rcv_valid & 0x03)==0x00) && !(*pt_plc_rcv_buffer & PLC_FLAG_SRF))			// if bpsk & not SRF, decode
								{
									*pt_plc_rcv_len = rsdecode_vec(pt_plc_rcv_buffer, *pt_plc_rcv_len);
								}
#endif
							}
							else
							{
								/* if bpsk && use_rscodec, decode the first 20 byte to check CS, if correct get the package length */
#ifdef USE_RSCODEC
								if((*pt_plc_rcv_len==20) && (rcv_info&0x10))		
								{
									rsdecode_vec(pt_plc_rcv_buffer,20);
								}
								
								if(((*pt_plc_rcv_len==20) && (rcv_info & 0x10)) || ((*pt_plc_rcv_len==10) && !(rcv_info & 0x10)))
#else
								if(*pt_plc_rcv_len==20)
#endif
								{
									if(check_cs(pt_plc_rcv_buffer,10) == CORRECT)	//check head cs
									{
#ifdef USE_RSCODEC
										if(rcv_info & 0x10) 		
										{
											pp->phy_rcv_pkg_len[i] = rsencoded_len((((pt_plc_rcv_buffer[SEC_PHPR]|BIT_PHPR_LENH)?256:0) + pt_plc_rcv_buffer[SEC_LENL])); // SEC_LENL is uncoded length
										}
										else
#endif
										{
											pp->phy_rcv_pkg_len[i] = ((pt_plc_rcv_buffer[SEC_PHPR]|BIT_PHPR_LENH)?256:0) + pt_plc_rcv_buffer[SEC_LENL];
										}
										
									}
									/* if CS Wrong, the package is broken, reset corresponding CH */
									else													
									{

										*pt_plc_rcv_len = 0;
										pp->phy_rcv_pkg_len[i] = 0;
										pp->plc_rcv_valid &= ~(0x10<<i);
										pp->plc_rcv_we &= ~(0x10<<i);

										temp = read_reg(phase,ADR_RCV_STA_MSK);
										write_reg(phase,ADR_RCV_STA_MSK,temp | (0x01<<i));
										write_reg(phase,ADR_RCV_STA_MSK,temp & (~(0x01<<i)));
#ifdef USE_MAC
										_phy_channel_busy_indication[phase] &= ~(0x10<<i);
#endif										
									}
								}
							}
						}
					}
					break;
				case 0x08:
					if(pp->plc_rcv_we & (0x10<<i))
					{
						u16 temp16;
						pp->plc_rcv_valid = (pp->plc_rcv_valid & 0xFC) + (rcv_info >> 5);
						pp->plc_rcv_valid |= (0x10<<i);
						pp->plc_rcv_we &= ~(0x10<<i);
						*pt_plc_rcv_len -= 1;
						pp->phy_rcv_pkg_len[i] = 0;

						write_reg(phase,ADR_SNR_CTRL,(i<<4));
						temp16 = read_reg(phase,ADR_SNR_POSH);
						temp16 <<= 8;
						temp16 += read_reg(phase,ADR_SNR_POSL);
						pp->plc_rcv_pos[i] = temp16;

						temp16 = read_reg(phase,ADR_SNR_PONH);
						temp16 <<= 8;
						temp16 += read_reg(phase,ADR_SNR_PONL);
						pp->plc_rcv_pon[i] = temp16;

						pp->plc_rcv_acps[i] = read_reg(phase,ADR_RCV_ACPS0 + i*4);
						pp->plc_rcv_agc_word = read_reg(phase,ADR_AGCWORD) & 0x7F;

#ifdef FIX_AGC_WHEN_RCV
						write_reg(phase,ADR_AGC_CWORD,0x00);
#endif
						/* BPSK RS����*/
#ifdef USE_RSCODEC
						if(((pp->plc_rcv_valid & 0x03)==0x00) && !(*pt_plc_rcv_buffer & PLC_FLAG_SRF))			// if bpsk & not SRF, decode
						{
							*pt_plc_rcv_len = rsdecode_vec(pt_plc_rcv_buffer, *pt_plc_rcv_len);
						}
#endif
					}
#ifdef USE_MAC
					_phy_channel_busy_indication[phase] = 0;				//2015-04-07 ֻҪ��һ��Ƶ���յ���eop,�ͽ���csma״̬
#endif
					break;
				case 0x10:
					pp->plc_rcv_valid &= ~(0x10<<i);
					pp->plc_rcv_we &= ~(0x10<<i);
					*pt_plc_rcv_len = 0;
					*pt_plc_rcv_parity_err = 0;
#ifdef USE_MAC
					_phy_channel_busy_indication[phase] &= ~(0x10<<i);
#endif
					break;
				case 0x20:
					if(pp->plc_rcv_we & (0x10<<i))
					{
						pp->plc_rcv_valid &= ~(0x10<<i);
						pp->plc_rcv_we &= ~(0x10<<i);
						*pt_plc_rcv_len = 0;
						*pt_plc_rcv_parity_err = 0;
#ifdef FIX_AGC_WHEN_RCV
						write_reg(phase,ADR_AGC_CWORD,0x00);
#endif
					}
#ifdef USE_MAC
					_phy_channel_busy_indication[phase] &= ~(0x10<<i);
#endif					
					break;
				case 0x01:
#ifdef USE_MAC
					if(!(rcv_info&0x10))
					{
						_phy_channel_busy_indication[phase] |= (0x10<<i);	//2015-04-03 treat DSSS Sync as CSMA
					}
#endif
					break;
				default:
					break;
			}
		}
	}
#ifdef DEBUG
#if DEVICE_TYPE == DEVICE_CC
		led_r_on();
#elif DEVICE_TYPE == DEVICE_SE
		led_phy_int_on();
#endif
#endif 

#ifdef USE_MAC
	clr_csma_deadlock(phase);
#endif
}

/*******************************************************************************
* Function Name  : phy_rcv_proc
* Description    : ͨ����ʱ�ȴ�, ���г������, SCAN��������㷢�ͷ�ʽ��ɵ�ʱ��
					��һ������,���ϲ�һ��ͳһ����������״ָ̬ʾ, Ψһ�����ݳ���, 
					Ψһ������ͷ��ַ����Ϣ
* Input          : 
* Output         : ��������״ָ̬ʾphyRcvValid, һ��Ψһ�����ݳ���phyRcvLen, һ��Ψһ������ͷ��ַphyRcvData, 
* Return         : 
*******************************************************************************/
void phy_rcv_proc(PHY_RCV_HANDLE pp)
{
	u8 i;
	u8 supposed_ch = 0;
	u8 current_ch = 0;
	u8 phpr = 0;
	RESULT check_ok;
	u8 phase=phase;
	BASE_LEN_TYPE xdata * pt_plc_rcv_len;

	phase = pp->phase;
	if(!pp->phy_rcv_valid)					// if phy pkg is not been dealed by upper layer, skip
	{
		if(pp->plc_rcv_valid & 0xF0)		// check rcv data valid by CH indication
		{
			for(i=0;i<=3;i++)
			{
				pt_plc_rcv_len = &pp->plc_rcv_len[i];
				current_ch = (0x10<<i);
				
				if(pp->plc_rcv_valid & current_ch)
				{
					phpr = pp->plc_rcv_buffer[i][0];
					supposed_ch = PHPR_CH_INFO_DECODE((phpr&0x70)>>4);		// ch used indicated in the PSPR

					/* check whole package validality */
					check_ok = WRONG;
					if(phpr & PLC_FLAG_SRF)
					{
						if(*pt_plc_rcv_len==4 && (check_srf(&(pp->plc_rcv_buffer[i][0])) == CORRECT))
						{
							check_ok = CORRECT;
							pp->phy_rcv_info |= PHY_FLAG_SRF;
						}
					}					
					else
					{
						if(check_crc(&(pp->plc_rcv_buffer[i][0]), *pt_plc_rcv_len) == CORRECT)
						{
							check_ok = CORRECT;
						}
					}

					if(check_ok == CORRECT)
					{
						if(phpr & PLC_FLAG_SCAN)										// SCANֻ��ע��ע��CH, г������
						{
							if(current_ch & supposed_ch)								//�����ǰ�����ͨ·��֡ͷָʾ��ͨ·֮��
							{
								pp->phy_rcv_info |= current_ch;						// SCANģʽ��rcvinginfo����¼֡ͷ��Ϣ��ָʾ��ͨ��;
								pp->phy_rcv_info |= (pp->plc_rcv_valid & 0x03); 	// ��¼��������
								pp->phy_rcv_ss[i] = dbuv(pp->plc_rcv_pos[i],pp->plc_rcv_agc_word);
								pp->phy_rcv_ebn0[i] = ebn0(pp->plc_rcv_pos[i],pp->plc_rcv_pon[i]);
								pp->phy_rcv_supposed_ch = supposed_ch;

								if(i==3)												// ch3�����һ��ɨ���Ƶ��, �յ����������
								{
									pp->phy_rcv_valid = pp->phy_rcv_info | PLC_FLAG_SCAN;
									pp->phy_rcv_len = *pt_plc_rcv_len;
									pp->phy_rcv_data = &(pp->plc_rcv_buffer[i][0]);
									pp->phy_rcv_protect |= current_ch;
									pp->phy_rcv_info = 0;
									if(!(read_reg(phase,ADR_XMT_RCV) & 0x80))
									{
										write_reg(phase,ADR_XMT_RCV,0x80);				// �������, ��λ�շ���;
										write_reg(phase,ADR_XMT_RCV,0x00);
									}
#ifdef USE_MAC
									_phy_channel_busy_indication[phase] = 0;			// SCAN�������
#endif
									
								}
								else													// �����ܲ���CH3, ��ȴ�ɨ��������ϱ�����������Ч(���յ�CH3��ȴ���ʱ)
								{
									pp->phy_rcv_len = *pt_plc_rcv_len;
									pp->phy_rcv_data = &(pp->plc_rcv_buffer[i][0]);
									pp->phy_rcv_protect |= current_ch;					// ����scan���������ͨ��, ����ĸ�Ƶ���źſ��ܲ���г��ˢ��ǰ���Ѿ����պõ�����, ������Ա���;
									pp->phy_rcv_info |= PHY_FLAG_SCAN;

									set_timer(pp->phy_tid, scan_expiring_sticks(pp->phy_rcv_len,(pp->plc_rcv_valid & 0x03),i));
#ifdef USE_MAC
									_phy_channel_busy_indication[phase] |= (0x10<<(i+1));	// ����scan���͵������ͨ��, ������һ��ch��, ����������һ��, ����ǰ����һ��Ƶ�ʿ���
#endif
								}
								if(!(read_reg(phase,ADR_XMT_RCV) & 0x80))
								{
									write_reg(phase,ADR_XMT_RCV,0x80);
									write_reg(phase,ADR_XMT_RCV,0x00);
								}
							}
						}
						else
						{
							pp->phy_rcv_info |= current_ch;							// ��SCAN֡, ��������Ч��Ϣͨ��ָʾ������¼, rcvCh�����ķ�Χ���ܴ���phyFlag��ָʾ���ź�ͨ·
							pp->phy_rcv_info |= (pp->plc_rcv_valid & 0x03); 		// ��¼��������
							pp->phy_rcv_ss[i] = dbuv(pp->plc_rcv_pos[i],pp->plc_rcv_agc_word);
							pp->phy_rcv_ebn0[i] = ebn0(pp->plc_rcv_pos[i],pp->plc_rcv_pon[i]);
							pp->phy_rcv_len = *pt_plc_rcv_len;
							pp->phy_rcv_data = &(pp->plc_rcv_buffer[i][0]);
							pp->phy_rcv_supposed_ch = supposed_ch;

							if(!((supposed_ch ^ pp->phy_rcv_info) & supposed_ch)) 	//���֡ͷָʾ��ͨ·���Ѿ������յ�
							{
								pp->phy_rcv_valid = pp->phy_rcv_info;
								pp->phy_rcv_info = 0;
								if(!(read_reg(phase,ADR_XMT_RCV) & 0x80))
								{
									write_reg(phase,ADR_XMT_RCV,0x80);
									write_reg(phase,ADR_XMT_RCV,0x00);
								}
#ifdef USE_MAC
								_phy_channel_busy_indication[phase] = 0;			//2015-04-07 ��SCAN��֡ͷָʾ��ͨ·�����յ������
#endif
							}
							else
							{
								set_timer(pp->phy_tid,non_scan_expiring_sticks(pp->plc_rcv_valid&0x03));
							}
						}
					}
#ifdef USE_MAC
					else
					{
						if(!pp->phy_rcv_info)
						{
							_phy_channel_busy_indication[phase] = 0;					//2015-04-07 ��scan�����ݰ�, ���ݰ����������csma
						}
					}
#endif
					pp->plc_rcv_valid &= ~(current_ch);								// �����꼴�ر��жϴ�������ͨ·ָʾ
				}
			}
		}
		else											// �������������ָʾ, �����Ƿ���δ�������������չ�����Ҫ����
		{
			if(pp->phy_rcv_info)	
			{
				if(!is_timer_idle(pp->phy_tid))
				{
					if(is_timer_over(pp->phy_tid))		//�ô�����̶�����ͨ����֡��SCAN����֡��ͳһ��, ����ֻ����phyRcvTimingOut��ͬ
					{
						pp->phy_rcv_valid = pp->phy_rcv_info;
						pp->phy_rcv_info = 0;
						if(!((read_reg(phase,ADR_XMT_RCV) & 0x80)))
						{
							write_reg(phase,ADR_XMT_RCV,0x80);
							write_reg(phase,ADR_XMT_RCV,0x00);
						}
						pp->plc_rcv_we = 0;
						reset_timer(pp->phy_tid);
#ifdef USE_MAC
						_phy_channel_busy_indication[phase] = 0;				//2015-04-07 ���ն�ʱ������csma
#endif
					}
				}
				else
				{
#ifdef DEBUG_DISP_INFO
					my_printf("phy tid invalid\r\n");
#endif
				}
			}
		}
	}
}

/*******************************************************************************
* Function Name  : phy_rcv
* Description    : 
* Input          : 
* Output         : pointer to plc rcv object
* Return         : length of rec bytes
*******************************************************************************/
//PHY_RCV_HANDLE phy_rcv()
//{
//	u8 i;
//	
//	for(i=0;i<CFG_PHASE_CNT;i++)
//	{
//		if(_phy_rcv_obj[i].phy_rcv_valid & 0xF0)
//		{
//			return &_phy_rcv_obj[i];
//		}
//	}
//	return NULL;
//}


/*******************************************************************************
* Function Name  : phy_rcv_resume
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void phy_rcv_resume(PHY_RCV_HANDLE pp) reentrant
{
	u8 i;
	//TIMER_ID_TYPE tid;
	if(check_struct_array_handle((u8*)pp,(u8*)_phy_rcv_obj,sizeof(PHY_RCV_STRUCT),CFG_PHASE_CNT))
	{
		//tid = pp->phy_tid;
		//debug_led_flash();
		//mem_clr(pp,sizeof(PHY_RCV_STRUCT),1);
		//debug_led_flash();
		//pp->phy_tid = tid;
		//for(i=0;i<4;i++)
		//{
		//	pp->phy_rcv_pkg_len[i] = 0xFF;
		//	pp->phy_rcv_ebn0[i] = -128;
		//	pp->phy_rcv_ss[i] = -128;
		//}

		pp->plc_rcv_we = 0;
		pp->phy_rcv_len = 0;
		pp->phy_rcv_data = NULL;

		pp->phy_rcv_protect = 0;
		pp->phy_rcv_info = 0;

		for(i=0;i<=3;i++)
		{
			pp->plc_rcv_len[i] = 0;
			pp->plc_rcv_parity_err[i] = 0;
			pp->plc_rcv_pos[i] = 0x00;
			pp->plc_rcv_pon[i] = 0x00;
			
			pp->phy_rcv_ss[i] = 0x80;
			pp->phy_rcv_ebn0[i] = 0x80;
			pp->phy_rcv_pkg_len[i] = 0;
		}
		pp->phy_rcv_valid = 0;
		pp->plc_rcv_valid = 0;
	}
	else
	{
#ifdef DEBUG_DISP_INFO
		my_printf("INVALID PHY HANDLE\r\n");
#endif
	}
}

/*******************************************************************************
* Function Name  : phy_send
* Description    : ����queueϸ�ڵ�phy_send����,����queue,����������queue��,
					��ȷ,����sid,�κδ���,����-1
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
SEND_ID_TYPE phy_send(PHY_SEND_HANDLE pps) reentrant
{
	SEND_ID_TYPE sid;
	
	sid = req_send_queue();
	if(sid != INVALID_RESOURCE_ID)
	{
		if(!push_send_queue(sid,pps))
		{
			sid = INVALID_RESOURCE_ID;
		}
	}
	return sid;
}

#ifdef USE_DIAG
/*******************************************************************************
* Function Name  : wait_until_send_over
* Description    : �����ȴ����ͽ���,���淢��״̬
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS wait_until_send_over(SEND_ID_TYPE sid)
{
	SEND_STATUS ss;
	while(1)							//�������Ѿ���queue����
	{
#if NODE_TYPE==NODE_MASTER
		SET_BREAK_THROUGH("quit wait_until_send_over\r\n");
		powermesh_main_thread_useless_resp_clear(); 	//���Э�����
		powermesh_clear_apdu_rcv(); 					//���Ӧ�ò����
#endif

		ss = inquire_send_status(sid);
		switch(ss)
		{
			case(SEND_STATUS_OKAY):
				return OKAY;
			case(SEND_STATUS_IDLE):
			case(SEND_STATUS_RESERVED):
			case(SEND_STATUS_FAIL):
				return FAIL;
			default:
				FEED_DOG();
		}
	}
	return OKAY;
}

/*******************************************************************************
* Function Name  : check_and_clear_send_status
* Description    : �����ж��з�������Ӧ��,ȱ����Ҫ���ֶν��Ѿ������queue���,����queue�������ĿԽ��Խ��,�����ⲻӰ��ʹ��
					���Ͼ����ÿ�,�������ѭ��������һ����ѯ��ȡ״̬,Ŀ���ǽ�SUCCESS��FAIL��־�����.
					��Щ��Ҫ���ķ��ͽ���Ľ���ʹ��wait_until_send_over�Ѿ�������ȡ����
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void check_and_clear_send_status(void)
{
	u8 i;
	for(i=0;i<CFG_QUEUE_CNT;i++)
	{
		inquire_send_status((SEND_ID_TYPE)(i));
	}
}
#endif

