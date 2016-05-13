/******************** (C) COPYRIGHT 2016 ***************************************
* File Name          : phy.h
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
#ifndef _PHY_H
#define _PHY_H

//#define DEBUG_PHY
/* Includes ------------------------------------------------------------------*/

/* Exported constants & macro --------------------------------------------------------*/
#define BIT_PHY_SEND_PROP_SCAN		0x08
#define BIT_PHY_SEND_PROP_SRF		0x04
#define BIT_PHY_SEND_PROP_ESF		0x02
#define BIT_PHY_SEND_PROP_ACUPDATE	0x01

#define GET_PHY_HANDLE(pt)	&_phy_rcv_obj[pt->phase]		//得到同相的PHY对象
#define GET_PPDU(pt)		_phy_rcv_obj[pt->phase].phy_rcv_data


/* Exported types ------------------------------------------------------------*/
struct PHY_RCV_STRUCT;

/* Exported variants ------------------------------------------------------- */
extern PHY_RCV_STRUCT xdata _phy_rcv_obj[];

/* Exported functions ------------------------------------------------------- */
void init_phy(void);
void init_bl6810_plc(void);
void phy_rcv_int_svr(PHY_RCV_HANDLE pp);
void phy_rcv_proc(PHY_RCV_HANDLE pp);

SEND_ID_TYPE phy_send(PHY_SEND_HANDLE pps) reentrant;
SEND_STATUS inquire_send_status(SEND_ID_TYPE sid);
PHY_RCV_HANDLE phy_rcv(void);
void phy_rcv_resume(PHY_RCV_HANDLE pp) reentrant;
STATUS wait_until_send_over(SEND_ID_TYPE sid);
void check_and_clear_send_status(void);

#endif





