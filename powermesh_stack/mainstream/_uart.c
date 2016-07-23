#include "compile_define.h"
#include "powermesh_include.h"
#include "stdarg.h"
#include "stdlib.h"

#ifdef DEBUG_MODE
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

void powermesh_debug_cmd_proc(u8 xdata * ptr, u16 total_rec_bytes);


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
	init_debug_uart_hardware();
}

/*******************************************************************************
* Function Name  : debug_uart_rcv_int_svr
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void debug_uart_rcv_int_svr(u8 rec_data)
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
#if NODE_TYPE==NODE_MASTER
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
* Function Name  : debug_uart_cmd_proc()
* Description    : 
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
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

#if NODE_TYPE==NODE_MASTER
/*******************************************************************************
* Function Name  : print_transaction_timing(TIMER_ID_TYPE tid, u32 expiring_stick)
* Description    : 
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
void print_transaction_timing(TIMER_ID_TYPE tid, u32 expiring_stick)
{
	my_printf("SET EXPIRE:%lu,ACTRUAL USED:%lu,DIFF:%lu\r\n",
				expiring_stick, expiring_stick-get_timer_val(tid), get_timer_val(tid));
}

/*******************************************************************************
* Function Name  : print_psr_err()
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void print_psr_err(ERROR_STRUCT * psr_err)
{
	my_printf("*psr err report:\r\n");
	my_printf("pipe id: %bX%bX\r\n",(u8)(psr_err->pipe_id>>8),(u8)(psr_err->pipe_id));
	my_printf("report uid: ");
	uart_send_asc(psr_err->uid,6);
	my_printf("\r\ntarget uid: ");
	if(pipeid2uid(psr_err->pipe_id))
	{
		uart_send_asc(pipeid2uid(psr_err->pipe_id),6);
	}
	else
	{
		my_printf("NA");
	}
	my_printf("\r\nerror code: %bX(",psr_err->err_code);
	switch(psr_err->err_code)
	{
		case(NO_ERROR):
		{
			my_printf("no_error)\r\n");
			break;
		}
		case(TARGET_EXECUTE_ERR):
		{
			my_printf("target_execute_err)\r\n");
			break;
		}
		case(NO_PIPE_INFO):
		{
			my_printf("no_pipe_info)\r\n");
			break;
		}
		case(NO_XPLAN):
		{
			my_printf("no_xplan)\r\n");
			break;
		}
		case(ACK_TIMEOUT):
		{
			my_printf("ack_timeout)\r\n");
			break;
		}
		case(PSR_TIMEOUT):
		{
			my_printf("psr_timeout)\r\n");
			break;
		}
		case(DIAG_TIMEOUT):
		{
			my_printf("diag_timeout)\r\n");
			break;
		}
		case(UID_NOT_MATCH):
		{
			my_printf("uid_not_match serious!)\r\n");
			break;
		}
		case(MEM_OUT):
		{
			my_printf("mem_out serious!)\r\n");
			break;
		}
		case(TIMER_RUN_OUT):
		{
			my_printf("timer_out serious!)\r\n");
			break;
		}
		case(SENDING_FAIL):
		{
			my_printf("sending fail serious!)\r\n");
			break;
		}
		default:
		{
			my_printf("unknown_err serious!)\r\n");
			break;
		}
	}

}

/*******************************************************************************
* Function Name  : print_network()
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
extern PIPE_MNPL_STRUCT psr_pipe_mnpl_db[];
extern u16 xdata psr_pipe_mnpl_db_usage;
extern NODE_HANDLE network_tree[];

void print_network()
{
	u16 i;
	u8 script_buffer[64];

	my_printf("/*-------------------------\r\n");
	
	my_printf("NETWORK TREE:\r\n");

	for(i=0;i<CFG_PHASE_CNT;i++)
	{
		my_printf("PH%bX:\r\n",i);
		print_build_network(i);
	}

	//for(i=0;i<CFG_PHASE_CNT;i++)
	//{
	//	my_printf("PH%bX:\r\n",i);
	//	print_branch(network_tree[i]);
	//}

	my_printf("\r\nPIPE STORAGE:\r\n");
	for(i=0;i<psr_pipe_mnpl_db_usage;i++)
	{
		PIPE_REF pipe;
		u16 script_len;

		pipe = &psr_pipe_mnpl_db[i];
		if(pipe->pipe_info != 0xCFFF)
		{
			my_printf("PIPE INFO:%X, PHASE:%bX, ",pipe->pipe_info,pipe->phase);
			my_printf("RECEPIENT:");
			uart_send_asc(inquire_addr_db(pipe->way_uid[pipe->pipe_stages-1]),6);
			my_printf("\r\nSCRIPT:");
//		script_len = get_path_script_from_topology(uid2node(inquire_addr_db(pipe->way_uid[pipe->pipe_stages-1])), script_buffer);
			script_len = get_path_script_from_pipe_mnpl_db(pipe->pipe_info&0x0FFF, script_buffer);
			uart_send_asc(script_buffer,script_len);
			my_printf("\r\n\r\n");
		}
		
	}
	my_printf("-------------------------*/\r\n");
	
}

