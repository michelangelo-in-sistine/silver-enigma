/******************** (C) COPYRIGHT 2016 ***************************************
* File Name          : PSR.h
* Author             : Lv Haifeng
* Version            : V 2.0
* Date               : 2013-03-16
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
#ifndef _PSR_H
#define _PSR_H

/* Exported constants & macro ------------------------------------------------*/
#define BIT_PSR_SEND_PROP_SETUP		0x80	//[setup patrol biway heal direction error index1 index0]
#define BIT_PSR_SEND_PROP_PATROL	0x40
#define BIT_PSR_SEND_PROP_BIWAY		0x20
#define BIT_PSR_SEND_PROP_HEAL 		0x10
#define BIT_PSR_SEND_PROP_DIRECTION	0x08
#define BIT_PSR_SEND_PROP_ERROR		0x04
#define BIT_PSR_SEND_PROP_INDEX		0x03

#define BIT_PSR_RCV_VALID			0x80		//psr_rcv_valid: [valid patrol biway uplink error index1 index0]
#define BIT_PSR_RCV_PATROL			0x40
#define BIT_PSR_RCV_BIWAY			0x20
#define BIT_PSR_RCV_UPLINK			0x08
#define BIT_PSR_RCV_ERROR			0x04
#define BIT_PSR_RCV_INDEX			0x03




#define BIT_PSR_CONF_SETUP			0x80		//This packet is a pipe setup packet
#define BIT_PSR_CONF_PATROL			0x40		//This packet is a patrol diagnose packet
#define BIT_PSR_CONF_BIWAY			0x20		//Link is bi-direction
#define BIT_PSR_CONF_SELFHEAL 		0x10		//Enable self-heal when ack communication failed
#define BIT_PSR_CONF_UPLINK			0x08		//1: uplink(from consignee to pipe setuper), 0:downlink(from setuper to consignee);
#define BIT_PSR_CONF_ERROR			0x04
#define BIT_PSR_CONF_INDEX			0x03

#define GET_NW_HANDLE(pt)	&_psr_rcv_obj[pt->phase]		//得到同相的PHY对象

/* Exported types ------------------------------------------------------------*/
typedef struct
{
	u16 pipe_info;				// [BIWAY ENDPOINT UPLINK 0 pipe_id(11:0)]	如果ENDPOINT, uplink决定自己在pipe中的位置, setup的一方永远为downlink, 被setup的一方永远为uplink
	ADDR_REF_TYPE downlink_next;
	ADDR_REF_TYPE uplink_next;
} PSR_PIPE_RECORD_STRUCT;

typedef PSR_PIPE_RECORD_STRUCT xdata * PSR_PIPE_RECORD_HANDLE;

/* Exported variants ------------------------------------------------------- */
extern PSR_RCV_STRUCT _psr_rcv_obj[];
extern DST_STACK_RCV_STRUCT _dst_rcv_obj[];

/* Exported functions ------------------------------------------------------- */
void init_psr(void);
u8 get_psr_index(u8 phase);

PSR_RCV_HANDLE psr_rcv(void);
void psr_rcv_resume(PSR_RCV_HANDLE pn) reentrant;
void nw_rcv_proc(PSR_RCV_HANDLE pn);
//PSR_ERROR_TYPE psr_send(PSR_SEND_HANDLE ppss) reentrant;
SEND_ID_TYPE psr_send(PSR_SEND_HANDLE ppss) reentrant;
PSR_PIPE_RECORD_HANDLE inquire_pipe_record_db(u16 pipe_id);

BOOL is_pipe_endpoint(u16 pipe_id);
BOOL is_pipe_endpoint_uplink(u16 pipe_id);

void notify_psr_expired_addr(ADDR_REF_TYPE expired_addr);

void dst_rcv_resume(DST_STACK_RCV_HANDLE pdst);


#endif





