/******************** (C) COPYRIGHT 2012 Belling Inc. ********************
* File Name          : ebc.h
* Author             : Lv Haifeng
* Version            : V2.0
* Date               : 2013-03-14
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
#ifndef _EBC_H
#define _EBC_H

/* Includes ------------------------------------------------------------------*/

/* Exported constants & macro --------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/


/* Exported variants ------------------------------------------------------- */
#ifdef DEBUG_EBC
extern u8 xdata debug_ebc_fixed_window;
#endif

/* Exported functions ------------------------------------------------------- */
void init_ebc(void);
u8 ebc_broadcast(EBC_BROADCAST_HANDLE pt_ebc);
u8 ebc_identify_transaction(EBC_BROADCAST_HANDLE pt_ebc, u8 num_nodes);

u8 get_neighbor_uid_db_usage(void);
STATUS delete_neighbor_uid_db(u8 index_to_delete);

void set_build_id(u8 phase, u8 new_build_id);
u8 get_build_id(u8 phase);

void ext_diag_protocol_response(PHY_RCV_HANDLE pp);
STATUS inquire_neighbor_uid_db(u8 index, ARRAY_HANDLE ptw);
u8 phy_acps_compare(PHY_RCV_STRUCT * phy);

#endif






