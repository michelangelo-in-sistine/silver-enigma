/******************** (C) COPYRIGHT 2012 Belling Inc. ********************
* File Name          : hardware.c
* Author             : Lv Haifeng
* Version            : V1.0.3
* Date               : 10/26/2012
* Description        : Hardware-specific actions
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
#define CNT_TH				(65536-912) / 4
#define	CNT_BAUD_38400		0xF0						// dec2hex(256 - floor(10e6*12/baud/192))
#define	CNT_BAUD_9600		0xBF
#define CNT_BAUD_4800		0x7E

/* Referenced Extern Variables -----------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/
#ifdef AMP_CONTROL
#define IS_TX_ON()			!P37
#endif

#if MEASURE_DEVICE == BL6523GX
	#define MEASURE_RST_PIN		P32
#elif MEASURE_DEVICE == BL6523B
	#define SCK					P32
	#define SCS					P33
	#define SDI					P34
	#define	SDO					P35
	#define MEASURE_RST_PIN		P36
#endif

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
void init_basic_hardware()
{
	WDTCON = 0xCF;
	FEED_DOG();
#ifdef DISABLE_IFP
	IEN2 = 0;
#endif
}

/*******************************************************************************
* Function Name  : reset_system_behavior()
* Description    : 
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
void system_reset_behavior()
{
	u16 i,j;
	
	P0 = 0xFF;
	P1 = 0xFF;
	P2 = 0xFF;
	P3 = 0xFF;
	
	for(i = 0; i<8; i++)
	{
		LED_1 = !LED_1;
		LED_2 = !LED_2;
		for(j = 0; j<32767; j++)
		{
			FEED_DOG();
		}
	}
}

/*******************************************************************************
* Function Name  : init_timer_hardware()
* Description    : 
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
void init_timer_hardware(void)
{
	TMOD2 = 0x02;
	TH2 = CNT_TH;
	ET2 = 1;													// 2013-01-10 Use Timer2
	TR2 = 1;
}


/*******************************************************************************
* Function Name  : timer2_int_svr()
* Description    : Timer Serve Routine of Timer 2. 
					Period of Timer 0 (minimum time stick) is (10e6/12/912)^-1 
					= 1.0944ms, 6 basic BPSK bit timing
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
void timer_hardware_int_svr(void) interrupt 6
{
	static u8 int_cnt = 0;
	u8 ext_adr_backup;

	int_cnt++;
	if(int_cnt>=0x04)
	{
		int_cnt = 0;
		ext_adr_backup = EXT_ADR;
		timer_int_svr();
	}
#ifdef AMP_CONTROL
	{
		static u8 xdata last_indication = 1;

		if(IS_TX_ON())
		{
			if(last_indication && !PIN_LOW_POWER)
			{
				ext_adr_backup = EXT_ADR;
				EXT_ADR = ADR_XMT_AMP;
				if(EXT_DAT > CFG_XMT_AMP_MIN_VALUE)				//2014-05-13 防止发送幅度一直降到0
				{
					EXT_DAT >>= 1;
				}
				EXT_ADR = ext_adr_backup;
			}
			last_indication = PIN_LOW_POWER;
		}
	}
#endif
}

/*******************************************************************************
* Function Name  : init_phy_hardware
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void init_phy_hardware(void)
{
	// INT Edge Trigger
	IT0 = 1;
	IT1 = 1;
	EX1 = 1;
}

/*******************************************************************************
* Function Name  : init_uart_hardware()
* Description    : bps:38400, no parity bit
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
#ifdef USE_UART
void init_debug_uart_hardware(void)
{
	TMOD &= 0x0F;    				//Timer1 工作在方式2
	TMOD |= 0x20;

	SCON = 0x50;
//	SCON = 0xD0;					// EVEN PARITY
	PCON = 0x88;					//SMOD = 1, X12 = 1
	TH1 = CNT_BAUD_38400;	
	TR1 = 1;
	TI = 0;
	ES = 1;
}
#endif

/*******************************************************************************
* Function Name  : uart_send8()
* Description    : bps:38400, no parity bit
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
#ifdef USE_UART
void uart_send8(u8 byte_data)
{
	ENTER_CRITICAL();
	ES = 0;
	TI = 0;
	SBUF = byte_data;
	while(!TI);
	TI = 0; 				// keep TI=0, avoid uart interrupt.
	ES = 1;
	FEED_DOG();
	EXIT_CRITICAL();
}
#endif

/*******************************************************************************
* Function Name  : uart_hardware_int_svr()
* Description    : 
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
void uart_hardware_int_svr(void) interrupt 4
{
	if(RI)
	{
#ifdef USE_UART
		debug_uart_rcv_int_svr(SBUF);
#endif
		RI = 0;
	}
	if(TI)
	{
		TI = 0;
	}
}


/*******************************************************************************
* Function Name  : phy_rcv_hardware_int_svr()
* Description    : 
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
void phy_rcv_hardware_int_svr(void) interrupt 2
{
	u8 ext_adr_backup;

	ext_adr_backup = EXT_ADR;
	phy_rcv_int_svr(&_phy_rcv_obj[0]);
	EXT_ADR = ext_adr_backup;
}

/*******************************************************************************
* Function Name  : read_reg_entity()
* Description    : 
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
u8 read_reg_entity(u8 addr)
{
	EXT_ADR = addr;
	return EXT_DAT;
}

/*******************************************************************************
* Function Name  : check_parity
* Description    : parity = [X X X X X P 0 1], and {byte P} should have an even number of "1" totally
* Input          : 
* Output         : 
* Return         : cs;
*******************************************************************************/
RESULT check_parity(u8 byte, u8 parity)
{
	ACC = byte;
	if(P)
	{
		if(parity == 0x05)
		{
			return CORRECT;			// pass return 0;
		}
	}
	else
	{
		if(parity == 0x01)
		{
			return CORRECT;			// pass return 0;
		}
	}
	return WRONG;
}

