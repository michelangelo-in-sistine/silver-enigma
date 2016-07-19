/******************** (C) COPYRIGHT 2016 ********************
* File Name          : ptp.c
* Author             : Lv Haifeng
* Version            : V 2.0
* Date               : 2016-07-16
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

/* Private define ------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/


/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
PTP_CONFIG_STRUCT xdata _ptp_config_obj;
PTP_STACK_RCV_STRUCT xdata _ptp_rcv_obj[CFG_PHASE_CNT];



/* Private function prototypes -----------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
/*******************************************************************************
* Function Name  : init_ptp
* Description    : 设置PTP缺省的通信方式
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void init_ptp(void)
{
	_ptp_config_obj.comm_mode = CHNL_CH3;
	mem_clr(_ptp_rcv_obj, sizeof(PTP_STACK_RCV_STRUCT), CFG_PHASE_CNT);
}


/*******************************************************************************
* Function Name  : set_ptp_comm_mode
* Description    : NODE_MASTER设置通信方式, NODE_SLAVE按照接收的通信方式改变通信方式
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void set_ptp_comm_mode(u8 comm_mode)
{
	_ptp_config_obj.comm_mode = comm_mode;
}


/*******************************************************************************
* Function Name  : get_ptp_comm_mode
* Description    : 调用通信方式
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u8 get_ptp_comm_mode(void)
{
	return _ptp_config_obj.comm_mode;
}


/*******************************************************************************
* Function Name  : ptp_rcv_proc
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void ptp_rcv_proc(DLL_RCV_HANDLE pd)
{
	if(pd->dll_rcv_valid)
	{
		PTP_STACK_RCV_HANDLE pptp;
		PHY_RCV_HANDLE pp;

		pptp = &_ptp_rcv_obj[pd->phase];
		pp = GET_PHY_HANDLE(pd);

		
		pptp->phase = pd->phase;
		pptp->comm_mode = (pp->phy_rcv_supposed_ch) | (pp->phy_rcv_valid & 0x0B);

		mem_cpy(pptp->src_uid,&pd->lpdu[SEC_LPDU_SRC],6);

		pptp->apdu = &pd->lpdu[SEC_LPDU_PTP_APDU];
		pptp->apdu_len = pd->lpdu_len-(LEN_LPCI+LEN_PTP_NPCI+LEN_MPCI);
		

		pptp->ptp_rcv_indication = 1;

#if NODE_TYPE == NODE_SLAVE
		set_ptp_comm_mode(pptp->comm_mode);
#else
		if(!(pptp->apdu[1] & 0x10))		//FOR DEBUG!!!
		{
			set_ptp_comm_mode(pptp->comm_mode);
		}
#endif
	}
}


/*******************************************************************************
* Function Name  : ptp_rcv_resume
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void ptp_rcv_resume(PTP_STACK_RCV_HANDLE pptp)
{
	dll_rcv_resume(GET_DLL_HANDLE(pptp));
	pptp->ptp_rcv_indication = 0;
}



