#include "compile_define.h"
#include "powermesh_include.h"
#include "stdarg.h"

#define UART_BUFFER_DEPTH			320
#define UART_RCV_EXPIRE_TIMING		2		//uart expire timing: 2 timer sticks

#define MY_PRINTF_BYTE	0x01
#define MY_PRINTF_LONG	0x02
#define MY_PRINTF_MINUS 0x04
#define MY_PRINTF_HEX	0x08
#define MY_PRINTF_STR	0x10

/* Private variables ---------------------------------------------------------*/
u8	xdata _uart_rcv_buffer[UART_BUFFER_DEPTH];
u8	xdata _uart_rcv_valid;
u8	xdata _uart_rcv_loading;
u16	xdata _uart_rcv_bytes;
u16 xdata _uart_rcv_timer_stamp;

#define my_putchar(x)	uart_send8(x)


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
	init_measure_com_hardware();
	init_usart2_hardware();
}

/*******************************************************************************
* Function Name  : uart_rcv_int_svr
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void uart_rcv_int_svr(u8 rec_data)
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
	va_end(args);
}

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
			uart_send8(temp);

			temp = (*pt++)&0x0F;
			temp = temp + ((temp < 10) ? '0' : ('A'-10));

			uart_send8(temp);
		}
		//uart_send8(' ');	
	}
}

u8 bcd2dec(u8 bcd)
{
	return (bcd>>4)*10 + (bcd&0x0F);
}


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


void powermesh_debug_cmd_proc(u8 xdata * ptr, u16 total_rec_bytes)
{
	u8 out_buffer[256];

	u8 xdata out_buffer_len;
	ARRAY_HANDLE ptw;
	u8 xdata proc_rec_bytes;
	u8 xdata cmd;
	u16 xdata rest_rec_bytes;
	
	out_buffer_len = 0;
	if(out_buffer)
	{
		ptw = out_buffer;
		proc_rec_bytes = 0;
		
		while(proc_rec_bytes < total_rec_bytes)
		{
			cmd = *ptr++;
			proc_rec_bytes++;
			rest_rec_bytes = total_rec_bytes - proc_rec_bytes;

			if(cmd=='C')								//0x43: check global vars
			{
				my_printf("response\n");
			}
			else if(cmd=='R' && rest_rec_bytes>=1)		//0x52: read a byte from current phase
			{
				*ptw++ = read_spi(*ptr++);
				proc_rec_bytes++;
				out_buffer_len++;
			}
			else if(cmd=='W' && rest_rec_bytes>=1)		//0x57: write a byte from current phase
			{
				u8 addr;
				u8 value;

				addr = *ptr++;
				value = *ptr++;
				
				write_spi(addr,value);
				proc_rec_bytes++;
			}
			else if(cmd=='r' && rest_rec_bytes>=1)	//0x72 读6532
			{
				extern s32 convert_uint24_to_int24(u32 value);

				s32 value;
				value = convert_uint24_to_int24(read_measure_reg(*ptr++));
				my_printf("read:%d\n",value);
			}
			else if(cmd=='w' && rest_rec_bytes>=4)	//0x77 写6532
			{
				u8 addr;
				u32 dword = 0;

				addr = *ptr++;
				dword += *ptr++;
				dword <<= 8;
				dword += *ptr++;
				dword <<= 8;
				dword += *ptr++;
				
				write_measure_reg(addr,dword);
				my_printf("set:%lX\n",dword);
			}
			
			else if(cmd=='v' && rest_rec_bytes>=2)	//0x76		设置测量点电压
			{
			
				u8 index;
				u32 value;
				
				index = *ptr++;
				value = read_int(ptr,rest_rec_bytes-1);

				set_v_calib_point(index,value);
				break;
			}
			else if(cmd=='i' && rest_rec_bytes>=2)	//0x69		设置测量点电流
			{
				u8 index;
				u32 value;
				
				index = *ptr++;
				value = read_int(ptr,rest_rec_bytes-1);

				set_i_calib_point(index,value);
				break;
			}

			
			else if(cmd=='t')	//0x74		测试读取当前电压电流
			{
				s32 v,i;
				
				v = measure_current_v();
				i = measure_current_i();
				my_printf("v:%d,i:%d\n",v,i);
				break;
			}
			
			
			else
			{
				uart_rcv_resume();
				break;
			}
		}
		uart_send_asc(out_buffer, out_buffer_len);
#if DEVICE_TYPE == DEVICE_MT	
		OSMemPut(SUPERIOR, out_buffer);
#endif
	}
}


void debug_uart_cmd_proc(void)
{
	u16 xdata total_rec_bytes;
	ARRAY_HANDLE ptr;
	
	total_rec_bytes = uart_rcv(&ptr);
	if(total_rec_bytes !=0)
	{
		powermesh_debug_cmd_proc(ptr,total_rec_bytes);
		uart_rcv_resume();
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


