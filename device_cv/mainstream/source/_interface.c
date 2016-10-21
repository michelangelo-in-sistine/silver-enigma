/******************** (C) COPYRIGHT 2016 ***************************************
* File Name          : interface.c
* Author             : Lv Haifeng
* Version            : V1.0.3
* Date               : 2016-09-14
* Description        : 
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODECTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, BELLING INC. SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAINMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

#include "compile_define.h"
#include "powermesh_include.h"
#include "stdarg.h"
#include "stdlib.h"

/* Macro Definition ---------------------------------------------------------*/
#ifdef DEBUG_MODE
#define UART_BUFFER_DEPTH			320
#define UART_RCV_EXPIRE_TIMING		2		//uart expire timing: 2 timer sticks

#define MY_PRINTF_BYTE	0x01
#define MY_PRINTF_LONG	0x02
#define MY_PRINTF_MINUS 0x04
#define MY_PRINTF_HEX	0x08
#define MY_PRINTF_STR	0x10
#endif

/* Function Code Definition ---------------------------------------------------------*/
#define INTERFACE_CMD_READ_UID			0x01
#define INTERFACE_CMD_GW_UID			0x02			// set GW UID
#define INTERFACE_CMD_READ_PHY_REG		0x03
#define INTERFACE_CMD_SET_PHY_REG		0x04
#define INTERFACE_CMD_SET_INDICATION_LEVEL			0x05
#define INTERFACE_CMD_PHY_SEND			0x10
#define INTERFACE_CMD_DLL_SEND			0x11
#define INTERFACE_CMD_DIAG				0x12
#define INTERFACE_CMD_PLC_INDICATION	0x20

#define INTERFACE_CMD_EXCEPTION					0x80
#define INTERFACE_CMD_UPLINK					0x40		//CV传给GW的标志
#define INTERFACE_CMD_EXCEPTION_UNKNOWN_CMD		0x81		//错误的命令字
#define INTERFACE_CMD_EXCEPTION_ERROR_FORMAT	0x82		//错误的命令格式
#define INTERFACE_CMD_EXCEPTION_DIAG_FAIL		0x83		//diag失败







/* Private variables ---------------------------------------------------------*/
u8	xdata _uart_rcv_buffer[UART_BUFFER_DEPTH];
u8	xdata _uart_rcv_valid;
u8	xdata _uart_rcv_loading;
u16	xdata _uart_rcv_bytes;
u16 xdata _uart_rcv_timer_stamp;

u8 _interface_local_addr[2];
u8 _interface_gw_addr[2];
u8 _interface_uart_buffer[UART_BUFFER_DEPTH];


/* External variables ---------------------------------------------------------*/
extern DLL_RCV_STRUCT xdata _dll_rcv_obj[];


/* Private prototype ---------------------------------------------------------*/
void interface_uart_cmd_proc(u8 xdata * ptr, u16 total_rec_bytes);
void interface_plc_proc(DLL_RCV_HANDLE pd);


/*******************************************************************************
* Function Name  : init_interface
* Description    : interface通信地址是uid的crc, 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void init_interface(void)
{
	u8 uid[6];
	u16 crc;
	
//	init_uart();
	
	get_uid(0,uid);
	crc = calc_crc(uid,6);
	_interface_local_addr[0] = (u8)(crc>>8);
	_interface_local_addr[1] = (u8)(crc);

	_interface_gw_addr[0] = 0;
	_interface_gw_addr[1] = 0;			//gw通信地址初始化为全0, gw上电应该对其设置, 当检查到返回的通信地址为0时, 即了解到CV复位
}


/*******************************************************************************
* Function Name  : init_uart
* Description    : ...
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void init_uart(void)
{
	/* 软件设置 */
	_uart_rcv_valid = 0;
	_uart_rcv_loading = 0;
	_uart_rcv_bytes = 0;
	_uart_rcv_timer_stamp = 0;

	/* 启动硬件 */
	init_interface_uart_hardware();
}

