/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _TIMER_H
#define _TIMER_H

/* Includes ------------------------------------------------------------------*/

/* Exported constants & macro --------------------------------------------------------*/
//#define TIMER0_PERIOD_MS	1.0944				//period of timer0 int

/* Exported types ------------------------------------------------------------*/

/* Exported variants ------------------------------------------------------- */

/* Exported functions ------------------------------------------------------- */
void init_timer(void);

u16 get_global_clock16(void);

TIMER_ID_TYPE req_timer(void);
void delete_timer(TIMER_ID_TYPE timer_id);
void set_timer(TIMER_ID_TYPE tid, u32 val) reentrant;
void pause_timer(TIMER_ID_TYPE tid);
void reset_timer(TIMER_ID_TYPE tid);
BOOL is_timer_over(TIMER_ID_TYPE tid) reentrant;
BOOL is_timer_idle(TIMER_ID_TYPE tid);
u32 get_timer_val(TIMER_ID_TYPE tid);
u8 check_available_timer(void);
//void init_tim3(void);

void timer_int_svr(void);
//void tim3_int_svr(void);

#if DEVICE_TYPE == DEVICE_CC
BOOL is_timer_valid(TIMER_ID_TYPE tid);
void resume_timer(TIMER_ID_TYPE tid);
#ifdef USE_MAC
void systick_int_svr(void);
u32 get_systick_time(void);
#endif
#endif

#endif