/*******************************************************************************
* Function Name  : print_build_network()
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
extern NODE_HANDLE solo_list[];
void print_build_network(u8 phase)
{
	u16 phase_nodes;
	u16 num_nodes;

	phase_nodes = get_list_from_branch(network_tree[phase], ALL, ALL, ALL, NULL,0,0)-1;
	my_printf("/*------------------------\r\nNETWORK REPORT: NODES IN PHASE %bd: %d, TOTAL NODES: %d\r\n", phase, phase_nodes, get_network_total_subnodes_count());
	print_branch(network_tree[phase]);

	num_nodes = get_list_from_branch(network_tree[phase], ALL, NODE_STATE_EXCLUSIVE, ALL, solo_list,0,CFG_MGNT_MAX_NEW_LINKS_BUFFER_CNT);
	if(num_nodes)
	{
		my_printf("EXCLUSIVE NODES: %d\r\n", num_nodes);
		print_list(solo_list,num_nodes);
	}
	num_nodes = get_list_from_branch(network_tree[phase], ALL, NODE_STATE_DISCONNECT, NODE_STATE_EXCLUSIVE, solo_list,0,CFG_MGNT_MAX_NEW_LINKS_BUFFER_CNT);
	if(num_nodes)
	{
		my_printf("DISCONNECTED NODES: %d\r\n", num_nodes);
		print_list(solo_list,num_nodes);
	}
	num_nodes = get_list_from_branch(network_tree[phase], ALL, NODE_STATE_REFOUND, NODE_STATE_EXCLUSIVE, solo_list,0,CFG_MGNT_MAX_NEW_LINKS_BUFFER_CNT);
	if(num_nodes)
	{
		my_printf("REFOUND NODES: %d\r\n", num_nodes);
		print_list(solo_list,num_nodes);
	}
	num_nodes = get_list_from_branch(network_tree[phase], ALL, NODE_STATE_RECONNECT, NODE_STATE_EXCLUSIVE, solo_list,0,CFG_MGNT_MAX_NEW_LINKS_BUFFER_CNT);
	if(num_nodes)
	{
		my_printf("RECONNECT NODES: %d\r\n", num_nodes);
		print_list(solo_list,num_nodes);
	}
	num_nodes = get_list_from_branch(network_tree[phase], ALL, NODE_STATE_UPGRADE, NODE_STATE_EXCLUSIVE, solo_list,0,CFG_MGNT_MAX_NEW_LINKS_BUFFER_CNT);
	if(num_nodes)
	{
		my_printf("UPGRADE NODES: %d\r\n", num_nodes);
		print_list(solo_list,num_nodes);
	}
	
	num_nodes = get_list_from_branch(network_tree[phase], ALL, NODE_STATE_OLD_VERSION, ALL, solo_list,0,CFG_MGNT_MAX_NEW_LINKS_BUFFER_CNT);
	if(num_nodes)
	{
		my_printf("EXPIRED TIMING VERSION NODES: %d\r\n", num_nodes);
		print_list(solo_list,num_nodes);
	}
	num_nodes = get_list_from_branch(network_tree[phase], ALL, NODE_STATE_ZX_BROKEN, ALL, solo_list,0,CFG_MGNT_MAX_NEW_LINKS_BUFFER_CNT);
	if(num_nodes)
	{
		my_printf("ZX CIRCUIT BROKEN NODES: %d\r\n", num_nodes);
		print_list(solo_list,num_nodes);
	}
	num_nodes = get_list_from_branch(network_tree[phase], ALL, NODE_STATE_AMP_BROKEN, ALL, solo_list,0,CFG_MGNT_MAX_NEW_LINKS_BUFFER_CNT);
	if(num_nodes)
	{
		my_printf("AMP CIRCUIT BROKEN NODES: %d\r\n", num_nodes);
		print_list(solo_list,num_nodes);
	}

	

//	my_printf("\r\nPIPE STORAGE:\r\n");
//	for(i=0;i<psr_pipe_mnpl_db_usage;i++)
//	{
//		PIPE_REF pipe;
//		u16 script_len;
//
//		pipe = &psr_pipe_mnpl_db[i];
//		if(pipe->phase == phase)
//		{
//			my_printf("PIPE INFO:%X, PHASE:%bX, ",pipe->pipe_info,pipe->phase);
//			my_printf("RECEPIENT:");
//			uart_send_asc(inquire_addr_db(pipe->way_uid[pipe->pipe_stages-1]),6);
//			my_printf("\r\nSCRIPT:");
//			script_len = get_path_script_from_pipe_mnpl_db(pipe->pipe_info&0x0FFF, script_buffer);
//			uart_send_asc(script_buffer,script_len);
//			my_printf("\r\n\r\n");
//		}
//	}
	
	my_printf("------------------------*/\r\n");
}