/*******************************************************************************
* Function Name  : interface_uart_rcv_int_svr
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void interface_uart_rcv_int_svr(u8 rec_data)
{
	if(!_uart_rcv_valid)
	{
		if(!_uart_rcv_loading)
		{
			_uart_rcv_buffer[0] = rec_data;
			_uart_rcv_bytes=1;
			_uart_rcv_loading = 1;
			_uart_rcv_timer_stamp = get_global_clock16();

		}
		else
		{
			if(_uart_rcv_bytes < UART_BUFFER_DEPTH)
			{
				_uart_rcv_buffer[_uart_rcv_bytes] = rec_data;
				_uart_rcv_bytes++;
				_uart_rcv_timer_stamp = get_global_clock16();
			}
		}
	}
}

/*******************************************************************************
* Function Name  : uart_rcv
* Description    : If uart is loading, check the time stamp to judge whether the transmission is over.
* Input          : None
* Output         : Head address of received uart data.
* Return         : byte length of received uart data.
*******************************************************************************/
u16 uart_rcv(ARRAY_HANDLE* pt_head_pointer)
{
	u16 temp;
	u16 time_stamp;

	if(_uart_rcv_valid)
	{
		return _uart_rcv_bytes;
	}
	else
	{
		if(_uart_rcv_loading)
		{
			temp = _uart_rcv_bytes;
			time_stamp = get_global_clock16();
			if(time_stamp - _uart_rcv_timer_stamp > UART_RCV_EXPIRE_TIMING)			//Expire time : 2 Byte 
			{
				if(temp == _uart_rcv_bytes)			// avoid such instance: a new byte received during the judgement process, and timing ticks increased;
				{
					_uart_rcv_loading = FALSE;
					_uart_rcv_valid = TRUE;
					*pt_head_pointer = _uart_rcv_buffer;
					return _uart_rcv_bytes;
				}
			}
		}
		return 0;
	}
}




/*******************************************************************************
* Function Name  : uart_rcv_resume
* Description    : After the usart buffer info has been proceeded, main func will call this func to enable usart receiving
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void uart_rcv_resume(void)
{
	_uart_rcv_valid = 0;
	_uart_rcv_loading = 0;
	_uart_rcv_bytes = 0;
}

/*******************************************************************************
* Function Name  : uart_send
* Description    : Send n-byte vector via USART2
* Input          : The head address, bytes info
* Output         : None
* Return         : None
*******************************************************************************/
#if NODE_TYPE==NODE_MASTER || NODE_TYPE==NODE_MODEM
void uart_send(u8 * pt, u8 len)
{
	u8	i;
	for(i=0;i<len;i++)
	{
		my_putchar(*(pt++));
	}
#ifdef USE_DMA
	dma_uart_start();
#endif
}

#endif

/*******************************************************************************
* Function Name  : uart_send_asc
* Description    : Send n-byte vector via USART by ASCII Code
* Input          : The head address, bytes info
* Output         : None
* Return         : None
*******************************************************************************/
void uart_send_asc(ARRAY_HANDLE pt, u16 len) reentrant
{
	u16 i;
	u8 temp;

	if(len>0)
	{
		for(i=0;i<len;i++)
		{
			temp = (*pt)>>4;
			temp = temp + ((temp < 10) ? '0' : ('A'-10));
			my_putchar(temp);

			temp = (*pt++)&0x0F;
			temp = temp + ((temp < 10) ? '0' : ('A'-10));

			my_putchar(temp);
		}
		//uart_send8(' ');	
#ifdef USE_DMA
		dma_uart_start();
#endif
	}
}


