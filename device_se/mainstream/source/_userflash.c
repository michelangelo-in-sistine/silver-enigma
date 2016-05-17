/******************** (C) COPYRIGHT 2016 ***************************************
* File Name          : flashprog.c
* Author             : Lv Haifeng
* Version            : V2.0
* Date               : 2016-05-12
* Description        : ��stm32f030���ڲ�flash���һ��page��Ϊ�û����ݷǻӷ����ݴ洢��(2k)
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODECTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, BELLING INC. SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAINMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx.h"

/* Private typedef -----------------------------------------------------------*/
typedef enum {FAILED = 0, PASSED = !FAILED} TestStatus;
/* Private define ------------------------------------------------------------*/
#ifdef STM32F072
 #define FLASH_PAGE_SIZE         ((uint32_t)0x00000800)   /* FLASH Page Size */
 #define FLASH_USER_START_ADDR   ((uint32_t)0x08009000)   /* Start @ of user Flash area */
 #define FLASH_USER_END_ADDR     ((uint32_t)0x08020000)   /* End @ of user Flash area */
#elif defined (STM32F091)
 #define FLASH_PAGE_SIZE         ((uint32_t)0x00000800)   /* FLASH Page Size */
 #define FLASH_USER_START_ADDR   ((uint32_t)0x08009000)   /* Start @ of user Flash area */
 #define FLASH_USER_END_ADDR     ((uint32_t)0x08040000)   /* End @ of user Flash area */
#else
 #define FLASH_PAGE_SIZE         ((uint32_t)0x00000400)   /* FLASH Page Size */
 #define FLASH_USER_START_ADDR   ((uint32_t)0x0800fc00)   /* Start @ of user Flash area */
 #define FLASH_USER_END_ADDR     ((uint32_t)0x08010000)   /* End @ of user Flash area */
#endif /* STM32F072 */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint32_t EraseCounter = 0x00, Address = 0x00;
__IO FLASH_Status FLASHStatus = FLASH_COMPLETE;
__IO TestStatus MemoryProgramStatus = PASSED;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */

void erase_user_storage(void)
{
	uint32_t EraseCounter,NbrOfPage;
	
	FLASH_Unlock();

	/* Clear pending flags (if any) */  
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR); 

	/* Define the number of page to be erased */
	NbrOfPage = (FLASH_USER_END_ADDR - FLASH_USER_START_ADDR) / FLASH_PAGE_SIZE;

	/* Erase the FLASH pages */
	for(EraseCounter = 0; (EraseCounter < NbrOfPage) && (FLASHStatus == FLASH_COMPLETE); EraseCounter++)
	{
		if (FLASH_ErasePage(FLASH_USER_START_ADDR + (FLASH_PAGE_SIZE * EraseCounter))!= FLASH_COMPLETE)
		{
		 /* Error occurred while sector erase. 
		     User can add here some code to deal with this error  */
			//my_printf("flash erase error!\n");
		}
	}
}

/*******************************************************************************
* Function Name  : read_user_storage
* Description    : ��flash��
					
* Input          : None
* Output         : None
* Return         : ���ݰ�����
*******************************************************************************/
uint16_t read_user_storage(unsigned char * read_buffer, unsigned short read_len)
{
	uint16_t i;
	__IO uint8_t * ptr;

	ptr = (__IO uint8_t *)(FLASH_USER_START_ADDR);

	for(i=0;i<read_len;i++)
	{
		read_buffer[i] = *ptr++;
	}

	return read_len;
}

/*******************************************************************************
* Function Name  : write_user_storage
* Description    : stm32f030ÿ����С��̵�λΪ16bit, д��ʱ���ֽڴ���flash�͵�ַ, ���ֽڴ�flash�ߵ�ַ
					���д��ʱ�ڴ�������2�ֽ�Ϊ��Ԫ��֯д��, �͵�ַ����ֽ�,�ߵ�ַ����ֽ�
					��֤����ʱ��flash���ڴ��������Ϳ���.
					
					�����Ҫ�洢������Ϊ�����ֽ���, ��8λ��FF
					
* Input          : None
* Output         : None
* Return         : ���ݰ�����
*******************************************************************************/
uint16_t write_user_storage(uint8_t * write_buffer, uint16_t write_len)
{
	uint16_t i;
	uint16_t write_halfword;

	if(write_len > (FLASH_USER_END_ADDR - FLASH_USER_START_ADDR))
	{
//		my_printf("out of user flash writable range!\n");
		return 0;
	}

	FLASH_Unlock();

	/* Clear pending flags (if any) */  
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR); 

	for(i=0; i<write_len; i+=2)
	{
		write_halfword = write_buffer[i];
		if(i == write_len-1)
		{
			write_halfword |= 0xFF00;
		}
		else
		{
			write_halfword |= (write_buffer[i+1]<<8);
		}

		if(FLASH_ProgramHalfWord(FLASH_USER_START_ADDR+i,write_halfword) != FLASH_COMPLETE)
		{
//			my_printf("write flash error\n");
			write_len = 0;
			break;
		}
	}
	FLASH_Lock();
	return write_len;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

