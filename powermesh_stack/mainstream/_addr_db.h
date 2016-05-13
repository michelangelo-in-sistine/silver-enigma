/******************** (C) COPYRIGHT 2012 Belling Inc. ********************
* File Name          : addr_db.h
* Author             : Lv Haifeng
* Version            : V2.0
* Date               : 2013-03-15
* Description        : 
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODECTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, BELLING INC. SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAINMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _ADDR_DB_H
#define _ADDR_DB_H

/* Includes ------------------------------------------------------------------*/

/* Exported constants & macro --------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
typedef struct
{
	u8 uid[6];
	u8 xmode_rmode;
	u8 expire_cnt;
}xdata ADDR_ITEM_STRUCT;

/* Exported variants ------------------------------------------------------- */

/* Exported functions ------------------------------------------------------- */
void init_addr_db(void);
ADDR_REF_TYPE add_addr_db(UID_HANDLE uid);
STATUS update_addr_db(ADDR_REF_TYPE addr_ref, u8 xmode, u8 rmode);
UID_HANDLE inquire_addr_db(ADDR_REF_TYPE addr_ref);
ADDR_REF_TYPE inquire_addr_db_by_uid(ARRAY_HANDLE uid);
u8 inquire_addr_db_xmode(ADDR_REF_TYPE addr_ref);
u8 inquire_addr_db_rmode(ADDR_REF_TYPE addr_ref);
RESULT check_addr_ref_validity(ADDR_REF_TYPE addr_ref);
void notify_expired_addr(ADDR_REF_TYPE expired_addr);
#endif