/*******************************************************************************
* Function Name  : my_printf
* Description    : customized my_printf function, same syntax as keil c51 printf function.
					support such data type:
					%%  -   ascii '%';
					%bu	-	unsigned char;
					%bd	-	signed char;
					%bx	-	unsigned char(hex format);
					%u  -   unsigned short;
					%d  -   signed short;
					%x  -   unsigned short(hex format);
					%lu  -   unsigned short;
					%ld  -   signed long;
					%lx  -   signed long(hex format);
					%s  -   string;
				   DATA TYPE SIGNATURE MUST MATCH THE VARIABLE LISTED BEHIND !!!
				   otherwise, un-expected result could happen!
				   (This problem is the weakness of keil c compiler since the limited resource of 8051, 
				   in ANSI C, va_arg is forced to be converted into 32bit int, avoiding such problems)
				   
				   AND, LONG DATA TYPE VARIABLES CAN'T BE PRINTED TO MUCH (>2) IN ONE PRINTF COMMAND.
				   
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void my_printf(const char code * fmt, ...) reentrant
{
	va_list args;
	u8 buffer[10];

	u32 value = 0;					// variables are readed and converted into u32, for unified output.
	u8 len;
	u8 flag;
	u8 temp;
	u8 * ptr = 0;

	va_start(args,fmt);			// make args pointed to the first variable called in.

	while(*fmt != 0)
	{
		if(*fmt != '%')
		{
			my_putchar(*fmt++);	// output directly if it is not %, so this function can support unlimited string length.
		}
		else
		{
			flag = 0;
			fmt++;
	    repeat:
			switch(*(fmt++))
			{
				case '%': my_putchar(*fmt++); continue;
				case 'b': flag |= MY_PRINTF_BYTE; goto repeat;
				case 'l': flag |= MY_PRINTF_LONG; goto repeat;
				case 'u':
				{
					if(flag & MY_PRINTF_BYTE)
					{
#ifdef CPU_STM32
						value = va_arg(args, unsigned int);			// avoid compiler warning
#else
						value = va_arg(args, unsigned char);
#endif						

					}
					else if(flag & MY_PRINTF_LONG)
					{
						value = va_arg(args, unsigned long);
					}
					else
					{
						value = va_arg(args, unsigned int);
					}
					break;
				}
				case 'd':
				{
					if(flag & MY_PRINTF_BYTE)
					{
#ifdef CPU_STM32
						value = va_arg(args, signed int);			// avoid compiler warning
#else
						value = va_arg(args, signed char);
#endif
					}
					else if(flag & MY_PRINTF_LONG)
					{
						value = va_arg(args, signed long);
					}
					else
					{
						value = va_arg(args, signed int);
					}
					
					if(value>=0x80000000)
					{
						flag |= MY_PRINTF_MINUS;
						value = -value;
					}
	
					break;
				}
				case 'x':
				case 'X':
				{
					flag |= MY_PRINTF_HEX;
					if(flag & MY_PRINTF_BYTE)
					{
#ifdef CPU_STM32
						value = va_arg(args, unsigned int);
#else
						value = va_arg(args, unsigned char);
#endif
					}
					else if(flag & MY_PRINTF_LONG)
					{
						value = va_arg(args, unsigned long);
					}
					else
					{
						value = va_arg(args, unsigned int);
						//value = va_arg(args, unsigned char);			// make %x is char
					}
					break;
				}
				case 's':
				{
					flag |= MY_PRINTF_STR;
					ptr = va_arg(args,unsigned char *);
				}
//				case 'f':
//				{
//					/**** float data type not supportted not ***/
//					value = (u32)va_arg(args, float);
//					continue;
//				}
			}
			
			/***************************************************************************
			convert data read in into ascii character and put in buffer in an inverted order, 
			***************************************************************************/
			if(!(flag & MY_PRINTF_STR))
			{
				len = 0;
				if(flag & MY_PRINTF_HEX)
				{
					if(flag & MY_PRINTF_BYTE)
					{
						for(len=0;len<2;len++)			// for hex format, pad 0
						{
							temp = value % 16;
							value /= 16;
							buffer[len] = temp + ((temp < 10) ? '0' : ('A'-10));
						}
					}
					else if(flag & MY_PRINTF_LONG)
					{
						for(len=0;len<8;len++)			// for hex format, pad 0
						{
							temp = value % 16;
							value /= 16;
							buffer[len] = temp + ((temp < 10) ? '0' : ('A'-10));
						}
					}
					else
					{
						for(len=0;len<4;len++)			// for hex format, pad 0
						{
							temp = value % 16;
							value /= 16;
							buffer[len] = temp + ((temp < 10) ? '0' : ('A'-10));
						}
					}
				}
				else
				{
					do
					{
						temp = value % 10;
						value /= 10;
						buffer[len++] = temp + '0';
					}while(value);
				}
			}

			/***************************************************************************
			output it by uart in a unified process.
			***************************************************************************/
			if(flag & MY_PRINTF_STR)
			{
				do
				{
					my_putchar(*ptr++);
				}while(*ptr);
			}
			else
			{
				if(flag & MY_PRINTF_MINUS)
				{
					my_putchar('-');
				}

				do
				{
					my_putchar(buffer[--len]);
				}
				while(len>0);
			}
		}

	}
