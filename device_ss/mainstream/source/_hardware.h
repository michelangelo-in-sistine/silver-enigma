/******************** (C) COPYRIGHT 2012 Belling Inc. ********************
* File Name          : hardware.h
* Author             : Lv Haifeng
* Version            : V1.0.3
* Date               : 10/28/2012
* Description        : Constant & Macro defination of 3-ph router board based on
						STM32
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODECTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, BELLING INC. SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAINMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _HARDWARE_H
#define _HARDWARE_H

/* Compile switch ------------------------------------------------------------*/

/* Includes ------------------------------------------------------------------*/
#include "reg8052.h"
#include "bl6810.h"

/* Exported constants & macro --------------------------------------------------------*/
#define LED_1				P36
#define LED_2				P35
#define PIN_TX_ON			P37
#define PIN_LED_TR			LED_1
#define PIN_LED_DBG			LED_2
#define led_d_on()			PIN_LED_DBG = 0
#define led_d_off()			PIN_LED_DBG = 1
#define led_d_flash()		PIN_LED_DBG = !PIN_LED_DBG
#define led_tr_flash()		PIN_LED_TR = !PIN_LED_TR




#define FEED_DOG()			WDTREL = 0x00
#define ENTER_CRITICAL()	EA = 0;			//disable interrupts
#define EXIT_CRITICAL()		EA = 1;  		//enable interrupts
#define enable_interrupt()	EA = 1;			//cc需要区分禁止全部中断和保留一个systick(ENTER_CRITICAL())两种情况
#define disable_interrupt()	EA = 0;			




/* Exported types ------------------------------------------------------------*/

/* Exported variants ------------------------------------------------------- */

/* Exported functions ------------------------------------------------------- */

/* System Behavior */
void init_basic_hardware(void);
void system_reset_behavior(void);
void reset_measure_device(void);


/* Hardware Initialize */
void init_timer_hardware(void);
void init_uart_hardware(void);
void init_phy_hardware(void);

/* UART Send8*/
#ifdef USE_UART
void uart_send8(u8 byte_data);
#define my_putchar(x)	uart_send8(x)
#endif

/* Register Access */
u8 read_reg_entity(u8 addr);
#define read_reg(phase,addr) read_reg_entity(addr)
#define write_reg(phase,addr,value) EXT_ADR=(addr);EXT_DAT=(value)
#define write_reg_all(addr,value) EXT_ADR=(addr);EXT_DAT=(value)
#define send_buf(phase,send_byte) DATA_BUF=(send_byte)

/* Device Control */
#define tx_on(phase)  PIN_TX_ON = 0; PIN_LED_TR = 0
#define tx_off(phase) PIN_TX_ON = 1; PIN_LED_TR = 1
#define led_r_on()	  PIN_LED_TR = 0
#define led_r_off()	  PIN_LED_TR = 1


#define led_running_flash() //LED_RUNNING = !LED_RUNNING;


/* INT Hardware Serve */
void timer_hardware_int_svr(void);
void uart_hardware_int_svr(void);

/* Common Check Operation */
RESULT check_parity(u8 byte, u8 parity);

/* Base Timing Calculation */
u32 phy_trans_sticks(BASE_LEN_TYPE ppdu_len, u8 rate, u8 scan) reentrant;
u32 srf_trans_sticks(u8 rate, u8 scan) reentrant;

/* UID Operation */
void get_uid_entity(u8 xdata * pt);
#define get_uid(phase,pt) get_uid_entity(pt)

/* Measurement */
void init_measure_com_hardware(void);

/* storage */
STATUS erase_nvr(void);
u16 read_user_storage(u8 * read_buffer, u16 read_len);
u16 write_user_storage(u8 * write_buffer, u16 write_len);
#define erase_user_storage() erase_nvr()

#endif
