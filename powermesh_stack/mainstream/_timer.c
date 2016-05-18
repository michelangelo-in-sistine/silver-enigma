/******************** (C) COPYRIGHT 2012 Belling Inc. ********************
* File Name          : timer.c
* Author             : Lv Haifeng
* Version            : V2.0
* Date               : 2016-05-12
* Description        : 
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODECTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, BELLING INC. SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAINMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "powermesh_include.h"

//#include "powermesh_datatype.h"
//#include "powermesh_config.h"
//#include "hardware.h"
//#include "general.h"
//#include "timer.h"
//#include "uart.h"
//#include "queue.h"
//#include "phy.h"
//#include "dll.h"
//#include "psr.h"
//#include "mgnt_app.h"

/* Private define ------------------------------------------------------------*/


/* Private typedef -----------------------------------------------------------*/
typedef enum
{
	IDLE = 0x00,
	PAUSE = 0x80,
	RUNNING = 0x81,
	STOP = 0x82
}TIMER_STATUS;							//use the bit 7 as the timer occupation flag

/* Private macro -------------------------------------------------------------*/

/* Extern Variables ----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
u16 xdata _global_clock;
u8 	xdata _timer_control[CFG_TIMER_CNT];		//
u32	xdata _timer_value[CFG_TIMER_CNT];

/* Extern function prototypes -----------------------------------------------*/
void mgnt_app_rcv_proc(APP_STACK_RCV_HANDLE papp);
/* Function definitions ------------------------------------------------------*/
/*******************************************************************************
* Function Name  : init_timer()
* Description    : Initialize timer related registers;
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
void init_timer(void)
{
	unsigned char i;

	_global_clock = 0;											//Tick = 1ms
	
	/*------------------------------------
	Timer data structure initialization
	*-----------------------------------*/
	for(i=0;i<CFG_TIMER_CNT;i++)
	{
		_timer_control[i] = IDLE;
		_timer_value[i] = 0;
	}
	
	init_timer_hardware();
}

/*******************************************************************************
* Function Name  : timer_int_svr()
* Description    : 	in timer svr, such task are performed:
					1. maintain a 16-bit clock variant _global_clock;
					2. drive system timer .
					3. drive phy & dll receiving procedure;
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
void timer_int_svr(void)
{
	unsigned char i;
	/* Global Clock */
	_global_clock++;
	/* Flash LED */
	if((u8)(_global_clock)==0)
	{
//		led_running_flash();
#if PLC_CONTROL_MODE == SPI_MODE
		if(spi_read((_global_clock%CFG_PHASE_CNT),0xFF)!=0xFF)		// 2013-10-30 check if 6810 resetted, 尽管低8位总是0, 但16位取余的结果还是以0,1,2往返重复的
		{
			my_printf("phase %bu 6810 reset: %bu\n",(_global_clock%CFG_PHASE_CNT),(spi_read((_global_clock%CFG_PHASE_CNT),0xFF)));
			init_bl6810_plc();
		}
#endif
	}
	/* Timer Control */
	for(i=0;i<CFG_TIMER_CNT;i++)
	{
		if(_timer_control[i] == RUNNING)
		{
			if(_timer_value[i]>0)
			{
				_timer_value[i]--;
			}
			else
			{
				_timer_control[i] = STOP;
			}
		}
	}

	/* Phy Send Proc */
	send_queue_proc();
#ifdef USE_MAC
	mac_queue_supervise_proc();
#endif


	/* Rcv Proc */
	for(i=0;i<CFG_PHASE_CNT;i++)
	{
		_phy_rcv_obj[i].phase = i;
		phy_rcv_proc(&_phy_rcv_obj[i]);
		
		_dll_rcv_obj[i].phase = i;
		dll_rcv_proc(&_dll_rcv_obj[i]);

		_psr_rcv_obj[i].phase = i;
		nw_rcv_proc(&_psr_rcv_obj[i]);
#ifdef USE_PSR
		_app_rcv_obj[i].phase = i;
		mgnt_app_rcv_proc(&_app_rcv_obj[i]);
#endif
	}