#ifdef USE_DMA
	dma_uart_start();
#endif	
	va_end(args);
}

/*******************************************************************************
* Function Name  : bcd2dec()
* Description    : 
* Input          : 
* Output         : None
* Return         : 
*******************************************************************************/
u8 bcd2dec(u8 bcd)
{
	return (bcd>>4)*10 + (bcd&0x0F);
}


/*******************************************************************************
* Function Name  : read_int()
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u32 read_int(u8 * ptr, u8 len)
{
	u32 value = 0;
	u8 i;
	for(i=0;i<len;i++)
	{
		value = value*100;
		value += bcd2dec(*ptr++);
	}
	return value;
}


/*******************************************************************************
* Function Name  : interface_proc()
* Description    : 
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
void interface_proc(void)
{
	u16 xdata total_rec_bytes;
	ARRAY_HANDLE ptr;
	u8 i;

	total_rec_bytes = uart_rcv(&ptr);
	if(total_rec_bytes !=0)
	{
		interface_uart_cmd_proc(ptr,total_rec_bytes);
		uart_rcv_resume();
	}


	for(i=0;i<CFG_PHASE_CNT;i++)
	{
		if(_dll_rcv_obj[i].dll_rcv_valid)
		{
			interface_plc_proc(&_dll_rcv_obj[i]);
			dll_rcv_resume(&_dll_rcv_obj[i]);
		}
	}

}

/*******************************************************************************
* Function Name  : print_packet()
* Description    : 
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
void print_packet(u8 x_mode, ARRAY_HANDLE ppdu, u8 ppdu_len) reentrant
{
	my_printf("@%bx| ",x_mode);
	uart_send_asc(ppdu,ppdu_len);
	my_printf("\r\n");
}

/*******************************************************************************
* Function Name  : print_phy_rcv()
* Description    : 
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
void print_phy_rcv(PHY_RCV_HANDLE pp) reentrant
{
	my_printf("rcv ph%bx,",pp->phase);
	print_packet(pp->phy_rcv_valid,pp->phy_rcv_data,pp->phy_rcv_len);
}

/*******************************************************************************
* Function Name  : check_format(u8 xdata * ptr, u16 total_rec_bytes)
* Description    : cv interactive frame should consist of all [0-9] or [A-Z] 
					ascii characters and included by a pair of '<' and '>'
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
RESULT check_format(u8 xdata * ptr, u16 total_rec_bytes)
{
	u16 i;
	u8 temp;

	if(total_rec_bytes < 12)			//<>2B, addr 4B(ASC Code), function code 2B, body N*2B, crc 4B at least 12B
	{
		return WRONG;
	}

	if(total_rec_bytes & 0x01)		//total_rec_bytes should be even number
	{
		return WRONG;
	}

	if(*ptr++ == '<')
	{
		for(i=1;i<total_rec_bytes-1;i++)
		{
			temp = *ptr++;
			if((temp>='0' && temp<='9') || (temp>='A' && temp<='F'))
			{
				continue;
			}
			else
			{
				return WRONG;
			}
		}
	}

	if(*ptr == '>')
	{
		return CORRECT;
	}
	
	return WRONG;
}

/*******************************************************************************
* Function Name  : hexstr2u8
* Description    : cv frame consists of ASCII character, and should be converted to unsigned char
					Here assume all characters has been checked by check_format() and guarantee all legal ascii numbers
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
u16 hexstr2u8(u8 xdata * ptr, u16 total_rec_bytes)
{
	u8 xdata * ptw;
	u8 temp;
	u16 i;

	ptw = ptr++;

	for(i=0;i<total_rec_bytes-1;i++)
	{
		temp = *ptr++;
		*ptw = ((temp<='9')?(temp-'0'):(temp-'A'+10))<<4;
		temp = *ptr++;
		*ptw++ += ((temp<='9')?(temp-'0'):(temp-'A'+10));
	}
	return (total_rec_bytes>>1)-1;
}

/*******************************************************************************
* Function Name  : check_dest_addr
* Description    : 
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
RESULT check_dest_addr(u8 xdata * ptr)
{
//	u8 uid[6];
//	u16 crc;
	
	if((ptr[0]==0) && (ptr[1]==0))
	{
		return CORRECT;
	}
	else
	{
		//get_uid(0,uid);
		//crc = calc_crc(uid,6);
		//if((ptr[0] == (u8)(crc>>8)) && (ptr[1] == (u8)(crc)))
		//{
		//	return CORRECT;
		//}
		if(ptr[0] == _interface_local_addr[0] && ptr[1] == _interface_local_addr[1])
		{
			return CORRECT;
		}
	}
	return WRONG;
}

void cv_frame_send(u8 xdata * buffer, u16 buffer_len)
{
	u8 * ptw;
	u16 crc;

	ptw = buffer;
	if(buffer_len < UART_BUFFER_DEPTH-4)
	{
		mem_shift(buffer,buffer_len,2);
		*ptw++ = _interface_gw_addr[0];
		*ptw++ = _interface_gw_addr[1];
		ptw += buffer_len;

		crc = calc_crc(buffer,buffer_len+2);
		*ptw++ = (u8)(crc>>8);
		*ptw++ = (u8)(crc);
		my_printf("<");
		uart_send_asc(buffer, buffer_len+4);
		my_printf(">");
	}
}

/*******************************************************************************
* Function Name  : interface_uart_cmd_proc()
* Description    : 
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
void interface_uart_cmd_proc(u8 xdata * ptr, u16 total_rec_bytes)
{
	u8 xdata out_buffer_len;
	ARRAY_HANDLE ptw;
	u8 xdata cmd;
	
	out_buffer_len = 0;

	if(check_format(ptr, total_rec_bytes))
	{
		total_rec_bytes = hexstr2u8(ptr, total_rec_bytes);
		if(check_crc(ptr, total_rec_bytes))
		{
			if(check_dest_addr(ptr))									// check destination(all zero or crc of uid)
			{
				ptr += 2;												// addr 2B
				ptw = _interface_uart_buffer;
				
				cmd = *ptr++;
				*ptw++ = cmd | INTERFACE_CMD_UPLINK;
				out_buffer_len = 1;

				switch(cmd)
				{
					case(INTERFACE_CMD_READ_UID):								//0x43: check global vars
					{
						get_uid(0,ptw);
						out_buffer_len += 6;
						break;
					}
					case(INTERFACE_CMD_GW_UID):
					{
						u16 crc;

						crc = calc_crc(ptr,6);
						_interface_gw_addr[0] = (u8)(crc>>8);
						_interface_gw_addr[1] = (u8)(crc);
						*ptw++ = (u8)(crc>>8);
						*ptw++ = (u8)(crc);
						out_buffer_len += 2;
						break;
					}
					case(INTERFACE_CMD_READ_PHY_REG):
					{
						*ptw++ = read_reg(0,*ptr);
						out_buffer_len += 1;
						break;
					}
					case(INTERFACE_CMD_SET_PHY_REG):
					{
						u8 addr,value;

						addr = *ptr++;
						value = *ptr;
						write_reg(0,addr,value);

						*ptw++ = read_reg(0,addr);
						out_buffer_len += 1;
						break;
					}
					case(INTERFACE_CMD_PHY_SEND):
					{
						PHY_SEND_STRUCT pss;
						u32 delay;
						SEND_ID_TYPE sid;

						mem_clr(&pss,sizeof(PHY_SEND_STRUCT),1);

						pss.phase = *ptr++;
						pss.psdu_len = *ptr++;
						pss.xmode = *ptr++;
						pss.prop = *ptr++;
						
						delay = *ptr++;
						delay <<= 8;
						delay += *ptr++;
						delay <<= 8;
						delay += *ptr++;
						delay <<= 8;
						delay += *ptr++;
						pss.delay = delay;
						pss.psdu = ptr;

						if(pss.psdu_len + 13 == total_rec_bytes)
						{
							sid = phy_send(&pss);
							*ptw = sid;
							out_buffer_len += 1;
						}
						else
						{
							ptw--;
							*ptw++ |= 0xC0;
							*ptw = INTERFACE_CMD_EXCEPTION_ERROR_FORMAT;
							out_buffer_len += 1;
						}
						break;
					}
					case(INTERFACE_CMD_DIAG):
					{
						DLL_SEND_STRUCT dss;
						u8 buffer[32];

						mem_clr(&dss,sizeof(DLL_SEND_STRUCT),1);
						dss.target_uid_handle = ptr;
						ptr += 6;

						dss.xmode = *ptr++;
						dss.rmode = *ptr++;
						dss.prop = BIT_DLL_SEND_PROP_DIAG | BIT_DLL_SEND_PROP_REQ_ACK;
						dss.prop |= (*ptr)?(BIT_DLL_SEND_PROP_SCAN):0;

						if(dll_diag(&dss,buffer))
						{
							mem_cpy(ptw,&buffer[1],buffer[0]);
							out_buffer_len += buffer[0];
						}
						else
						{
							ptw--;
							*ptw++ |= 0xC0;
							*ptw = INTERFACE_CMD_EXCEPTION_DIAG_FAIL;
							out_buffer_len += 1;
						}
						break;
					}
					default:
					{
						ptw--;
						*ptw++ |= 0xC0;
						*ptw = INTERFACE_CMD_EXCEPTION_UNKNOWN_CMD;
						out_buffer_len += 1;
						break;
					}
				}
				cv_frame_send(_interface_uart_buffer, out_buffer_len);
			}
		}
		else
		{
			my_printf("crc check fail\r\n");
			uart_send_asc(ptr, total_rec_bytes);
		}
	}
	else
	{
		my_printf("format check fail\r\n");
		uart_send(ptr, total_rec_bytes);
	}
	uart_rcv_resume();
}

void interface_plc_proc(DLL_RCV_HANDLE pd)
{
	ARRAY_HANDLE ptw;
	PHY_RCV_HANDLE pp;

	ptw = _interface_uart_buffer;
	pp = GET_PHY_HANDLE(pd);

	if(pd->dll_rcv_valid)
	{
		*ptw++ = INTERFACE_CMD_PLC_INDICATION | INTERFACE_CMD_UPLINK;
		*ptw++ = pp->phase;
		*ptw++ = pp->phy_rcv_valid;
		mem_cpy(ptw,pp->phy_rcv_data,pp->phy_rcv_len);
		cv_frame_send(_interface_uart_buffer, pp->phy_rcv_len + 3);

		dll_rcv_resume(pd);
	}
}


