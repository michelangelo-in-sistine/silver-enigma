/******************** (C) COPYRIGHT 2016 ***************************************
* File Name          : general.c
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
#include "stdlib.h"
#include "ver_info.h"
#include "stdio.h"

//#if DEVICE_TYPE == DEVICE_CC
//#include "hal_sflash.h"

//#ifdef TEST_TRANSACTION
//#include "_app_transaction.h"
//#endif
//#endif

/* Extern Reference ------------------------------------------------------------*/
extern MGNT_RCV_STRUCT xdata _mgnt_rcv_obj[];

/* Private define ------------------------------------------------------------*/
#define UV_PER_STEP		195.3			//BL6810V12 ADC parameter: 195.3uV per step(800mv divided into 4096 steps)
#define DB_PER_STEP		2.26

/* Private typedef -----------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
u8 code CODE2MODE_VAR[] = {0x10,0x20,0x40,0x80,0x11,0x21,0x41,0x81,0x12,0x22,0x42,0x82,0xf0,0xf1,0xf2,0xf2}; // 4BITS CODE TO ACTRUAL XMODE
u8 xdata _meter_id[7];
#if BRING_USER_DATA == 1
u8 xdata _user_data_len = 0;						//2015-06-04 如mt不设置_user_data_len,会在组网时造成溢出出错
u8 xdata _user_data[CFG_USER_DATA_BUFFER_SIZE];
#endif

#ifdef DEBUG_MODE
#if DEVICE_TYPE==DEVICE_CC
u8 code start_code[] = "FIRMWARE: POWERMESH_CC";
#elif DEVICE_TYPE==DEVICE_MT
u8 code start_code[] = "FIRMWARE: POWERMESH_MT";
#elif DEVICE_TYPE==DEVICE_DC
u8 code start_code[] = "FIRMWARE: POWERMESH_DC";
#elif DEVICE_TYPE==DEVICE_CV
u8 code start_code[] = "FIRMWARE: POWERMESH_CV";
#elif DEVICE_TYPE==DEVICE_SE
u8 code start_code[] = "FIRMWARE: POWERMESH_SE";
#elif DEVICE_TYPE==DEVICE_SS
u8 code start_code[] = "FIRMWARE: POWERMESH_SS";
#endif
u8 code compile_date[] =  __DATE__;
u8 code compile_time[] =  __TIME__;
#endif

#if NODE_TYPE==NODE_MASTER
u8 xdata quit_loops = 0;
#endif

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : init_powermesh()
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void init_powermesh(void)
{
	u8 xdata self_uid[6];

	/**************************
	STM32 System Initialization
	**************************/
	ENTER_CRITICAL();						// absolutely disable all interrupts

	init_basic_hardware();
	system_reset_behavior(); 

#ifdef USE_ADDR_DB
	init_addr_db();
#endif
	init_mem();
	init_timer();
#ifdef USE_UART
	init_uart();
#endif

#if PLC_CONTROL_MODE==DEVICE_MODE
	if(check_spi()!=CORRECT)					// 2014-08-07 防止打ESD时系统复位正好此时SPI不工作导致死机
	{
#ifdef DEBUG_MODE
		my_printf("SPI CHECK FAILED.\r\n");
#endif
	}
#endif

//#if DEVICE_TYPE==DEVICE_CC
//#ifndef RELEASE	
//	if(check_external_sram(0x80000))
//	{
//#ifdef USE_UART
//		my_printf("EXTERNAL SRAM CHECK PASS.\r\n");
//#endif
//	}
//	else
//	{
//#ifdef USE_UART
//		my_printf("EXTERNAL SRAM CHECK FAIL.\r\n");
//#endif
//	}
//#endif
//	hal_sFLASH_Init();
//	Flash_Init();

//#endif

	init_phy();
	init_dll();

#ifdef USE_PSR
	init_psr();
#endif
#ifdef USE_DST
	init_dst();
#endif
#ifdef USE_PTP
	init_ptp();
#endif
	init_mgnt_app();

#if NODE_TYPE==NODE_MASTER
#ifdef TEST_TRANSACTION
	init_app_transaction();
#endif
	init_psr_mnpl();
	init_network_management();