#ifdef USE_MAC
	for(i=0;i<CFG_PHASE_CNT;i++)
	{
		mac_layer_proc(i);
	}
#endif
}

/*---------------------------------------------------------------------------- 
* Belows are Platform independent function definitions
* It means it can be import into C51 directly or with very little modification
* --------------------------------------------------------------------------*/

/*******************************************************************************
* Function Name  : get_global_clock()
* Description	 : 
* Input 		 : None
* Output		 : None
* Return		 : u16 clock value;
*******************************************************************************/
#ifdef DEBUG_MODE
u16 get_global_clock16(void)
{
	u16 val;
	
	ENTER_CRITICAL();
	val = _global_clock;
	EXIT_CRITICAL();
	
	return val;
}
#endif 
/*******************************************************************************
* Function Name  : req_timer()
* Description    : 
* Input          : None
* Output         : None
* Return         : success: timer id, else: -1;
*******************************************************************************/
TIMER_ID_TYPE req_timer(void)
{
	s8 i;

	for(i=0;i<CFG_TIMER_CNT;i++)
	{
		if(_timer_control[i] == IDLE)
		{
			_timer_control[i] = PAUSE;
			return i;
		}
	}
	return INVALID_RESOURCE_ID;
}

/*******************************************************************************
* Function Name  : set_timer()
* Description    : After using timer, call this function to release it.
* Input          : timer id, timer value;
* Output         : None
* Return         : 
*******************************************************************************/
void set_timer(TIMER_ID_TYPE tid, u32 val) reentrant
{
	if(_timer_control[tid] == IDLE)
	{
#ifdef DEBUG_DISP_INFO
		my_printf("set a null timer!");
#endif
	}
	else
	{
		_timer_value[tid] = val;
		_timer_control[tid] = RUNNING;
	}
}

/*******************************************************************************
* Function Name  : reset_timer()
* Description    : After using timer, call this function to release it.
* Input          : timer id, timer value;
* Output         : None
* Return         : 
*******************************************************************************/
void reset_timer(TIMER_ID_TYPE tid)
{
	if(_timer_control[tid] == IDLE)
	{
#ifdef DEBUG_DISP_INFO
		my_printf("reset a null timer!");
#endif
	}
	else
	{
		_timer_value[tid] = 0;
		_timer_control[tid] = PAUSE;
	}
}

/*******************************************************************************
* Function Name  : get_timer_val()
* Description    : After using timer, call this function to release it.
* Input          : timer id, timer value;
* Output         : None
* Return         : 
*******************************************************************************/
#if DEVICE_TYPE == DEVICE_CC || DEVICE_TYPE == DEVICE_CV
u32 get_timer_val(TIMER_ID_TYPE tid)
{
	if(_timer_control[tid] == IDLE)
	{
#ifdef DEBUG_DISP_INFO
		my_printf("access a null timer!");
#endif
		return 0;
	}
	else
	{
		return _timer_value[tid];
	}
}
#endif

/*******************************************************************************
* Function Name  : is_timer_over()
* Description    : check the timer .
* Input          : timer id;
* Output         : None
* Return         : 
*******************************************************************************/
BOOL is_timer_over(TIMER_ID_TYPE tid) reentrant
{
	u8 state = _timer_control[tid];			//防止判断的时候状态发生变化
	
	if((state == STOP))
	{
		return TRUE;
	}
	else if((state == RUNNING) || (state == PAUSE))
	{
		return FALSE;
	}
	else
	{
#ifdef DEBUG_DISP_INFO
		my_printf("check a invalid timer %bx %bx!",tid,state); 	// 013-04-07 if timer is deleted, return true avoid infinited loops.
#endif
		return TRUE;
	}
}

