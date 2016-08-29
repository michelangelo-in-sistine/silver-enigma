/******************** (C) COPYRIGHT 2012 Belling Inc. ********************
* File Name          : nvr.h
* Author             : Lv Haifeng
* Version            : V1.0.0
* Date               : 2015-07-28
* Description        : NVR Access
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODECTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, BELLING INC. SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAINMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _NVR_H
#define _NVR_H


unsigned char ifp_nvr_read(unsigned char addr_h, unsigned char addr_l);
void ifp_nvr_write(unsigned char addr_h, unsigned char addr_l, unsigned char dt);
STATUS erase_nvr(void);
STATUS read_nvr(unsigned int nvr_addr, unsigned char * data_addr, u16 data_len);
STATUS write_nvr(unsigned int nvr_addr, unsigned char * data_addr, u16 data_len);
#endif