#endif

	EXIT_CRITICAL();							// absolutely enable all interrupts

	get_uid(2,self_uid);
	{
		u16 temp;
		
		temp = self_uid[4];
		temp ^= read_reg(2,0x66);				//利用mfos的最低位加随机扰	
		temp <<= 8;
		temp += self_uid[5];
		temp ^= read_reg(2,ADR_ACPS);
		srand(temp);							//利用uid和相位确保每次上电的随机化种子不同
	}
#ifdef DEBUG_MODE
	{
		TIMING_VERSION_TYPE ver;
		my_printf("/*-------------------\r\n");
		my_printf("%s\r\nUID: ",start_code);
		uart_send_asc(self_uid,6);
#ifdef USE_RSCODEC
		my_printf("\r\nRS_DECODE_LEN:%bu, RS_ENCODE_LEN:%bu\r\n",CFG_RS_DECODE_LENGTH,CFG_RS_ENCODE_LENGTH);
#endif
		get_timing_version(&ver);
		my_printf("\r\nLAST COMPILED AT %s, %s\r\n",compile_date,compile_time);

		#ifndef RELEASE
		my_printf("DISTURB CODE:%bx\r\n",CRC_DISTURB);
		#endif
#ifdef USE_DMA
		my_printf("DMA MODE\r\n");
#endif
	}
#endif
}

/*******************************************************************************
* Function Name  : get_operation_phase
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void powermesh_event_proc()
{
	u8 i;
	
	FEED_DOG();
	EXIT_CRITICAL();			// 2013-04-08 avoid 死机

	for(i=0;i<CFG_PHASE_CNT;i++)
	{
#ifdef USE_PSR
		/* 管理层命令处理, 因为涉及到diag, psr, broadcast等事务, 不能在中断里做, 放在主线程里完成, 由主线程调用 */
		mgnt_proc(&_mgnt_rcv_obj[i]);
#endif
	}

	/* 垃圾处理, 消除超时收到的响应包, 这些控制包或者应该在主线程的transaction, 或者应该在中断的处理程序中处理完毕, 
		在主线程中出现则无法消除掉, 因此单独设置一个处理将其清掉*/
	powermesh_main_thread_useless_resp_clear();
	
#if DEBUG_UART_PROC == 1
	debug_uart_cmd_proc();
#endif
	
	//	/* 整理flash碎片*/
//#if DEVICE_TYPE==DEVICE_CC
//	{
//		static u16 fragment_timer = 0;
//		
//		if(fragment_timer++ == 0)
//		{
//			FragmentTick();
//		}
//	}
//#endif

//#if DEVICE_TYPE==DEVICE_CC

//	/* app transaction 处理*/
//#ifdef TEST_TRANSACTION
//	FEED_DOG();
//	app_transaction_tick();
//#endif

//	/* 取消全局退出循环标志 */
//	clr_quit_loops();
//#endif

}

/*******************************************************************************
* Function Name  : powermesh_main_thread_useless_resp_clear
* Description    : 清理接收栈中可能存在的阻碍接收的响应包
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void powermesh_main_thread_useless_resp_clear(void)
{
	DLL_RCV_HANDLE pd;
#ifdef USE_PSR
	PSR_RCV_HANDLE pn;
	MGNT_RCV_HANDLE pm;

	pm = mgnt_rcv();
	if(pm)
	{
#ifdef DEBUG_DISP_INFO
		my_printf("discard an mgnt resp\r\n");
#endif
		mgnt_rcv_resume(pm);
	}

	pn = psr_rcv();
	if(pn)
	{
#ifdef DEBUG_DISP_INFO
		my_printf("discard an nw resp\r\n");
#endif
		psr_rcv_resume(pn);				
	}
#endif

	pd = dll_rcv();
	if(pd)
	{
		if(pd->dll_rcv_valid & BIT_DLL_SRF)
		{
//#ifdef DEBUG_DISP_INFO
//			my_printf("discard an srf.\r\n");
//#endif
		}
		else
		{
#ifdef DEBUG_DISP_INFO
			my_printf("discard an dll resp\r\n");
#endif
		}
		dll_rcv_resume(pd);
	}
}



/*******************************************************************************
* Function Name  : lg
* Description    : log10的快速算法
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
#define C10 3.321928		// log(2)(10)
float lg(unsigned int x)
{	
	float xdata y = x;	
	unsigned char xdata pwr = 0;

	while(x > 1)
	{				
		x = (x >> 1);
		pwr++;		
		y = y/2;
	}	
	
	y -= 1;
		
	if(y < 0.25)
	{
		y = 1.3125 * y;
	}
	else if(y < 0.625)
	{
		y = y + 0.078125;
	}
	else
	{
		y = 0.78125 * y + 0.21875;
	}
	
	return (pwr+y)/C10;
}




/*******************************************************************************
* Function Name  : mem_cpy
* Description    : memory copy
* Input          : dest address, src address, copy length
* Output         : 
* Return         : 
*******************************************************************************/
void mem_cpy(ARRAY_HANDLE pt_dest, ARRAY_HANDLE pt_src, BASE_LEN_TYPE len) reentrant
{
	BASE_LEN_TYPE i;

#if CPU_TYPE != CPU_BL6810
	if(pt_dest && pt_src)
#endif
	{
		for(i=0; i<len; i++)
		{
			*pt_dest++ = *pt_src++;
		}
	}
}

