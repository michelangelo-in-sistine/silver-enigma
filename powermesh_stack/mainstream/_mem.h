/******************** (C) COPYRIGHT 2016 ***************************************
* File Name          : mem.h
* Author             : Lv Haifeng
* Version            : V1.0.3
* Date               : 10/26/2012
* Description        : Dynamic memory allocation, based on UCOS dynamic memory allocation scheme
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODECTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, BELLING INC. SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAINMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _MEM_H
#define _MEM_H

/* Public define ------------------------------------------------------------*/

/* Public datatype ------------------------------------------------------------------*/
typedef enum
{
	MINOR,
	SUPERIOR
}OS_MEM_TYPE;

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
void init_mem(void);
void * OSMemGet(OS_MEM_TYPE type) reentrant;
void OSMemPut(OS_MEM_TYPE type, void * pblk) reentrant;

u8 check_available_mem(OS_MEM_TYPE type);

#endif

