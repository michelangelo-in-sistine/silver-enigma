/******************** (C) COPYRIGHT 2012 Belling Inc. ********************
* File Name          : rscodec.c
* Author             : Lv Haifeng
* Version            : V1.0.3
						2008-08-28 Draft Ver. By Lv Haifeng
						2012-09-11 Imported to ARM STM32 Platform, By Lv Haifeng
* Date               : 10/29/2012
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
//#include "compile_define.h"
//#include "powermesh_datatype.h"
//#include "powermesh_config.h"
//#include "hardware.h"
#include "powermesh_include.h"

#ifdef USE_RSCODEC
/*******************************************************************************
* Function Name  : rsencode_vec()
* Description    : 对一组字节数组做RS编码, 编码后的字节长度为floor(len/10)*20 + (mod(len,10)>0)*(mod(len,10)+10)
					编码后的数据仍存储在原数组空间, 因此要保证内存空间足够, 程序内不检查越界, 由调用程序确保
					由于数据类型为unsigned char, 可表示字节长度小于255, 因此需要保证未编码长度字节len <= 125;
* Input          : 首字节地址, 数据长度
* Output         : None
* Return         : 编码后数据长度
*******************************************************************************/
BASE_LEN_TYPE rsencode_vec(u8 xdata * pt, BASE_LEN_TYPE len)
{
	BASE_LEN_TYPE ti,tj,i;
	BASE_LEN_TYPE xdata pi,pj;
	BASE_LEN_TYPE xdata final_len;
	
	if (len>CFG_RS_ENCODE_LENGTH)
	{
		return 0;
	}
	
	pi = 0;
	pj = 0;
	while((pi+10)<len)
	{
		pi += 10;
		pj += 20;
	}
	
	// encode the last section
	EXT_ADR = ADR_RS_CTRL;
	EXT_DAT = 0x80;
	EXT_ADR = ADR_RS_DATA0;
	ti = pi;
	tj = pj ; 
	while(ti<len)
	{
		EXT_DAT = pt[ti];
		pt[tj++] = pt[ti++];
		EXT_ADR++;
	}
	EXT_ADR = ADR_RS_CTRL;
	EXT_DAT = 0x40;
	while(!(EXT_DAT & 0x10));		// wait until coding ends
	EXT_ADR = ADR_RS_REDU0;
	for(ti=0;ti<10;ti++)
	{
		pt[tj++] = EXT_DAT;
		EXT_ADR++;
	}
	final_len = tj;
	
	// encode prior section
	while(pi!=0)
	{
		ti = pi-10;
		tj = pj-20;
		pi = ti;
		pj = tj;


		EXT_ADR = ADR_RS_CTRL;
		EXT_DAT = 0x80;
		EXT_ADR = ADR_RS_DATA0;
		for(i=0;i<10;i++)
		{
			EXT_DAT = pt[ti];
			pt[tj++] = pt[ti++];
			EXT_ADR++;
		}
		EXT_ADR = ADR_RS_CTRL;
		EXT_DAT = 0x40;
		while(!(EXT_DAT & 0x10));		// wait until coding ends
		EXT_ADR = ADR_RS_REDU0;
		for(ti=0;ti<10;ti++)
		{
			pt[tj++] = EXT_DAT;
			EXT_ADR++;
		}
	}
	return final_len;
}

