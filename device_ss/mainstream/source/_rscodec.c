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
* Description    : ��һ���ֽ�������RS����, �������ֽڳ���Ϊfloor(len/10)*20 + (mod(len,10)>0)*(mod(len,10)+10)
					�����������Դ洢��ԭ����ռ�, ���Ҫ��֤�ڴ�ռ��㹻, �����ڲ����Խ��, �ɵ��ó���ȷ��
					������������Ϊunsigned char, �ɱ�ʾ�ֽڳ���С��255, �����Ҫ��֤δ���볤���ֽ�len <= 125;
* Input          : ���ֽڵ�ַ, ���ݳ���
* Output         : None
* Return         : ��������ݳ���
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
* Description    : ��RS��������������, ����������Դ洢��ԭ�ռ�
* Input          : ���ֽڵ�ַ, ���ݳ���
* Output         : None
* Return         : ������ֽڳ���/����ֽڳ��Ȳ���, ����ԭ����
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
			//return len;			// ������RS���볤��, ����ԭ����(�����޸�)
			return 0;			// 2014-09-25 ����ԭ���Ƚ�������Щ�ޱ����powermesh�����ܱ��б����powermesh������Ӧ
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
* Description    : ��RS�����������ָ�, �Դ洢��ԭ�ռ�, ����SCAN���ͷ���ʱ����������
* Input          : ���ֽڵ�ַ, ���ݳ���
* Output         : None
* Return         : �ָ������ݳ���
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
	temp = len-ti;		// tempΪ���һ�����ݵĳ���
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
* Description    : ��RS���������ݳ���
* Input          : ����ǰ����
* Output         : None
* Return         : ����󳤶�
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