/*******************************************************************************
* Function Name  : phy_trans_sticks
* Description    : 计算数据包在物理层信道传输时间
* Input          : len为数据包长(编码前), rate: 传输速率; scan: scan模式
* Output         : 
* Return         : 
*******************************************************************************/
u32 phy_trans_sticks(BASE_LEN_TYPE ppdu_len, u8 rate, u8 scan) reentrant
{
	u32 timing;

	timing = packet_chips(ppdu_len,rate,0);
	
	if(scan)
	{
		//timing = timing * 4 / 6 + XMT_SCAN_INTERVAL_STICKS * 4;
		timing = (timing << 1) / 3;					// timing * 4 / 6;
		timing += (XMT_SCAN_INTERVAL_STICKS << 2);	// timing += XMT_SCAN_INTERVAL_STICKS * 4;
	}
	else
	{
		timing /= 6;
	}
	return timing;
}

u32 srf_trans_sticks(u8 rate, u8 scan) reentrant
{
	u32 timing;
	
	timing = packet_chips(4,rate,1);
	
	if(scan)
	{
		//timing = timing * 4 / 6 + XMT_SCAN_INTERVAL_STICKS * 4;
		timing = (timing << 1) / 3;					// timing * 4 / 6;
		timing += (XMT_SCAN_INTERVAL_STICKS << 2);	// timing += XMT_SCAN_INTERVAL_STICKS * 4;
	}
	else
	{
		timing /= 6;
	}
	return timing;
}

/*******************************************************************************
* Function Name  : get_uid
* Description    : 6810: NVR ADR 0x0300~0x0305
* Input          : 相位, 首地址
* Output         : 
* Return         : 
*******************************************************************************/
void get_uid_entity(u8 xdata * pt)
{
	unsigned char i;

	ENTER_CRITICAL();
	for(i=0;i<6;i++)				// fill self uid
	{
		IFP_ADR = ADR_NVR_ADDRL;
		IFP_DAT = i;
		IFP_ADR = ADR_NVR_ADDRH;
		IFP_DAT = 0x03;
		IFP_ADR = ADR_NVR_CTRL;
		IFP_DAT = 0x10; 			//read
		IFP_ADR = ADR_NVR_RD;
		*pt++ = IFP_DAT;
	}
	EXIT_CRITICAL();
}

