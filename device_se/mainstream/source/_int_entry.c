/******************** (C) COPYRIGHT 2016 Lv Haifeng ********************
* File Name          : _int_entry.c
* Author             : Lv Haifeng
* Version            : V2.0
* Date               : 2016-05-13
* Description        : unified INT entry, hardware proc(clear flag, etc.)
						specified INT sources were defined in stm32f0xx_it.c.
						software proc details were proc in individual c source file
						ͳһ���ж���ڴ���, ��������״̬λ, �ָ��жϼĴ�����
						������ж�Դ������stm32f0xx_it.c.
						��������������ڷֱ��c�ļ���, timer.c, usart.c, phy.c...
						���ļ��Ǿ����Ӳ�������֮���һ��������м��, ��Ϊ���漰����Ӳ��
						, ����и��õĿ���ֲ��
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODECTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, BELLING INC. SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAINMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/
#include "powermesh_include.h"
#include "stm32f0xx.h"

/* Private function prototypes -----------------------------------------------*/
void bad_usart_int_proc(USART_TypeDef* bad_usart_port);

/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : timer_int_entry
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void timer_int_entry(TIM_TypeDef * timer)
{
    if (TIM_GetITStatus(timer, TIM_IT_Update) != RESET)	//����ж�
	{
		TIM_ClearITPendingBit(timer, TIM_IT_Update);		//����жϱ�־λ
        timer_int_svr();
    }
}

/*******************************************************************************
* Function Name  : usart_int_entry
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void usart_int_entry(USART_TypeDef * usart)
{
	u8 byte_data;

	if(USART_GetITStatus(usart, USART_IT_RXNE) != RESET)
	{
		byte_data = USART_ReceiveData(usart);
		debug_uart_rcv_int_svr(byte_data);
	}
	else
	{
		bad_usart_int_proc(usart);
	}
}

/*******************************************************************************
* Function Name  : phy_int_entry
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void phy_int_entry(uint32_t exti_line)
{
	if(EXTI_GetITStatus(exti_line) != RESET)
	{
		EXTI_ClearITPendingBit(exti_line);
		_phy_rcv_obj[0].phase = 0;				// refresh phase, to prevent ram byte damaged
		phy_rcv_int_svr(&_phy_rcv_obj[0]);;
	}
}

/*******************************************************************************
* Function Name  : bad_usart_int_proc
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void bad_usart_int_proc(USART_TypeDef* bad_usart_port)
{
	my_printf("error int:");
	if(USART_GetITStatus(bad_usart_port, USART_IT_PE) != RESET)
	{
		my_printf("USART_IT_PE\r\n");
		USART_ClearITPendingBit(bad_usart_port,USART_IT_PE);
	}
	else if(USART_GetITStatus(bad_usart_port, USART_IT_TXE) != RESET)
	{
		my_printf("USART_IT_TXE\r\n");
		USART_ClearITPendingBit(bad_usart_port,USART_IT_TXE);
	}
	else if(USART_GetITStatus(bad_usart_port, USART_IT_TC) != RESET)
	{
		my_printf("USART_IT_TC\r\n");
		USART_ClearITPendingBit(bad_usart_port,USART_IT_TC);
	}
//	else if(USART_GetITStatus(bad_usart_port, USART_IT_RXNE) != RESET)
//	{
//		my_printf("USART_IT_RXNE\r\n");
//		USART_ClearITPendingBit(bad_usart_port,USART_IT_RXNE);
//	}
	else if(USART_GetITStatus(bad_usart_port, USART_IT_IDLE) != RESET)
	{
		my_printf("USART_IT_IDLE\r\n");
		USART_ClearITPendingBit(bad_usart_port,USART_IT_IDLE);
	}
	else if(USART_GetITStatus(bad_usart_port, USART_IT_LBD) != RESET)
	{
		my_printf("USART_IT_LBD\r\n");
		USART_ClearITPendingBit(bad_usart_port,USART_IT_LBD);
	}
	else if(USART_GetITStatus(bad_usart_port, USART_IT_CTS) != RESET)
	{
		my_printf("USART_IT_CTS\r\n");
		USART_ClearITPendingBit(bad_usart_port,USART_IT_CTS);
	}
	else if(USART_GetITStatus(bad_usart_port, USART_IT_ORE) != RESET)
	{
		my_printf("USART_IT_ORE\r\n");
		USART_ClearITPendingBit(bad_usart_port,USART_IT_ORE);
	}
	else if(USART_GetITStatus(bad_usart_port, USART_IT_NE) != RESET)
	{
		my_printf("USART_IT_NE\r\n");
		USART_ClearITPendingBit(bad_usart_port,USART_IT_NE);
	}
	else if(USART_GetITStatus(bad_usart_port, USART_IT_FE) != RESET)
	{
		my_printf("USART_IT_FE\r\n");
		USART_ClearITPendingBit(bad_usart_port,USART_IT_FE);
	}
	else
	{
		if (USART_GetFlagStatus(bad_usart_port, USART_FLAG_ORE) != RESET)//ע�⣡����ʹ��if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)���ж�
		{
#if CPU_TYPE==CPU_STM32F030C8
			USART_ClearITPendingBit(bad_usart_port,USART_IT_ORE);
#endif
			USART_ReceiveData(bad_usart_port);						//stm32f030 ȥ��־, ������, ����������������
			my_printf("over run\r\n");
		}
	}
}

/*******************************************************************************
* Function Name  : dma_tc_svr
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
#define DEBUG_DMA_FLAG									(DMA1_FLAG_TC4|DMA1_FLAG_HT4|DMA1_FLAG_GL4)
void dma_tc_svr(void)
{
	DMA_ClearFlag(DEBUG_DMA_FLAG);
	dma_uart_start();
}

