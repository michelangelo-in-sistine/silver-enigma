#include "powermesh_include.h"
#include "stdarg.h"
#include "stdio.h"

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

#define ITM_Port8(n)    (*((volatile unsigned char *)(0xE0000000+4*n)))
#define ITM_Port16(n)   (*((volatile unsigned short*)(0xE0000000+4*n)))
#define ITM_Port32(n)   (*((volatile unsigned long *)(0xE0000000+4*n)))

#define DEMCR           (*((volatile unsigned long *)(0xE000EDFC)))
#define TRCENA          0x01000000


struct __FILE { int handle; /* Add whatever is needed */ };
FILE __stdout;
FILE __stdin;

int fputc(int ch, FILE *f) {
  if (DEMCR & TRCENA) {
    while (ITM_Port32(0) == 0);
    ITM_Port8(0) = ch;
  }
  return(ch);
}

void my_putchar(u8 x)
{
  if (DEMCR & TRCENA) {
    while (ITM_Port32(0) == 0);
    ITM_Port8(0) = x;
  }
}

#define MY_PRINTF_BYTE	0x01
#define MY_PRINTF_LONG	0x02
#define MY_PRINTF_MINUS 0x04
#define MY_PRINTF_HEX	0x08
#define MY_PRINTF_STR	0x10
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
void _my_printf(const char code * fmt, ...) reentrant
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

void __aeabi_assert(const char * s, const char * f, int line)
{
	printf("%s,%s,%d",s,f,line);
}

void _uart_send(u8 * ptr, u16 len)
{
	u16 i;
	
	for(i=0;i<len;i++)
	{
		my_putchar(*ptr++);
	}
}

void _uart_send_asc(ARRAY_HANDLE pt, u16 len) reentrant
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
	}
	
}