/*******************************************************************************
* Function Name  : mem_cmp
* Description    : memory compare
* Input          : dest address, src address, compare length
* Output         : 
* Return         : TRUE/FALSE
*******************************************************************************/
BOOL mem_cmp(ARRAY_HANDLE pt1, ARRAY_HANDLE pt2, BASE_LEN_TYPE len) reentrant
{
	BASE_LEN_TYPE i;

	for(i=0; i<len; i++)
	{
		if((*pt1++) != (*pt2++))
		{
			return FALSE;
		}
	}
	return TRUE;
}

#if NODE_TYPE == NODE_MASTER
/*******************************************************************************
* Function Name  : mem_cmp_reverse
* Description    : memory compare, compare from right to left, shorten address comparasion time
* Input          : dest address, src address, compare length
* Output         : 
* Return         : TRUE/FALSE
*******************************************************************************/
BOOL mem_cmp_reverse(ARRAY_HANDLE pt1, ARRAY_HANDLE pt2, BASE_LEN_TYPE len) reentrant
{
	BASE_LEN_TYPE i;

	pt1 += (len-1);
	pt2 += (len-1);

	for(i=0; i<len; i++)
	{
		if((*pt1--) != (*pt2--))
		{
			return FALSE;
		}
	}
	return TRUE;
}
#endif

/*******************************************************************************
* Function Name  : mem_clr
* Description    : memory block clear
* Input          : head address, mem block size, mem block quantity
* Output         : 
* Return         : TRUE/FALSE
*******************************************************************************/
void mem_clr(void xdata * mem_block_addr, u16 mem_block_size, u16 mem_block_cnt) reentrant
{
	u16 i;
	u16 j;
	ARRAY_HANDLE pt = mem_block_addr;

#if CPU_TYPE != CPU_BL6810
	if(mem_block_addr)					//2015-03-11 6810 xdata指针可以为0
#endif
	{
		for(i=0;i<mem_block_cnt;i++)
		{
			for(j=0;j<mem_block_size;j++)
			{
				*(pt++) = 0;
			}
		}
	}
}

/*******************************************************************************
* Function Name  : mem_shift
* Description    : shift mem n bytes right
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void mem_shift(ARRAY_HANDLE mem_addr, BASE_LEN_TYPE mem_len, BASE_LEN_TYPE shift_bytes) reentrant
{
	ARRAY_HANDLE ptw;
	BASE_LEN_TYPE i;

#if CPU_TYPE != CPU_BL6810
	if(mem_addr)
#endif
	{
		mem_addr = mem_addr + mem_len - 1;	//make mem point to the last byte
		ptw = mem_addr + shift_bytes;

		for(i=0;i<mem_len;i++)
		{
			*ptw-- = *mem_addr--;
		}
	}
}

/*******************************************************************************
* Function Name  : check_struct_array_handle
* Description    : 检查struct_array类型的资源句柄有效性
* Input          : head address, mem block size, mem block quantity
* Output         : 
* Return         : TRUE/FALSE
*******************************************************************************/
RESULT check_struct_array_handle(ARRAY_HANDLE handle, ARRAY_HANDLE struct_array_addr, u16 struct_size, u8 struct_cnt) reentrant
{
	u8 i;
	
	for(i=0;i<struct_cnt;i++)
	{
		if(handle==struct_array_addr)
		{
			return CORRECT;
		}
		else
		{
			struct_array_addr += struct_size;
		}
	}
	return WRONG;
}

