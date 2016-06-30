/******************** (C) COPYRIGHT 2012 Belling Inc. ********************
* File Name          : mac.h
* Author             : Lv Haifeng
* Version            : V1.0.3
* Date               : 2015-04-03
* Description        : 6810 media access layer definition
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODECTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, BELLING INC. SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAINMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _MAC_H
#define _MAC_H

/* Includes ------------------------------------------------------------------*/

/* Exported constants & macro --------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* Exported variants ------------------------------------------------------- */

/* Exported functions ------------------------------------------------------- */
void init_mac(void);
void set_mac_nav_timing_from_esf(PHY_RCV_HANDLE pp);
SEND_ID_TYPE send_esf(u8 phase, u16 nav_value);
STATUS config_esf_xmode(COMM_MODE_TYPE comm_mode);
void mac_layer_proc(void);
STATUS declare_channel_occupation(SEND_ID_TYPE dll_sid, u32 wait_time_after_sending);
BOOL is_channel_busy(void);

/* Nearby Nodes Monitoring */
STATUS update_nearby_active_nodes(UID_HANDLE src_uid);
u8 get_nearby_active_nodes_num(void);
u8 acquire_nearby_active_nodes_set(u8 * buffer);
void set_mac_parameter(u16 mac_stick_base_value, u16 mac_stick_anneal_value);

void declare_channel_monopoly(u32 over_time_stick);
void cancel_channel_monopoly(void);



#endif







