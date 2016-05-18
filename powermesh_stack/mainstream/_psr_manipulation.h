/******************** (C) COPYRIGHT 2012 Belling Inc. ********************
* File Name          : psr_manipulation.h
* Author             : Lv Haifeng
* Version            : V2.0
* Date               : 2013-03-18
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
#ifndef _PSR_MANIPULATION_H
#define _PSR_MANIPULATION_H

/* Includes ------------------------------------------------------------------*/

/* Exported constants & macro --------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
typedef enum
{
	DISALLOW = 0,
	ALLOW = !DISALLOW
}ENABILITY_TYPE;				// DISABLE, ENABLE已被库.h占用

/* Exported variants ------------------------------------------------------- */

/* Exported functions ------------------------------------------------------- */
// CC Exclusive Functions
void init_psr_mnpl(void);
void reset_psr_mnpl(u8 phase);
u8 get_psr_index(u8 phase);
STATUS psr_setup(u8 phase, u16 pipe_id, ARRAY_HANDLE script, u16 script_len);
u16 psr_patrol(u8 phase, u16 pipe_id, ARRAY_HANDLE return_buffer);

STATUS update_pipe_mnpl_db(u8 phase, u16 pipe_id, u8 * script, u16 script_len);
void notify_psr_mnpl_db_expired_addr(ADDR_REF_TYPE addr_ref);

PIPE_REF inquire_pipe_mnpl_db(u16 pipe_id);
PIPE_REF inquire_pipe_mnpl_db_by_uid(u8 * uid);
u8 get_pipe_phase(u16 pipe_id);
UID_HANDLE inquire_pipe_recepient(u16 pipe_id);
u16 get_available_pipe_id(void);
u16 get_path_script_from_pipe_mnpl_db(u16 pipe_id, ARRAY_HANDLE script_buffer);

STATUS psr_transaction_core(u16 pipe_id, ARRAY_HANDLE nsdu, BASE_LEN_TYPE nsdu_len, u32 expiring_sticks, 
 						ARRAY_HANDLE return_buffer, ERROR_STRUCT * psr_err);				//无重试和修复
STATUS psr_transaction(u16 pipe_id, ARRAY_HANDLE nsdu, BASE_LEN_TYPE nsdu_len, u32 expiring_sticks, 
 						ARRAY_HANDLE return_buffer, AUTOHEAL_LEVEL autoheal_level); 						
u16 get_pipe_usage(void);
u16 get_pipe_id(PIPE_REF pipe_ref);
u8 inquire_pipe_stages(u16 pipe_id);


#endif