#ifdef USE_DIAG
/*******************************************************************************
* Function Name  : check_ack_src
* Description    : check ack src 
* Input          : 检查返回的ack地址是否正确
* Output         : 
* Return         : CORRECT/ERROR
*******************************************************************************/
RESULT check_ack_src(ARRAY_HANDLE pt_ack_src, ARRAY_HANDLE pt_dest)
{
	unsigned i;
	ARRAY_HANDLE pt;

	pt = pt_dest;
	for(i=0; i<6; i++)
	{
		if(*pt != 0x00)
		{
			break;
		}
		if(i==5)
		{
			return CORRECT;				//broadcast ack
		}
	}

	pt = pt_dest;
	for(i=0; i<6; i++)
	{
		if(*pt != 0xFF)
		{
			break;
		}
		if(i==5)
		{
			return CORRECT;				//unknown ack
		}
	}
	
	return (RESULT)mem_cmp(pt_ack_src,pt_dest,6);
}

#endif



#if CPU_TYPE==CPU_BL6810


#else
///*******************************************************************************
//* Function Name  : calc_crc
//* Description    : Calculate a u8 vector into crc-16 checksum
//* Input          : Head address, length
//* Output         : u16 crc value
//* Return         : u16 crc value
//*******************************************************************************/
//u16 calc_crc(ARRAY_HANDLE pt, u8 len) //reentrant
//{
//	u16 crc;
//	u16 i;
//	u8 j;
//	u8 newbyte,crcbit,databit;
//
//	crc = 0xffff;
//	for(i=0;i<len;i++)
//	{
//		newbyte = *(pt++);
//		for (j = 0; j < 8; j++)
//		{
//			crcbit = (crc & 0x8000) ? 1 : 0;
//			databit = (newbyte & 0x80) ? 1 : 0;
//			crc = crc << 1;
//			if (crcbit != databit)
//			{
//				crc = crc ^ POLY;
//			}
//			newbyte = newbyte << 1;
//		}
//	}
//	crc = crc^0xffff;
//
//#ifdef CRC_ISOLATED
//	crc ^= CRC_DISTURB;
//#endif
//	return crc;
//}

#endif

/*******************************************************************************
* Function Name  : check_crc
* Description    : 
* Input          : 
* Output         : 
* Return         : CORRECT/WRONG;
*******************************************************************************/
RESULT check_crc(ARRAY_HANDLE pt, BASE_LEN_TYPE len)
{
	unsigned int crc;

	if(len<2 || len>CFG_MEM_SUPERIOR_BLOCK_SIZE)
	{
		return WRONG;
	}

#ifdef CRC_ISOLATED
		pt[len-1] ^= CRC_DISTURB;
#endif
	
	crc = calc_crc(pt, len);
#ifdef CRC_ISOLATED
	crc ^= CRC_DISTURB;
#endif

	if(crc==0xE2F0)
	{
		return CORRECT;
	}
	else
	{
		return WRONG;
	}
}

/*******************************************************************************
* Function Name  : calc_cs
* Description    : cs = 256 - sum(x1 + x2 + ... xn);
* Input          : 
* Output         : 
* Return         : cs;
*******************************************************************************/
u8 calc_cs(ARRAY_HANDLE pt, BASE_LEN_TYPE len) reentrant
{
	BASE_LEN_TYPE i;
	unsigned char cs = 0;
	
	for(i=0;i<len;i++)
	{
		cs += *(pt++);
	}
	cs = 0-cs;

#ifdef CRC_ISOLATED
	cs += CRC_DISTURB;
#endif
	
	return cs;
}

/*******************************************************************************
* Function Name  : check_cs
* Description    : 
* Input          : 
* Output         : 
* Return         : CORRECT/WRONG;
*******************************************************************************/
RESULT check_cs(ARRAY_HANDLE pt, BASE_LEN_TYPE len)
{
	unsigned char cs;

	cs = calc_cs(pt, len);

	if(cs==0)
	{
		return CORRECT;
	}
	else
	{
		return WRONG;
	}
}


