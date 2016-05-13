/******************** (C) COPYRIGHT 2016 ***************************************
* File Name          : hardware.h
* Author             : Lv Haifeng
* Version            : V1.0.3
* Date               : 2016-05-07
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
#ifndef _HARDWARE_H
#define _HARDWARE_H

/* Includes ------------------------------------------------------------------*/
#include "bl6810.h"


/* Exported constants & macro --------------------------------------------------------*/
#define read_reg(phase, addr) 		read_spi(addr)
#define write_reg(phase, addr, value)	write_spi(addr,value)
#define write_reg_all(addr, value)	write_spi(addr, value)



/* Exported functions ------------------------------------------------------- */
void ENTER_CRITICAL(void);
void EXIT_CRITICAL(void);
void FEED_DOG(void);

void init_basic_hardware(void);
void system_reset_behavior(void);

void init_phy_hardware(void);


void init_timer_hardware(void);
void init_usart2_hardware(void);
void init_gpio(void);
void uart_send8(u8 byte_data);
void usart2_int_entry(void);



u8 read_spi(u8 addr);
void write_spi(u8 addr, u8 value);
void init_spi(void);
u8 switch_spi(u8 value);
RESULT check_spi(void);

void send_buf(u8 phase, u8 send_byte);
void tx_on(u8 phase);
void tx_off(u8 phase);



void exti_int_entry(void);

void led_timer_int_on(void);
void led_timer_int_off(void);
void led_main_loop_flash(void);

/* Common Check Operation */
RESULT check_parity(u8 byte, u8 parity);

/* Base Timing Calculation */
TIMING_CALCULATION_TYPE phy_trans_sticks(BASE_LEN_TYPE ppdu_len, u8 rate, u8 scan);
TIMING_CALCULATION_TYPE srf_trans_sticks(u8 rate,u8 scan);


/* Measure Calculation */
void init_measure_com_hardware(void);
void measure_com_send8(u8 byte_data);
u8 measure_com_read8(u8 * rec_byte);

#endif

