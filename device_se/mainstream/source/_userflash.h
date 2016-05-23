/******************** (C) COPYRIGHT 2016 ***************************************
* File Name          : userflash.h
* Author             : Lv Haifeng
* Version            : V 1.6.0
* Date               : 2016-05-16
* Description        : 
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODECTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, BELLING INC. SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAINMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/
#ifndef _USERFLASH_H
#define _USERFLASH_H

#include "stm32f0xx.h"

void erase_user_storage(void);
uint16_t read_user_storage(unsigned char * read_buffer, unsigned short read_len);
uint16_t write_user_storage(uint8_t * write_buffer, uint16_t write_len);


#endif
