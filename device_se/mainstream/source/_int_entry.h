/******************** (C) COPYRIGHT 2016 Lv Haifeng ********************
* File Name          : _int_entry.h
* Author             : Lv Haifeng
* Version            : V2.0
* Date               : 2016-05-13
* Description        : unified INT entry, hardware proc(clear flag, etc.)
						specified INT sources were defined in stm32f0xx_it.c.
						software proc details were proc in individual c source file
						¨ª3¨°?¦Ì??D??¨¨??¨²¡ä|¨¤¨ª, ?o?e??3y¡Á¡ä¨¬???, ???¡ä?D????¡ä??¡Â¦Ì¨¨
						??¨¬?¦Ì??D???¡ä¡ã2???¨²stm32f0xx_it.c.
						??¨¬?¦Ì?¨¨¨ª?t¡ä|¨¤¨ª¡¤??¨²¡¤?¡Àe¦Ì?c???t?D, timer.c, usart.c, phy.c...
						¡ä????t¨º???¨¬?¦Ì?¨®2?to¨ª¨¨¨ª?t????¦Ì?¨°?2?3¨¦?¨®¡ä|¨¤¨ª?D??2?, ¨°¨°?a2?¨¦??¡ã??¨¬?¨®2?t
						, ¨°¨°¡ä?¨®D?¨¹o?¦Ì??¨¦¨°??2D?
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODECTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, BELLING INC. SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAINMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/
void timer_int_entry(TIM_TypeDef * timer);
void usart_int_entry(USART_TypeDef * usart);
void phy_int_entry(uint32_t exti_line);



