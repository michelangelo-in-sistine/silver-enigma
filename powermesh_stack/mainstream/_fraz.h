/******************** (C) COPYRIGHT 2012  ********************
* File Name          : fraz.h
* Author             : Lv Haifeng
* Version            : V2.0
* Date               : 2016-08-10
* Description        : a fraze data database
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODECTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, BELLING INC. SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAINMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

void init_fraz(void);
STATUS req_fraz_record(u8 feature_code);
STATUS write_fraz_record(u8 feature_code, u8 mask, s16 value);
STATUS read_fraz_record(u8 feature_code, u8 mask, s16 * pt_data);