/*******************************************************************************
* Function Name  : print_pipe()
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void print_pipe(u16 pipe_id)
{
	u8 script_buffer[64];
	u8 script_len;
	PIPE_REF pipe;

	my_printf("/*-------------------------\r\n");

	pipe = inquire_pipe_mnpl_db(pipe_id);
	if(!pipe)
	{
		my_printf("PIPE %X NOT FOUND\r\n",pipe_id);
	}
	else
	{
		my_printf("pipe_id:0x%X\r\n",pipe->pipe_info&0x0FFF);
		my_printf("phase:%bx\r\n",pipe->phase);
		my_printf("dest:[");
		uart_send_asc(inquire_addr_db(pipe->way_uid[pipe->pipe_stages-1]),6);
		my_printf("]\r\nscript:");
		script_len = get_path_script_from_pipe_mnpl_db(pipe_id,script_buffer);
		uart_send_asc(script_buffer,script_len);
		my_printf("\r\n");
	}
	my_printf("-------------------------*/\r\n");
}
#endif

void powermesh_debug_cmd_proc(u8 xdata * ptr, u16 total_rec_bytes)
{
	u8 phase;
	u8 xdata out_buffer[256];

	u8 xdata out_buffer_len;
	ARRAY_HANDLE ptw;
	u8 xdata proc_rec_bytes;
	u8 xdata cmd;
	u16 xdata rest_rec_bytes;
	
	phase = phase;
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
			//else if(cmd=='r' && rest_rec_bytes>=1)	//0x72 读6532
			//{
			//	extern s32 convert_uint24_to_int24(u32 value);

			//	s32 value;
			//	value = convert_uint24_to_int24(read_measure_reg(*ptr++));
			//	my_printf("read:%d\n",value);
			//}
			//else if(cmd=='w' && rest_rec_bytes>=4)	//0x77 写6532
			//{
			//	u8 addr;
			//	u32 dword = 0;

			//	addr = *ptr++;
			//	dword += *ptr++;
			//	dword <<= 8;
			//	dword += *ptr++;
			//	dword <<= 8;
			//	dword += *ptr++;
			//	
			//	write_measure_reg(addr,dword);
			//	my_printf("set:%lX\n",dword);
			//}
			
			else if(cmd==0x70 && rest_rec_bytes>=2)	//0x70		设置测量点电压
			{
			
				u8 index;
				u16 value;
				
				index = *ptr++;
				value = read_int(ptr,rest_rec_bytes-1);
				my_printf("calib v, point:%d, value:%d\r\n",index,value);
				set_v_calib_point(index,value);
				break;
			}
			else if(cmd==0x71 && rest_rec_bytes>=2)	//0x71		设置测量点电流
			{
				u8 index;
				u16 value;
				
				index = *ptr++;
				value = read_int(ptr,rest_rec_bytes-1);
				my_printf("calib i, point:%d, value:%d\r\n",index,value);
				set_i_calib_point(index,value);

				break;
			}

			
			else if(cmd==0x72)	//0x72		测试读取当前电压电流
			{
				s32 v,i;
				
				v = measure_current_v();
				i = measure_current_i();
				my_printf("v:%d,i:%d\n",v,i);
				break;
			}
			else if(cmd==0x73)					//73
			{
				my_printf("reset 6523\n");
				reset_measure_device();			//重启6523
				break;
			}
			else if(cmd==0x74 && rest_rec_bytes >= 4)
			{
				s16 t;

				s32 reg_value;

				reg_value = *ptr++;
				reg_value <<= 8;
				reg_value += *ptr++;
				reg_value <<= 8;
				reg_value += *ptr++;
				reg_value <<= 8;
				reg_value += *ptr++;

				t = calc_temperature(reg_value);

				my_printf("reg_value:%d,t:%d\r\n",reg_value,t);
				break;
			}

			else if(cmd == 0x75 && rest_rec_bytes >= 2)					// calib t
			{
				s16 t;

				t = read_int(ptr,rest_rec_bytes);
			
				my_printf("t=%d\r\n",t);
				set_t_calib_point(t);
				break;
			}

			else if(cmd==0x76)						//0x76 calc t
			{
				s16 t;
				
				t = measure_current_t();
				my_printf("current t:%d\r\n",t);
				break;
			}

			else if(cmd == 0x77 && rest_rec_bytes >= 3)					// calib t
			{
				s16 t;
				s32 reg_value;
				float t0;

				t = read_int(ptr,2);
				reg_value = read_int(ptr+2,rest_rec_bytes-2);
			
				my_printf("t=%d,reg=%d\r\n",t,reg_value);
				t0 = set_exp_calib_point(t,reg_value);

				my_printf("calc t0 = %d.%d\r\n",(s16)(t0),((s16)(t0*100) % 100));
				
				break;
			}

			else if(cmd==0x78 && rest_rec_bytes >= 2)					// 
			{
				s32 reg_value;
				s16 t;
				
				reg_value = read_int(ptr,rest_rec_bytes);
				my_printf("reg_value:%d\r\n",reg_value);

				t = calc_temperature(reg_value);
				my_printf("reg_value:%d,t:%d\r\n",reg_value,t);
				break;
			}

			

			

			else if(cmd==0x79)					//0x73
			{
				save_calib_into_app_nvr();
				my_printf("save");
				break;
			}

			
			
			else if(cmd=='S' && rest_rec_bytes>=9)		//0x53: 53 + X_MODE + SCAN + SRF + AC_UPDATE + 4B Delay + PACKAGE
			{
				PHY_SEND_STRUCT xdata pss;
				SEND_ID_TYPE sid;

				pss.xmode = *ptr++;
				pss.prop = (*ptr++)?(BIT_PHY_SEND_PROP_SCAN):0;
				pss.prop |= (*ptr++)?(BIT_PHY_SEND_PROP_SRF):0;
				pss.prop |= (*ptr++)?(BIT_PHY_SEND_PROP_ACUPDATE):0;
				pss.delay = *ptr++;
				pss.delay = (pss.delay<<8)+(*ptr++);
				pss.delay = (pss.delay<<8)+(*ptr++);
				pss.delay = (pss.delay<<8)+(*ptr++);
#if DEVICE_TYPE == DEVICE_CC	
				pss.phase = phase;
#else
				pss.phase = 0;
#endif
				pss.psdu = ptr;
				pss.psdu_len = rest_rec_bytes-8;
				sid = phy_send(&pss);
				sid = sid;
				//if(sid != INVALID_RESOURCE_ID)
				//{
				//	my_printf("ACCEPTED!\r\n");
				//}
				//else
				//{
				//	my_printf("ERROR: SEND QUEUE NOT ACCEPTED!\r\n");
				//}
				break;
			}
			else if(cmd=='D' && rest_rec_bytes>=9)								//0x44	Diag Test 格式: 44 + 对方UID + XMODE + RMODE + SCAN
			{
				/* 测试Dll.Diag */
				DLL_SEND_STRUCT xdata dds;
				ARRAY_HANDLE buffer;

				mem_clr(&dds,sizeof(DLL_SEND_STRUCT),1);
#if DEVICE_TYPE == DEVICE_MT				
				dds.phase = 0;
#else
				dds.phase = 0;
#endif
				dds.target_uid_handle = ptr;
				ptr+=6;
				dds.xmode = *ptr++;
				dds.rmode = *ptr++;
				dds.prop = BIT_DLL_SEND_PROP_DIAG | BIT_DLL_SEND_PROP_REQ_ACK;
				dds.prop |= (*ptr)?(BIT_DLL_SEND_PROP_SCAN):0;

				buffer = OSMemGet(MINOR);
				if(buffer != NULL)
				{
					if(dll_diag(&dds, buffer))
					{
						u8 i;
//						uart_send_asc(buffer+1,buffer[0]);
						my_printf("\r\nTarget:");
						uart_send_asc(dds.target_uid_handle,6);
						my_printf("\r\n------------------------------------------\r\n\tDownlink\t\tUplink\t\r\n");
						my_printf("Diag\tSS(dbuv)\tSNR(dB)\tSS(dbuv)\tSNR(dB)\r\n");
						for(i=0;i<4;i++)
						{
							my_printf("CH%bu\t%bd\t%bd",i,buffer[i*2+1],buffer[i*2+2]);
							my_printf("\t%bd\t%bd\r\n",buffer[i*2+9],buffer[i*2+10]);
						}
						
						
						my_printf("\r\n");
					}
					else
					{
						my_printf("Diag Fail!\r\n");
					}
					OSMemPut(MINOR,buffer);
				}
				break;
			}
			else if(cmd=='E' && rest_rec_bytes)		//4545
			{
				erase_user_storage();
				break;
			}
			else if(cmd=='F' && rest_rec_bytes)		//46 + 要写入的字节
			{
				u16 result;
				result = write_user_storage(ptr,rest_rec_bytes);
				if(result)
					my_printf("write %d bytes\n",result);
				else
					my_printf("write fail");
				break;
			}
			else if(cmd=='G' && rest_rec_bytes==1)		//47 + 要读出的字节数
			{
				u16 read_bytes;

				read_bytes = *ptr++;
				read_bytes = read_user_storage(out_buffer,read_bytes);
				out_buffer_len += read_bytes;
				break;
			}
			else if(cmd == 'A' && rest_rec_bytes >= 6)	//0x41 + 域ID, VID, GID 设置
			{
				extern u16 _self_domain;
				extern u16 _self_vid;
				extern u16 _self_gid;

				u16 temp;

				temp = *ptr++;
				temp <<= 8;
				temp += *ptr++;
				_self_domain = temp;

				temp = *ptr++;
				temp <<= 8;
				temp += *ptr++;
				_self_vid = temp;

				temp = *ptr++;
				temp <<= 8;
				temp += *ptr++;
				_self_gid = temp;

				save_id_info_into_app_nvr();
				init_app_nvr_data();
				init_app();
				break;
			}
			else if(cmd == 'H' && rest_rec_bytes >= 12)	//0x48 + UID + 域ID, VID, GID
			{
				u16 did, vid, gid;
				UID_HANDLE uid;

				uid = ptr;

				ptr += 6;

				did = *ptr++;
				did <<=8;
				did += *ptr++;

				vid = *ptr++;
				vid <<=8;
				vid += *ptr++;

				gid = *ptr++;
				gid <<=8;
				gid += *ptr++;

				if(set_se_node_id_by_uid(uid,did,vid,gid))
				{
					my_printf("addr set ok");
				}
				break;
			}
			else if(cmd == 0x4A && rest_rec_bytes>= 7) //acp_req_by_uid: 0x4A + UID + wspbody
			{
				UID_HANDLE uid;
				u8 return_len;

				uid = ptr;
				ptr += 6;
				
				return_len = acp_req_by_uid(uid, ptr, rest_rec_bytes-6, ptw, 20);
				out_buffer_len += return_len;
				break;
			}
			else if(cmd == 0x4B && rest_rec_bytes>= 8) //0x4B + domain id + vid + wsp_body
			{
				u16 domain_id;
				u16 vid;
				u16 temp;
				u8 return_len;

				//u8 uid[] = {0x57,0x0a,0x00,0x4f,0x00,0x29};

				temp = *ptr++;
				temp <<= 8;
				temp += *ptr++;
				domain_id = temp;

				temp = *ptr++;
				temp <<= 8;
				temp += *ptr++;
				vid = temp;

				return_len = acp_req_by_vid(NULL, domain_id, vid, ptr, rest_rec_bytes-4, ptw, 20);
				out_buffer_len += return_len;
				break;
			}
			else if(cmd==0x4C && rest_rec_bytes>= 2)
			{
				u16 domain_id;
				u16 temp;

				temp = *ptr++;
				temp <<= 8;
				temp += *ptr++;
				domain_id = temp;
			
				acp_domain_broadcast(domain_id,ptr,rest_rec_bytes-1);
				break;
			}
			else if(cmd==0x4D && rest_rec_bytes>= 7)
			{
				u16 temp;
				u16 domain_id;
				u16 start_vid;
				u16 end_vid;
				u8 return_len;

				temp = *ptr++;
				temp <<= 8;
				temp += *ptr++;
				domain_id = temp;

				temp = *ptr++;
				temp <<= 8;
				temp += *ptr++;
				start_vid = temp;
				
				temp = *ptr++;
				temp <<= 8;
				temp += *ptr++;
				end_vid = temp;
			
				return_len = acp_req_mc(domain_id, start_vid, end_vid, ptr, rest_rec_bytes-6, NULL, (end_vid-start_vid) * 20);
				out_buffer_len += return_len;
				break;
			}
			
			else if(cmd==0xBD && rest_rec_bytes==1)		//0xBDBD, 搜索根节点
			{
				u8 new_nodes = 0;
				u8 total_nodes = 0;
				EBC_BROADCAST_STRUCT es;


				/* Reset topology */
				build_network_by_step(0, 0, BUILD_RESTART);	//执行一次即三相均重置

				es.phase = 0;
				while((es.bid & 0x0F)==0)
				{
					es.bid = (u8)(rand());
				}
				es.xmode = 0x10;
				es.rmode = 0x10;
				es.scan = 1;
				es.window = 4;
				es.mask = 0;
				es.max_identify_try =2;

				do
				{
					new_nodes = root_explore(&es);
					total_nodes += new_nodes;
				}while(new_nodes!=0);


				{
					u8 i;
					u8 target_uid[6];

					my_printf("Neibour Nodes:[");
					for(i=0;i<total_nodes;i++)
					{
						if(inquire_neighbor_uid_db(i, target_uid))
						{
							uart_send_asc(target_uid,6);
							if(i!=total_nodes-1)
							{
								my_printf(",");
							}
						}
					}
					my_printf("]\n");
				}
				break;
			}

			else if(cmd==0x02 && rest_rec_bytes==7)		//02 + uid + mask:读取当前参数
			{
				s16 current_parameter[2];
				STATUS status;

				status = call_vid_for_current_parameter(ptr, ptr[6], current_parameter);
				if(status)
				{
					my_printf("read succuess, u:%d, i:%d\r\n", current_parameter[0], current_parameter[1]);
				}
				else
				{
					my_printf("read fail\r\n");
				}
				break;
			}

			else if(cmd==0x82 && rest_rec_bytes >= 1)
			{
				set_ptp_comm_mode(*ptr);
				my_printf("new commmode %bX\r\n", *ptr);
				ptr++;
			}
			
			else if(cmd==0xCC && rest_rec_bytes >= 1)			//0xCCCC 自检
			{
				u8 nvr_data[] = {0x1E,0x12,0xD0,0x3B,0x30,0xF1,0x3C,0xC3,0xD7,0xE2,0x3D,0x3E,0x80,0xFB,0x30,0xC2,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x41,0xC0};
				s32 v,i;
				
				write_user_storage(nvr_data,sizeof(nvr_data));
				init_app_nvr_data();
				init_measure();
				
				v = measure_current_v();
				i = measure_current_i();
				my_printf("v:%d,i:%d\n",v,i);

				{
					/* 测试Dll.Diag */
					DLL_SEND_STRUCT xdata dds;
					ARRAY_HANDLE buffer;
					u8 uid[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
	
					mem_clr(&dds,sizeof(DLL_SEND_STRUCT),1);
					dds.phase = 0;
					dds.target_uid_handle = uid;
					ptr+=6;
					dds.xmode = 0x10;
					dds.rmode = 0x10;
					dds.prop = BIT_DLL_SEND_PROP_DIAG | BIT_DLL_SEND_PROP_REQ_ACK;
					dds.prop |= BIT_DLL_SEND_PROP_SCAN;
	
					buffer = OSMemGet(MINOR);
					if(buffer != NULL)
					{
						if(dll_diag(&dds, buffer))
						{
							u8 i;
	//						uart_send_asc(buffer+1,buffer[0]);
							my_printf("\r\nTarget:");
							uart_send_asc(dds.target_uid_handle,6);
							my_printf("\r\n------------------------------------------\r\n\tDownlink\t\tUplink\t\r\n");
							my_printf("Diag\tSS(dbuv)\tSNR(dB)\tSS(dbuv)\tSNR(dB)\r\n");
							for(i=0;i<4;i++)
							{
								my_printf("CH%bu\t%bd\t%bd",i,buffer[i*2+1],buffer[i*2+2]);
								my_printf("\t%bd\t%bd\r\n",buffer[i*2+9],buffer[i*2+10]);
							}
							
							
							my_printf("\r\n");
						}
						else
						{
							my_printf("Diag Fail!\r\n");
						}
						OSMemPut(MINOR,buffer);
					}
				}
				break;
			}

			else
			{
				uart_rcv_resume();
				break;
			}
		}
		uart_send_asc(out_buffer, out_buffer_len);
	}
}
#endif