/*******************************************************************************
* Function Name  : check_srf
* Description    : 
* Input          : 
* Output         : 
* Return         : CORRECT/WRONG;
*******************************************************************************/
RESULT check_srf(unsigned char xdata * pt)
{
	return (calc_cs(pt,4) == 0) ? CORRECT : WRONG;
}

/*******************************************************************************
* Function Name  : dbuv
* Description    :  算法: 20*log10(pos*UV_PER_STEP/16/(db2mag(DB_PER_STEP*agc)))
					化简得: 20*log10(pos) + 20*log10(195.3/16) - (2.26*agc_value)
					    	=  20*log10(pos) + 21.7316 - (2.26*agc_value) 
* Input          : 
* Output         : 
* Return         : float p2p amplitude in uV;
*******************************************************************************/
s8 dbuv(u16 pos, u8 agc_value)
{
	float xdata x;
	
	x = lg(pos)*20 + 21.7316 - (DB_PER_STEP*agc_value);
	if (x > 127)
	{
		x = 127;
	}
	return (s8)(x);
}

/*******************************************************************************
* Function Name  : ebn0
* Description    : 
* Input          : 
* Output         : 
* Return         : calculate the signal to N0 ratio after receiving complement;
*******************************************************************************/
s8 ebn0(u16 pos, u16 pon)
{
	if(pon == 0)
	{
		pon = 1;
	}
	return (s8)(20*lg(pos/pon)-3.0103);			//10*log10(pos^2/pon^2/2) = 20*log10(pos/pon)-10*log10(2)
}

#ifdef USE_RSCODEC
/*******************************************************************************
* Function Name  : is_xmode_bpsk
* Description    : 判断发送模式是否是BPSK
* Input          : 
* Output         : 
* Return         : BOOL
*******************************************************************************/
BOOL is_xmode_bpsk(u8 xmode)
{
	return ((xmode&0x03) == 0)?TRUE:FALSE;
}
#endif

#ifdef USE_RSCODEC
/*******************************************************************************
* Function Name  : is_xmode_dsss
* Description    : 判断发送模式是否是DSSS
* Input          : 
* Output         : 
* Return         : BOOL
*******************************************************************************/
BOOL is_xmode_dsss(u8 xmode)
{
	return ((xmode&0x03) > 0)?TRUE:FALSE;
}
#endif



/*******************************************************************************
* Function Name  : packet_chips()
* Description    : calculate the total chips of the packet.
					e.g. if BPSK, chips = bits; 
						 if DS15: chips = bits x 15; 
						 if DS63: chips = bits x 63;
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u32 packet_chips(BASE_LEN_TYPE ppdu_len, u8 rate, u8 srf) reentrant
{
	u32 chips;
#ifdef USE_RSCODEC
	if(((rate&0x03) == 0) && !srf)
	{
		ppdu_len = rsencoded_len(ppdu_len);
	}
#else
	srf = srf;
#endif

	chips = (ppdu_len * 11 + 57); 
	
	if((rate&0x03) == 1)
	{
		chips = chips * 15;
	}
	else if((rate&0x03) == 2)
	{
		chips = chips * 63;
	}
	return chips;
}

/*******************************************************************************
* Function Name  : encode_xmode()
* Description    : 
		4-bit Xmt Mode Code Table:
		0x0:CH0+BPSK;0x1:CH1+BPSK;0x2:CH2+BPSK;0x3:CH3+BPSK
		0x4:CH0+DS15;0x5:CH1+DS15;0x6:CH2+DS15;0x7:CH3+DS15
		0x8:CH0+DS63;0x9:CH1+DS63;0xA:CH2+DS63;0xB:CH3+DS63
		(SCAN==0)
		0x0C:SALVO+BPSK;0x0D:SALVO+DS15;0x0E:SALVO+DS63;
		(SCAN==1)
		0x0C:SCAN+BPSK;0x0D:SCAN+DS15;0x0E:SCAN+DS63;

* Input          : xmode, xmode must be either single channel or salvo mode if not scan enable; 
* Output         : 
* Return         : code
*******************************************************************************/
u8 encode_xmode(u8 xmode, u8 scan)
{
	u8 xcode = 0;

	if(!scan)									// 2012-12-26 add xmode parameter check
	{
		switch(xmode & 0xf0)
		{
			case(0x10):
			case(0x20):
			case(0x40):
			case(0x80):
			case(0xf0):
			break;
			default:
			xmode |= 0xf0;
		}
	}
	
	if(scan)
	{
		xcode = 0x0C + (xmode&0x03);
	}
	else
	{
		if((xmode & 0xF0) == 0xF0)
		{
			xcode = 0x0C + (xmode&0x03);
		}
		else
		{
			xcode = ((xmode>>5)==4)?0x03:((xmode>>5));
			xcode += ((xmode&0x0F)<<2);
		}
	}
	return xcode;
}

