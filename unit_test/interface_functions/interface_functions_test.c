#include "_powermesh_datatype.h"
#include "test_func.h"
#include "stdio.h"

void _my_printf(const char code * fmt, ...) reentrant;
void _uart_send(u8 * ptr, u16 len);
void _uart_send_asc(ARRAY_HANDLE pt, u16 len) reentrant;
extern RESULT check_format(u8 xdata * ptr, u16 total_rec_bytes);

u8 test1[] = {'<','1','2','3','4','5','6','7','8','9','0','A','B','C','D','E','F','>'};
u8 test2[] = {'<','1','2','3','4','5','6','7','8','9','0','A','B','C','D','E','G','>'};	//illegal character
u8 test3[] = {'<','1','2','3','4','5','6','7','8','9','0','A','B','C','D','E','>'};		//odd number;
u8 test4[] = {'0','1','2','3','4','5','6','7','8','9','0','A','B','C','D','E','F','>'};	// no <
u8 test5[] = {'<','0','1','2','3','4','5','6','7','8','9','0','A','B','C','D','E','F'};	// no >





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

	if(total_rec_bytes & 0x01)		//total_rec_bytes should be even number
	{
		_my_printf("odd number\n");
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
				_my_printf("illegal character\n");
				return WRONG;
			}
		}
	}

	if(*ptr == '>')
	{
		return CORRECT;
	}
	
	_my_printf("error start/end character\n");

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

int main(void)
{
	assert(check_format(test1,sizeof(test1)) == CORRECT);
	assert(check_format(test2,sizeof(test2)) == WRONG);
	assert(check_format(test3,sizeof(test3)) == WRONG);
	assert(check_format(test4,sizeof(test4)) == WRONG);
	assert(check_format(test5,sizeof(test5)) == WRONG);
	
	{
		u16 new_len;
		printf("before convert\n");
		_uart_send(test1, sizeof(test1));
		
		printf("\nafter convert\n");
		new_len = hexstr2u8(test1, sizeof(test1));
		_uart_send_asc(test1,new_len);
	}

	printf("unit test finish");

	return 0;
}