/*******************************************************************************
* Function Name  : rsdecode_vec()
* Description    : 对RS编码后的向量解码, 解码后向量仍存储在原空间
* Input          : 首字节地址, 数据长度
* Output         : None
* Return         : 解码后字节长度/如果字节长度不符, 返回原长度
*******************************************************************************/
BASE_LEN_TYPE rsdecode_vec(u8 xdata * pt, BASE_LEN_TYPE len)
{
	BASE_LEN_TYPE ti,tj;
	BASE_LEN_TYPE i;
	BASE_LEN_TYPE temp;

	if(len>CFG_RS_DECODE_LENGTH)
	{
		return 0;
	}
	else
	{
		temp = len;				// length check, correct len should be mod(len,20) = 0 or between [11,19]
		while(temp>20)
		{
			temp -= 20;
		}
		if(temp<=10)
		{
			//return len;			// 不符合RS编码长度, 返回原长度(不做修改)
			return 0;			// 2014-09-25 返回原长度将导致有些无编码的powermesh分组能被有编码的powermesh接收响应
		}
	}
	
	ti=0;
	tj=0;
	
	if(len>20)
	{
		while(ti<= (len-20))
		{
			EXT_ADR = ADR_RS_CTRL;
			EXT_DAT = 0x80;					// clr rs buffer
			EXT_ADR = ADR_RS_DATA0;
			for(i=0;i<20;i++)				// fill data
			{
				EXT_DAT = pt[ti++];
				EXT_ADR++;
			}
			EXT_ADR = ADR_RS_CTRL;
			EXT_DAT = 0x20;
			while(!(EXT_DAT & 0x08));		// wait until decode ends
	
			EXT_ADR = ADR_RS_DATA0;			// pick decoded data
			for(i=0;i<10;i++)
			{
				if(tj<CFG_RS_DECODE_LENGTH)
				{
					pt[tj++] = EXT_DAT;
					EXT_ADR++;
				}
			}
		}
	}
	
	if(ti<len)
	{
		EXT_ADR = ADR_RS_CTRL;
		EXT_DAT = 0x80;					// clr rs buffer
		EXT_ADR = ADR_RS_DATA0;			// fill the last data
		temp = 0;
		while(ti < (len-10))
		{
			EXT_DAT = pt[ti++];
			EXT_ADR++;
			temp++;
		}
		EXT_ADR = ADR_RS_REDU0;
		for(i=0;i<10;i++)
		{
			EXT_DAT = pt[ti++];
			EXT_ADR++;
		}
		EXT_ADR = ADR_RS_CTRL;
		EXT_DAT = 0x20;
		while(!(EXT_DAT & 0x08));		// wait until decode ends

		EXT_ADR = ADR_RS_DATA0;			// pick decoded data
		for(i=0;i<temp;i++)
		{
			if(tj<CFG_RS_DECODE_LENGTH)
			{
				pt[tj++] = EXT_DAT;
				EXT_ADR++;
			}
		}
	}
	return tj;
}


/*******************************************************************************
* Function Name  : rsencode_recover()
* Description    : 对RS编码后的向量恢复, 仍存储在原空间, 用于SCAN类型发送时的数据重填
* Input          : 首字节地址, 数据长度
* Output         : None
* Return         : 恢复后数据长度
*******************************************************************************/
BASE_LEN_TYPE rsencode_recover(u8 xdata * pt, BASE_LEN_TYPE len)
{
	BASE_LEN_TYPE ti,tj;
	BASE_LEN_TYPE i,temp;

	if(len>CFG_RS_DECODE_LENGTH)
	{
		return 0;
	}
		
	ti=0;
	tj=0;
	while(ti<= (len-20))
	{
		for(i=0;i<10;i++)
		{
			pt[tj++] = pt[ti++];
		}
		ti += 10;
	}
	temp = len-ti;		// temp为最后一组数据的长度
	if((temp>0) && (temp<=10))
	{
		return 0;
	}
	else
	{
		if(ti<len)
		{
//			for(i=0;i<=9;i++)
//			{
//				pt[len-i-1+20-temp] = pt[len-i-1];
//			}
//			for(i=0;i<=20-temp-1;i++)
//			{
//				pt[ti+temp-10+i] = 0;
//			}
			for(i=0;i<(temp-10);i++)
			{
				pt[tj++] = pt[ti++];
			}
		}
	}
	return tj;
}

/*******************************************************************************
* Function Name  : encoded_len()
* Description    : 对RS编码后的数据长度
* Input          : 编码前长度
* Output         : None
* Return         : 编码后长度
*******************************************************************************/
BASE_LEN_TYPE rsencoded_len(BASE_LEN_TYPE len)
{
	BASE_LEN_TYPE encoded_len = 0;
	while(len>=10)
	{
		len -= 10;
		encoded_len += 20;
	}
	if(len>0)
	{
		encoded_len += (len+10);
	}
	return encoded_len;
}

#endif


/*******************************************************************************
* Function Name  : calc_crc_hw
* Description    : use crc register hardware to calculate a u8 vector into crc-16 checksum
					bl6810 only
* Input          : Head address, length
* Output         : u16 crc value
* Return         : u16 crc value
*******************************************************************************/
u16 calc_crc(ARRAY_HANDLE pt, BASE_LEN_TYPE len) reentrant
{
	unsigned int crc;
	BASE_LEN_TYPE i;

	ENTER_CRITICAL();
	EXT_ADR = ADR_CRC_INIT;
	EXT_DAT = 0x01;
	EXT_ADR = ADR_CRC_DATA;
	for(i=0;i<len;i++)
	{
		EXT_DAT = *(pt++);
	}
	EXT_ADR = ADR_CRC_HIGH;
	*(unsigned char *)(&crc) = EXT_DAT;
	EXT_ADR = ADR_CRC_LOW;
	*((unsigned char *)(&crc)+1) = EXT_DAT;
#ifdef CRC_ISOLATED
	crc ^= CRC_DISTURB;
#endif
	EXIT_CRITICAL();
	return crc;
}