#ifdef USE_PSR
/*******************************************************************************
* Function Name  : decode_xmode()
* Description    : 
* Input          : code
* Output         : 
* Return         : xmode
*******************************************************************************/
u8 decode_xmode(u8 xcode)
{
	//my_printf("decode0x%bx:%bx\n",xcode,CODE2MODE_VAR[xcode]);
	return CODE2MODE_VAR[xcode];
}
#endif

/*******************************************************************************
* Function Name  : is_zx_valid()
* Description    : 检查芯片过零点信号是否有效
* Input          : phase
* Output         : 
* Return         : 
* Revision History
* 2015-01-13 zx invalid double check;
*******************************************************************************/
#if POWERLINE == PL_AC
BOOL is_zx_valid(u8 phase)
{
	u8 period;									// 2014-03-21 BL6810ZX逻辑bug,可能为00或FF

	phase = phase;

	period = read_reg(phase,ADR_ACPS_PERIOD);
	if((period ==0x8C)||(period ==0x00)||(period ==0xFF))	//BL6810 ZX逻辑BUG, 可能为8C, 可能为00或FF
	{
		period = read_reg(phase,ADR_ACPS_PERIOD);
		if((period ==0x8C)||(period ==0x00)||(period ==0xFF))
		{
#ifdef DEBUG_DISP_INFO
			my_printf("phase %bu zx invalid!, period:0x%bx\r\n",phase,period);
#endif
			return FALSE;
		}
	}

	return TRUE;
}
#endif


/*******************************************************************************
* Function Name  : get_timing_version()
* Description    : 得到4字节的版本号信息, 分别代表编译的年月日时, 均为16进制BCD码
* Input          : phase
* Output         : 
* Return         : 
*******************************************************************************/
void get_timing_version(TIMING_VERSION_HANDLE ver_handle)
{
	ver_handle->year = VER_YEAR;
	ver_handle->month = VER_MONTH;
	ver_handle->day = VER_DAY;
	ver_handle->hour = VER_HOUR;
}

/*******************************************************************************
* Function Name  : set_meter_id()
* Description    : 应用层调用设置表号(METER ID), 用于设置  //mt only
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void set_meter_id(METER_ID_HANDLE meter_id_handle)
{
	u8 cs;
	u8 i;
	ARRAY_HANDLE ptr;

	if(meter_id_handle)
	{
		mem_cpy(_meter_id,meter_id_handle,6);
		cs = 0;
		ptr = meter_id_handle;
		for(i=0;i<6;i++)
		{
			cs = cs + *(ptr++);
		}
		cs = ~cs;
		
		_meter_id[6] = cs;
	}
}

#if BRING_USER_DATA == 1
/*******************************************************************************
* Function Name  : set_user_data()
* Description    : MT设置在组网过程中需要向集中器携带的用户信息
* Input          : 
* Output         : 
* Return         : 
* Author		 : Lv Haifeng
* Date			 : 2014-09-24
*******************************************************************************/
STATUS set_user_data(ARRAY_HANDLE user_data, u8 user_data_len)
{
	if(user_data_len > CFG_USER_DATA_BUFFER_SIZE)
	{
		user_data_len = CFG_USER_DATA_BUFFER_SIZE;
	}
	mem_cpy(_user_data,user_data,user_data_len);
	_user_data_len = user_data_len;
	return OKAY;
}
#endif