/*******************************************************************************
* Function Name  : reset_measure_device
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void reset_measure_device(void)
{
	u16 i;
	
	MEASURE_RST_PIN = 0;
	for(i=0;i<50000UL;i++)
	{
		FEED_DOG();
	}
	MEASURE_RST_PIN = 1;
	for(i=0;i<50000UL;i++)									//6523 need time to finish reset
	{
		FEED_DOG();
	}
}

#if MEASURE_DEVICE == BL6523GX
/*******************************************************************************
* Function Name  : init_measure_hardware
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void init_measure_hardware(void)
{
#if MEASURE_DEVICE == BL6523GX
	TMOD &= 0x0F;    				//Timer1 工作在方式2
	TMOD |= 0x20;

	SCON = 0x50;
//	SCON = 0xD0;					// EVEN PARITY
	PCON = 0x88;					//SMOD = 1, X12 = 1
	TH1 = CNT_BAUD_4800;	
	TR1 = 1;
	TI = 0;
//	ES = 1;
#endif

}


/*******************************************************************************
* Function Name  : measure_com_send
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void measure_com_send8(u8 byte_data)
{
	ENTER_CRITICAL();
//	ES = 0;
	TI = 0;
	SBUF = byte_data;
	while(!TI);
	TI = 0; 				// keep TI=0, avoid uart interrupt.
//	ES = 1;
	FEED_DOG();
	EXIT_CRITICAL();

}

/*******************************************************************************
* Function Name  : measure_com_read8
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u8 measure_com_read8(u8 * pt_rec_byte)
{
	u16 timer = 0;

	for(timer=0;timer<9999UL;timer++)
	{
		if(RI)
		{
			*pt_rec_byte = SBUF;
			RI = 0;
			return 1;
		}
		FEED_DOG();
	}
	return 0;
}


/*******************************************************************************
* Function Name  : measure_com_send
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void measure_com_send(u8 * head, u8 len)
{
	u8 i;
	for(i=0;i<len;i++)
	{
		measure_com_send8(*head++);
	}
}

/*******************************************************************************
* Function Name  : write_bl6523
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void write_measure_reg(u8 addr, u32 dword_value)
{
	u8 cs=0;
	u8 buffer[6];
	u8 i;
	
	buffer[0] = 0xCA;
	buffer[1] = addr;
	cs = addr;

	for(i=2;i<5;i++)
	{
		buffer[i] = (u8)dword_value;
		cs += buffer[i];
		dword_value>>=8;
	}
	buffer[5] = ~cs;

	measure_com_send(buffer,sizeof(buffer));
}

/*******************************************************************************
* Function Name  : read_bl6523
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u32 read_measure_reg(u8 addr)
{
	u8 rec_byte;
	u8 i;
	u8 xdata buffer[4];
	u32 value = 0;
	
	measure_com_send8(0x35);
	measure_com_send8(addr);

	for(i=0;i<4;i++)
	{
		if(measure_com_read8(&rec_byte))
		{
			buffer[i] = rec_byte;
		}
		else
		{
#ifdef DEBUG_MODE
			my_printf("bl6532 return fail\n");
#endif
			return 0;
		}
		
	}

	for(i=0;i<4;i++)
	{
		addr += buffer[i];
	}

	if(addr!=255)
	{
#ifdef DEBUG_MODE
		my_printf("bl6532 return bytes checked fail\n");
		uart_send_asc(buffer,4);
#endif
		return 0;
	}
	else
	{
		for(i=2;i!=0xFF;i--)
		{
			value <<= 8;
			value += buffer[i];
		}
		return value;
	}
}

#elif MEASURE_DEVICE == BL6523B


void spi_delay()
{
	u8 i;

	for(i=0;i<200;i++);
}

void spi_write_byte(u8 byte)
{
	u8 i;

	for(i=0;i<8;i++)
	{
		SDI = (byte&0x80)?1:0;
		byte <<= 1;
		SCK = 0;
		spi_delay();
		SCK = 1;
		spi_delay();
	}
	SCK = 0;
}

u8 spi_read_byte(void)
{
	u8 i;
	u8 byte = 0;
	
	for(i=0;i<8;i++)
	{
		SCK = 0;
		spi_delay();
		SCK = 1;
		spi_delay();
		byte <<= 1;
		byte |= SDO?1:0;
	}
	SCK = 0;
	return byte;
}

void write_measure_reg(u8 addr, u32 value)
{
	addr |= 0x40;
	SCS = 0;
	spi_write_byte(addr);
	spi_write_byte(value>>16);
	spi_write_byte(value>>8);
	spi_write_byte(value);
	SCS = 1;
}


u32 read_measure_reg(u8 addr)
{
	u32 value;
	
	addr &= 0x3F;					//BL6523B读操作最高两bit为'00'
	SCS = 0;
	spi_write_byte(addr);
	value = spi_read_byte();
	value <<= 8;
	value += spi_read_byte();
	value <<= 8;
	value += spi_read_byte();
	SCS = 1;

	return value;
}



#endif


/*******************************************************************************
* Function Name  : init_measure_hardware
* Description    : 避免进入中断
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void hardware_int_svr_0(void) interrupt 0
{
	
}

void hardware_int_svr_1(void) interrupt 1
{
}

void hardware_int_svr_3(void) interrupt 3
{
}




void hardware_int_svr_5(void) interrupt 5
{
}

//void hardware_int_svr_6(void) interrupt 6		//timer 2 int
//{
//}

void hardware_int_svr_7(void) interrupt 7
{
}






/******************* (C) COPYRIGHT 2012 Belling *****END OF FILE****/
