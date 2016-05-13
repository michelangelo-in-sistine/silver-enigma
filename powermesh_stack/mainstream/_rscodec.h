/******************** (C) COPYRIGHT 2016 ***************************************
* File Name          : rscodec.h
* Author             : Lv Haifeng
* Version            : V1.0.3
* Date               : 10/28/2012
* Description        : a customized (20,10) rs coder and decoder in GF(2^8)
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODECTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, BELLING INC. SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAINMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _RSCODEC_H
#define _RSCODEC_H

/* Includes ------------------------------------------------------------------*/

/* Exported constants & macro --------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* Exported variants ------------------------------------------------------- */

/* Exported functions ------------------------------------------------------- */
#ifdef USE_RSCODEC
extern BASE_LEN_TYPE rsencode_vec(u8 xdata * pt, BASE_LEN_TYPE len);
extern BASE_LEN_TYPE rsencode_recover(ARRAY_HANDLE pt, BASE_LEN_TYPE len);
extern BASE_LEN_TYPE rsdecode_vec(ARRAY_HANDLE pt, BASE_LEN_TYPE len);
extern BASE_LEN_TYPE rsencoded_len(BASE_LEN_TYPE len);
#endif

#endif