/*******************************************************************************
* Function Name  : check_meter_id()
* Description    : 检查表号是否有效
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
RESULT is_meter_id_valid()
{
	u8 cs;
	u8 i;
	ARRAY_HANDLE ptr;

	cs = 0;
	ptr = _meter_id;
	for(i=0;i<6;i++)
	{
		cs += *(ptr++);
	}
	
	return (RESULT)((~cs)==(*ptr));
}

/*******************************************************************************
* Function Name  : check_meter_id()
* Description    : 检查表号是否匹配
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
RESULT check_meter_id(METER_ID_HANDLE meter_id_handle)
{
	return (RESULT)(mem_cmp(_meter_id,meter_id_handle,6));
}

#if NODE_TYPE==NODE_MASTER
/*******************************************************************************
* Function Name  : check_quit_loops()
* Description    : 检查全局退出循环等待,回到主线程循环标志
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
BOOL check_quit_loops()
{
	return (BOOL)quit_loops;
}

/*******************************************************************************
* Function Name  : set_quit_loops()
* Description    : 设置全局退出循环等待,回到主线程循环标志
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void set_quit_loops()
{
	quit_loops = 1;
}

/*******************************************************************************
* Function Name  : clr_quit_loops()
* Description    : 清除全局退出循环等待,回到主线程循环标志
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void clr_quit_loops()
{
	quit_loops = 0;
}

/*******************************************************************************
* Function Name  : read_pwer
* Description    : 读pwer值
* Input          : ch: 0~3
* Output         : 
* Return         : 
*******************************************************************************/
u32 read_pwer(u8 phase, u8 ch)
{
	u32 pwer;
	u8 i;
	u32 pwer_total = 0;

	ch &= 0x03;
	write_reg(phase, 0x61, ch<<4);


	for(i=0;i<100;i++)
	{
		pwer = read_reg(phase,0x62);
		pwer <<= 8;
		pwer +=  read_reg(phase,0x63);
		pwer <<= 8;
		pwer +=  read_reg(phase,0x64);			//ignore the sync signal (spi speed too low)
		pwer_total += pwer;
	}

	return pwer_total/100;
}

//#define depth 1024
//u8 mfoi[depth];
//void mfoi_report(u8 phase, u8 ch)
//{
//	u16 i;
//	u8 * ptr;

//	ENTER_CRITICAL();
//	
//	ptr = mfoi;
//	for(i=0; i<depth/2; i++)
//	{
//		write_reg(phase, 0x61, (ch<<4));		// clear mfoi valid;
////		temp = read_reg(phase, 0x61);
////		while(!(temp & 0x02))
////		{
////			my_printf("not ready");
////			temp = read_reg(phase, 0x61);
////		}
//	
//		*ptr++ = read_reg(phase,0x65);
//		*ptr++ = read_reg(phase,0x66);
//	}
//	EXIT_CRITICAL();
//	uart_send_asc(mfoi,depth);
//}

//u8 pwer[depth];
////void pwer_report(u8 phase, u8 ch)
////{
////	u16 i;
////	u8 temp;
////	u8 * ptr;
////
////	ENTER_CRITICAL();
////	
////	ptr = pwer;
////	for(i=0; i<depth/3; i++)
////	{
////		write_reg(phase, 0x61, (ch<<4));		// clear pwer valid;
////		temp = read_reg(phase, 0x61);
////		while(!(temp & 0x01))
////		{
////		//	my_printf("not ready");
////			temp = read_reg(phase, 0x61);
////		}
////	
////		*ptr++ = read_reg(phase,0x62);
////		*ptr++ = read_reg(phase,0x63);
////		*ptr++ = read_reg(phase,0x64);
////	}
////	EXIT_CRITICAL();
////	uart_send_asc(pwer,depth);
////}

//void pwer_report(u8 phase, u8 ch)
//{
//	unsigned char xdata * ptr = pwer;
//	unsigned int i;
//	
//	ENTER_CRITICAL();
//	write_reg(phase,0x61,(ch<<4));
//	
//	for(i = 0; i<depth/3*3; i = i+3)
//	{
//		unsigned char temp;

