/******************** (C) COPYRIGHT 2015 Belling Inc. ********************
* File Name          : nvr.c
* Author             : Lv Haifeng
* Version            : V1.0
* Date               : 2015-07-28
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
//#include "_nvr.h"

#define NVR_CAPACITY 512

/*******************************************************************************
* Function Name  : erase_nvr
* Description    : 
					
					
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS erase_nvr(void)
{
	ENTER_CRITICAL();
	IFP_ADR = ADR_NVR_ADDRL;
	IFP_DAT = 0x00;
	IFP_ADR = ADR_NVR_ADDRH;
	IFP_DAT = 0x00;
	IFP_ADR = ADR_NVR_CTRL;
	IFP_DAT = 0x2A; 
	EXIT_CRITICAL();
	return OKAY;
}

/*******************************************************************************
* Function Name  : read_nvr(unsigned char start_addr, unsigned char * data_addr, unsigned char data_len)
* Description    : 
					
					
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS read_nvr(unsigned int nvr_addr, unsigned char * data_addr, u16 data_len)
{
	u16 i;
	
	if(nvr_addr+data_len>NVR_CAPACITY)
	{
#ifdef DEBUG_DISP_INFO
		my_printf("out of nvr write range\r\n");
#endif
		return FAIL;
	}
	
	ENTER_CRITICAL();
	for(i=0;i<data_len;i++)
	{
		IFP_ADR = ADR_NVR_ADDRL;
		IFP_DAT = (u8)nvr_addr;
		IFP_ADR = ADR_NVR_ADDRH;
		IFP_DAT = (u8)(nvr_addr++>>8);
		IFP_ADR = ADR_NVR_CTRL;
		IFP_DAT = 0x10;
		IFP_ADR = ADR_NVR_RD;
		data_addr[i] = IFP_DAT;
	}
	EXIT_CRITICAL();

	return OKAY;
}



/*******************************************************************************
* Function Name  : write_nvr(unsigned char start_addr, unsigned char * data_addr, unsigned char data_len)
* Description    : 
					
					
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS write_nvr(unsigned int nvr_addr, unsigned char * data_addr, u16 data_len)
{
	u16 i;
	
	if(nvr_addr+data_len>NVR_CAPACITY)
	{
#ifdef DEBUG_DISP_INFO
		my_printf("out of nvr write range\r\n");
#endif
		return FAIL;
	}

	ENTER_CRITICAL();
	for(i=0;i<data_len;i++)
	{
		u8 value;
		value = data_addr[i];
		
		IFP_ADR = ADR_NVR_ADDRL;
		IFP_DAT = (u8)nvr_addr;
		IFP_ADR = ADR_NVR_ADDRH;
		IFP_DAT = (u8)(nvr_addr++>>8);
		IFP_ADR = ADR_NVR_WD;
		IFP_DAT = value;
		IFP_ADR = ADR_NVR_CTRL;
		IFP_DAT = 0x01;
	}
	
	EXIT_CRITICAL();

	return OKAY;
}

/*******************************************************************************
* Function Name  : check_nvr(unsigned char start_addr, unsigned char * data_addr, unsigned char data_len)
* Description    : 检查nvr内容与指定的内存内容, 
					
					
* Input          : 
* Output         : 一致返回CORRECT, 不一致返回WRONG
* Return         : 
*******************************************************************************/
RESULT check_nvr(unsigned int nvr_addr, unsigned char * data_addr, u16 data_len)
{
	u16 i;

	if(nvr_addr+data_len>NVR_CAPACITY)
	{
#ifdef DEBUG_DISP_INFO
		my_printf("out of nvr range\r\n");
#endif
		return FAIL;
	}
	
	ENTER_CRITICAL();
	for(i=0;i<data_len;i++)
	{
		IFP_ADR = ADR_NVR_ADDRL;
		IFP_DAT = (u8)nvr_addr;
		IFP_ADR = ADR_NVR_ADDRH;
		IFP_DAT = (u8)(nvr_addr++>>8);
		IFP_ADR = ADR_NVR_CTRL;
		IFP_DAT = 0x10;
		IFP_ADR = ADR_NVR_RD;
		if(*data_addr++ != IFP_DAT)
		{
			EXIT_CRITICAL();
			return WRONG;
			
		}
	}
	EXIT_CRITICAL();
	return CORRECT;
}

/*******************************************************************************
* Function Name  : write_user_storage
* Description    : 将数据写入nvr
					
* Input          : None
* Output         : None
* Return         : 数据包长度
*******************************************************************************/
u16 write_user_storage(u8 * write_buffer, u16 write_len)
{
	if(erase_nvr())
	{
		write_nvr(0, write_buffer, write_len);
		if(check_nvr(0,write_buffer, write_len))
		{
			return write_len;
		}
	}
	return 0;
}

/*******************************************************************************
* Function Name  : read_user_storage
* Description    : 
					
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
u16 read_user_storage(u8 * read_buffer, u16 read_len)
{
	if(read_nvr(0, read_buffer, read_len))
	{
		return read_len;
	}
	return 0;
}