/*******************************************************************************
* Function Name  : is_timer_idle()
* Description    : check the timer .
					2013-04-07 avoid phy timer invalid
* Input          : timer id;
* Output         : None
* Return         : 
*******************************************************************************/
BOOL is_timer_idle(TIMER_ID_TYPE tid)
{
	if(_timer_control[tid] == IDLE)
	{
		return TRUE;
	}
	return FALSE;
}

/*******************************************************************************
* Function Name  : delete_timer()
* Description    : After using timer, call this function to release it.
* Input          : timer id;
* Output         : None
* Return         : 
*******************************************************************************/
void delete_timer(TIMER_ID_TYPE timer_id)
{
	if(_timer_control[timer_id] == IDLE)
	{
#ifdef DEBUG_DISP_INFO
		my_printf("Try to delete a null timer!");
#endif
	}
	else
	{
		_timer_control[timer_id] = IDLE;
		_timer_value[timer_id] = 0;
	}
}

#ifdef USE_DIAG
/*******************************************************************************
* Function Name  : pause_timer()
* Description    : pause a timer
* Input          : timer id;
* Output         : None
* Return         : 
*******************************************************************************/
void pause_timer(TIMER_ID_TYPE tid)
{
	if(_timer_control[tid] == IDLE)
	{
#ifdef DEBUG_TIMER
		my_printf("Try to pause a null timer!");
#endif
	}
	else
	{
		_timer_control[tid] = PAUSE;
	}
}
#endif

#if BRING_USER_DATA == 0
/*******************************************************************************
* Function Name  : check_available_timer()
* Description    : for debug check;
* Input          : None;
* Output         : available timer
* Return         : 
*******************************************************************************/
u8 check_available_timer()
{
	u8 i;
	u8 cnt;
	
	cnt = 0;
	for(i=0;i<CFG_TIMER_CNT;i++)
	{
		if(_timer_control[i] == IDLE)
		{
			cnt++;
		}
	}
	return cnt;
}
#endif

/********************************Below Functions Exist Only in CC ************************/
#if DEVICE_TYPE==DEVICE_CC || DEVICE_TYPE==DEVICE_CV

/*******************************************************************************
* Function Name  : is_timer_valid()
* Description    : check the validality of tid, in case of different module use the same tid handle
* Input          : timer id;
* Output         : None
* Return         : BOOL
*******************************************************************************/
BOOL is_timer_valid(TIMER_ID_TYPE tid)
{
	if(tid > CFG_TIMER_CNT - 1)
	{
		return FALSE;
	}
	else if(_timer_control[tid] == IDLE)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

/*******************************************************************************
* Function Name  : resume_timer()
* Description    : resume a paused timer
* Input          : timer id;
* Output         : None
* Return         : BOOL
*******************************************************************************/
void resume_timer(TIMER_ID_TYPE tid)
{
	if(_timer_control[tid] != PAUSE)
	{
#ifdef DEBUG_TIMER
		my_printf("Try to resume a non paused timer!\r\n");
#endif
	}
	else
	{
		_timer_control[tid] = RUNNING;
	}
}

#ifdef USE_MAC
/*******************************************************************************
* Function Name  : systick_int_svr()
* Description	 : maintain a auto-inc global counter in the highest int priority
* Input 		 : 
* Output		 : None
* Return		 : BOOL
*******************************************************************************/
u32 _systick_cnt_ms = 0;

void systick_int_svr(void)
{
//led_t_flash();

	_systick_cnt_ms++;
}


/*******************************************************************************
* Function Name  : systick_int_svr()
* Description	 : maintain a auto-inc global counter in the highest int priority
* Input 		 : 
* Output		 : None
* Return		 : BOOL
*******************************************************************************/
u32 get_systick_time(void)
{
	return _systick_cnt_ms;
}
#endif
#endif


/******************* (C) COPYRIGHT 2012 Belling *****END OF FILE****/