//		read_reg(phase,0x00);
//		read_reg(phase,0xFF);
//		
//		temp = read_reg(phase,0x61);
//		while(!(temp & 0x01))
//		{
//			temp = read_reg(phase,0x61);
//		}
//		read_reg(phase,0x00);
//		read_reg(phase,0xFF);
//	
//		*ptr++ = read_reg(phase,0x62);
//		*ptr++ = read_reg(phase,0x63);
//		*ptr++ = read_reg(phase,0x64);
//		
//		write_reg(phase,0x61,temp&0xFE);

//		read_reg(phase,0x00);
//		read_reg(phase,0xFF);
//		
//	}
//	EXIT_CRITICAL();
//	uart_send_asc(pwer, depth/3*3);
//}

/*******************************************************************************
* Function Name  : get_noise_status(u8 phase)
* Description    : 获得无信号时噪声等级
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u8 get_noise_status(u8 phase)
{
	u8 agc_value,agc_backup;
	u32 pwer_ch0;
	u32 pwer_ch1;
	u32 pwer_ch2;
	u32 pwer_ch3;
	u32 pwer;
	float temp;
	float dbm;
	
	/**fix agc**/
	agc_value = read_reg(phase,ADR_AGCWORD);
	agc_backup = agc_value;
	write_reg(phase,ADR_AGCWORD,0x80);					//fix agc to minimum

	/** delay and wait wave to be stable **/
	for(pwer_ch0=0;pwer_ch0<10000;pwer_ch0++);			//delay for a while
	
	/** read 4 ch pwer **/
	ENTER_CRITICAL();
	pwer_ch0 = read_pwer(phase, 0);
	pwer_ch1 = read_pwer(phase, 1);
	pwer_ch2 = read_pwer(phase, 2);
	pwer_ch3 = read_pwer(phase, 3);
	EXIT_CRITICAL();

	my_printf("pwer_ch0:%lu,pwer_ch1:%lu,pwer_ch2:%lu,pwer_ch3:%lu,agc:%bx\r\n",pwer_ch0,pwer_ch1,pwer_ch2,pwer_ch3,agc_value);

	pwer = (pwer_ch0 + pwer_ch1 + pwer_ch2 + pwer_ch3)/4;

	temp = (float)(pwer)/3*32/24;								//从pwer转到dfoi,dfoq的平方和均值
	temp = temp*UV_PER_STEP*UV_PER_STEP/1000000UL/1000000UL;	//样值平方均值(单位:伏)
	temp = temp/2*1000000;											//转换为微瓦
	dbm = 10*lg(temp);											//转换为dbm, 32/106是系数
	dbm = (dbm+163)/200*16;
	my_printf("dbm:%bd\r\n",(s8)dbm);
	
	
	my_printf("max\r\n");
	agc_value = 0;
	pwer = 9427971;
	temp = (float)(pwer)/3*32/24;								//从pwer转到dfoi,dfoq的平方和均值
	temp = temp*UV_PER_STEP*UV_PER_STEP/1000000UL/1000000UL;	//样值平方均值(单位:伏)
	temp = temp/2*1000000;											//转换为微瓦
	dbm = 10*lg(temp);											//转换为dbm, 32/106是系数
	dbm = (dbm+163)/200*16;
	my_printf("dbm:%bd\r\n",(s8)dbm);
	
	my_printf("min\r\n");
	agc_value = 0x1F;
	pwer = 3;
	temp = (float)(pwer)/3*32/24;								//从pwer转到dfoi,dfoq的平方和均值
	temp = temp*UV_PER_STEP*UV_PER_STEP/1000000UL/1000000UL;	//样值平方均值(单位:伏)
	temp = temp/2*1000000;											//转换为微瓦
	dbm = 10*lg(temp);											//转换为dbm, 32/106是系数
	dbm = (dbm+163)/200*16;
	my_printf("dbm:%bd\r\n",(s8)dbm);


	/** release agc **/
	write_reg(phase,ADR_AGCWORD,agc_backup);				//release agc
	
	return (u8)dbm;
}



/*******************************************************************************
* Function Name  : powermesh_clear_apdu_rcv(void)
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void powermesh_clear_apdu_rcv(void)
{
	u8 * apdu;	
	APP_RCV_STRUCT ars;
	
	apdu = OSMemGet(SUPERIOR);
	if(apdu)
	{
		ars.apdu = apdu;
		app_rcv(&ars);
		OSMemPut(SUPERIOR,apdu);
	}
}



#endif


