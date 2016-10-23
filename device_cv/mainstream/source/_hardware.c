
/* Includes ------------------------------------------------------------------*/
#include "compile_define.h"
#include "stm32f0xx.h"
#include "powermesh_include.h"
#include "core_cmFunc.h"


/* Private define ------------------------------------------------------------*/
#define PLC_SPI					SPI2
#define	PLC_SPI_GPIO_PORT		GPIOB
#define PLC_SPI_SCS_PIN			GPIO_Pin_12
#define PLC_SPI_SCK_PIN			GPIO_Pin_13
#define PLC_SPI_SCK_PIN_SOURCE	GPIO_PinSource13
#define PLC_SPI_MISO_PIN		GPIO_Pin_14
#define PLC_SPI_MISO_PIN_SOURCE	GPIO_PinSource14
#define PLC_SPI_MOSI_PIN		GPIO_Pin_15
#define PLC_SPI_MOSI_PIN_SOURCE	GPIO_PinSource15

#define PLC_SPI_INT_GPIO_PORT	GPIOA
#define PLC_SPI_INT_PIN			GPIO_Pin_8
#define PLC_SPI_INT_EXTI_LINE	EXTI_Line8
#define PLC_SPI_INT_IRQC		EXTI8IRQn

#define PLC_SPI_SCS_LOW()		(PLC_SPI_GPIO_PORT->BRR=PLC_SPI_SCS_PIN)
#define PLC_SPI_SCS_HIGH()		(PLC_SPI_GPIO_PORT->BSRR=PLC_SPI_SCS_PIN)

#define LED_GPIO_PORT			GPIOA
#define LED_TEST3_PIN			GPIO_Pin_5
#define LED_TEST4_PIN			GPIO_Pin_6
#define LED_TIMER_INT			LED_TEST3_PIN
#define LED_PHY_INT				LED_TEST4_PIN
#define LED_PHY					LED_PHY_INT

#define BL6810_CTRL_GPIO_PORT	GPIOB
#define BL6810_MODE_PIN			GPIO_Pin_3
#define BL6810_RST_PIN			GPIO_Pin_4
#define BL6810_TXON_PIN			GPIO_Pin_5

#define RS485_TXEN_GPIO_PORT		GPIOB
#define RS485_TXEN_PIN			GPIO_Pin_6


/* Private variables ---------------------------------------------------------*/
u8  _uart_output_enable = 1;
u8  _dma_uart_enable = 1;


/* Private function prototypes -----------------------------------------------*/
void led_flash(uint16_t pin);
void led_on(uint16_t pin);
void led_off(uint16_t pin);

/*******************************************************************************
* Function Name  : init_basic_hardware()
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void init_basic_hardware(void)
{
//#ifndef RELEASE
//	init_rcc();
//#endif
	init_gpio();
//#ifndef RELEASE
//	init_nvic();		//主程序
//#endif
	init_spi();
}

/*******************************************************************************
* Function Name  : system_reset_behavior
* Description    : flash led, & reset, restore 6810, 6523
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void system_reset_behavior()
{
	u32 i,j;
	
	GPIO_ResetBits(BL6810_CTRL_GPIO_PORT, BL6810_MODE_PIN | BL6810_RST_PIN);
	
	for(i=0;i<4;i++)
	{
		for(j=0;j<599999UL;j++);
		led_flash(LED_TEST3_PIN);
		led_flash(LED_TEST4_PIN);
	}
	GPIO_SetBits(BL6810_CTRL_GPIO_PORT, BL6810_RST_PIN);	

	for(i=0;i<4;i++)
	{
		for(j=0;j<599999UL;j++);
		led_flash(LED_TEST3_PIN);
		led_flash(LED_TEST4_PIN);
	}
}

/*******************************************************************************
* Function Name  : reset_measure_device
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
//void reset_measure_device(void)
//{
//	u32 i;
	
//	GPIO_ResetBits(RS485_TXEN_GPIO_PORT, RS485_TXEN_PIN);
//	for(i=0;i<999999UL;i++);
//	GPIO_SetBits(RS485_TXEN_GPIO_PORT, RS485_TXEN_PIN);
//	for(i=0;i<999999UL;i++);									//6523 need time to finish reset
//}

///*******************************************************************************
//* Function Name  : measure_com_send
//* Description    : 
//* Input          : 
//* Output         : 
//* Return         : 
//*******************************************************************************/
//void measure_com_send(u8 * head, u8 len)
//{
//	u8 i;
//	for(i=0;i<len;i++)
//	{
//		measure_com_send8(*head++);
//	}
//}

///*******************************************************************************
//* Function Name  : write_bl6523
//* Description    : 
//* Input          : 
//* Output         : 
//* Return         : 
//*******************************************************************************/
//void write_measure_reg(u8 addr, u32 dword_value)
//{
//	u8 cs=0;
//	u8 buffer[6];
//	u8 i;
//	
//	buffer[0] = 0xCA;
//	buffer[1] = addr;
//	cs = addr;

//	for(i=2;i<5;i++)
//	{
//		buffer[i] = (u8)dword_value;
//		cs += buffer[i];
//		dword_value>>=8;
//	}
//	buffer[5] = ~cs;

//	measure_com_send(buffer,sizeof(buffer));
//}

///*******************************************************************************
//* Function Name  : read_bl6523
//* Description    : 
//* Input          : 
//* Output         : 
//* Return         : 
//*******************************************************************************/
//u32 read_measure_reg(u8 addr)
//{
//	u8 rec_byte;
//	u8 i;
//	u8 buffer[4];
//	u32 value = 0;
//	
//	measure_com_send8(0x35);
//	measure_com_send8(addr);

//	for(i=0;i<4;i++)
//	{
//		if(measure_com_read8(&rec_byte))
//		{
//			buffer[i] = rec_byte;
//		}
//		else
//		{
//			my_printf("bl6532 return fail\n");
//			return 0;
//		}
//		
//	}

//	for(i=0;i<4;i++)
//	{
//		addr += buffer[i];
//	}

//	if(addr!=255)
//	{
//		my_printf("bl6532 return bytes checked fail\n");
//		uart_send_asc(buffer,4);
//		return 0;
//	}
//	else
//	{
//		for(i=2;i!=0xFF;i--)
//		{
//			value <<= 8;
//			value += buffer[i];
//		}
//		return value;
//	}
//}



void ENTER_CRITICAL(void)
{
	__set_PRIMASK(1);
}

void EXIT_CRITICAL(void)
{
	__set_PRIMASK(0);
}

void FEED_DOG(void)
{
}



/**
  * @brief  Configure the TIM IRQ Handler.
  * @param  None
  * @retval None
  */
void init_timer_hardware(void)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	/* TIM3 clock enable */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = 239;
	TIM_TimeBaseStructure.TIM_Prescaler = 199;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	/* Enable the TIM3 gloabal Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure); 
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

	TIM_Cmd(TIM3, ENABLE);

}

void led_on(uint16_t pin)
{
	GPIO_ResetBits(LED_GPIO_PORT,pin);
}

void led_off(uint16_t pin)
{
	GPIO_SetBits(LED_GPIO_PORT,pin);
}


void toggle_gpio_pin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
	if(GPIOx->ODR & GPIO_Pin)
	{
		GPIOx->BRR = GPIO_Pin;
	}
	else
	{
		GPIOx->BSRR = GPIO_Pin;
	}
	
}

void led_flash(uint16_t pin)
{
	toggle_gpio_pin(LED_GPIO_PORT,pin);
}

void led_timer_int_on(void)
{
	led_on(LED_TIMER_INT);
}

void led_timer_int_off(void)
{
	led_off(LED_TIMER_INT);
}

void led_r_on(void)
{
	led_on(LED_PHY_INT);
}

void led_r_off(void)
{
	led_off(LED_PHY_INT);
}


/*******************************************************************************
* Function Name  : init_gpio()
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void init_gpio(void)
{
	volatile u32 i,j;
	
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = LED_TEST3_PIN | LED_TEST4_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(LED_GPIO_PORT, &GPIO_InitStructure);
	GPIO_SetBits(LED_GPIO_PORT, LED_TEST3_PIN | LED_TEST4_PIN);

	GPIO_InitStructure.GPIO_Pin = BL6810_MODE_PIN | BL6810_RST_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(BL6810_CTRL_GPIO_PORT, &GPIO_InitStructure);

	//RS485 TXEN PIN
	GPIO_InitStructure.GPIO_Pin = RS485_TXEN_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(RS485_TXEN_GPIO_PORT, &GPIO_InitStructure);
}

void init_interface_uart_hardware(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
  	USART_ClockInitTypeDef  USART_ClockInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	/* Enable GPIO clock */
	RCC_AHBPeriphClockCmd(DEBUG_UART_GPIO_RCC_AHBPeriph, ENABLE);

	/* Enable USART clock */

#if UART_PORT == UART_PORT_1
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
#elif UART_PORT == UART_PORT_2
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
#endif

	/* Connect PXx to USARTx_Tx */
	GPIO_PinAFConfig(DEBUG_UART_GPIO, DEBUG_UART_TXD_PinSource, DEBUG_UART_GPIO_AF);	//USART1
	GPIO_PinAFConfig(DEBUG_UART_GPIO, DEBUG_UART_RXD_PinSource, DEBUG_UART_GPIO_AF);

	/* Configure USART Tx as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = DEBUG_UART_TXD;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(DEBUG_UART_GPIO, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = DEBUG_UART_RXD;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_Init(DEBUG_UART_GPIO, &GPIO_InitStructure);

	/* USART时钟 */
	USART_ClockInitStructure.USART_Clock = USART_Clock_Disable;
	USART_ClockInitStructure.USART_CPOL = USART_CPOL_Low;
	USART_ClockInitStructure.USART_CPHA = USART_CPHA_2Edge;
	USART_ClockInitStructure.USART_LastBit = USART_LastBit_Disable;
	USART_ClockInit(DEBUG_UART_PORT, &USART_ClockInitStructure );

	
	/* Uart口配置 */
	USART_InitStructure.USART_BaudRate = 38400;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(DEBUG_UART_PORT, &USART_InitStructure );

	/* 设置,使能 */
	USART_Cmd(DEBUG_UART_PORT, ENABLE);

	/* Uart接收中断使能 */
	USART_ITConfig(DEBUG_UART_PORT, USART_IT_RXNE, ENABLE);

	/* Enable the USART Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = DEBUG_UART_IRQChannel;
	NVIC_InitStructure.NVIC_IRQChannelPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

#ifdef USE_DMA
	init_dma();
#endif	
}


void uart_send8(u8 byte_data)
{
	rs485_tx_on();
	while(USART_GetFlagStatus(DEBUG_UART_PORT, USART_FLAG_TXE) == RESET);
	USART_SendData(DEBUG_UART_PORT, byte_data);
	while(USART_GetFlagStatus(DEBUG_UART_PORT, USART_FLAG_TC) == RESET);
	rs485_tx_off();
}


//void usart2_int_entry(void)
//{
//	u8 byte_data;

//	if(USART_GetITStatus(DEBUG_UART_PORT, USART_IT_RXNE) != RESET)
//	{
//		byte_data = USART_ReceiveData(DEBUG_UART_PORT);
//		interface_uart_rcv_int_svr(byte_data);
//	}
//	else
//	{
//		my_printf("error int:");
//		if(USART_GetITStatus(DEBUG_UART_PORT, USART_IT_PE) != RESET)
//		{
//			my_printf("USART_IT_PE\r\n");
//			USART_ClearITPendingBit(DEBUG_UART_PORT,USART_IT_PE);
//		}
//		else if(USART_GetITStatus(DEBUG_UART_PORT, USART_IT_TXE) != RESET)
//		{
//			my_printf("USART_IT_TXE\r\n");
//			USART_ClearITPendingBit(DEBUG_UART_PORT,USART_IT_TXE);
//		}
//		else if(USART_GetITStatus(DEBUG_UART_PORT, USART_IT_TC) != RESET)
//		{
//			my_printf("USART_IT_TC\r\n");
//			USART_ClearITPendingBit(DEBUG_UART_PORT,USART_IT_TC);
//		}
//		else if(USART_GetITStatus(DEBUG_UART_PORT, USART_IT_RXNE) != RESET)
//		{
//			my_printf("USART_IT_RXNE\r\n");
//			USART_ClearITPendingBit(DEBUG_UART_PORT,USART_IT_RXNE);
//		}
//		else if(USART_GetITStatus(DEBUG_UART_PORT, USART_IT_IDLE) != RESET)
//		{
//			my_printf("USART_IT_IDLE\r\n");
//			USART_ClearITPendingBit(DEBUG_UART_PORT,USART_IT_IDLE);
//		}
//		else if(USART_GetITStatus(DEBUG_UART_PORT, USART_IT_LBD) != RESET)
//		{
//			my_printf("USART_IT_LBD\r\n");
//			USART_ClearITPendingBit(DEBUG_UART_PORT,USART_IT_LBD);
//		}
//		else if(USART_GetITStatus(DEBUG_UART_PORT, USART_IT_CTS) != RESET)
//		{
//			my_printf("USART_IT_CTS\r\n");
//			USART_ClearITPendingBit(DEBUG_UART_PORT,USART_IT_CTS);
//		}
//		else if(USART_GetITStatus(DEBUG_UART_PORT, USART_IT_ORE) != RESET)
//		{
//			my_printf("USART_IT_ORE\r\n");
//			USART_ClearITPendingBit(DEBUG_UART_PORT,USART_IT_ORE);
//		}
//		else if(USART_GetITStatus(DEBUG_UART_PORT, USART_IT_NE) != RESET)
//		{
//			my_printf("USART_IT_NE\r\n");
//			USART_ClearITPendingBit(DEBUG_UART_PORT,USART_IT_NE);
//		}
//		else if(USART_GetITStatus(DEBUG_UART_PORT, USART_IT_FE) != RESET)
//		{
//			my_printf("USART_IT_FE\r\n");
//			USART_ClearITPendingBit(DEBUG_UART_PORT,USART_IT_FE);
//		}
//		else
//		{
//			if (USART_GetFlagStatus(DEBUG_UART_PORT, USART_FLAG_ORE) != RESET)//注意！不能使用if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)来判断
//		    {
//				my_printf("over run\r\n");
//		        USART_ReceiveData(DEBUG_UART_PORT);
//    		}
//		}
//	}
//}

/*******************************************************************************
* Function Name  : init_spi
* Description    : Initialize SPI INT(PA8)
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void init_exti(void)
{
	GPIO_InitTypeDef   	GPIO_InitStructure;
	EXTI_InitTypeDef	EXTI_InitStructure;
	NVIC_InitTypeDef	NVIC_InitStructure;
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);

	GPIO_InitStructure.GPIO_Pin = PLC_SPI_INT_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(PLC_SPI_INT_GPIO_PORT, &GPIO_InitStructure);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource8);

	/* Configure EXTI0 line */
	EXTI_InitStructure.EXTI_Line = PLC_SPI_INT_EXTI_LINE;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	/* Enable and set EXTI0 Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = EXTI4_15_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPriority = 0x00;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
}

/*******************************************************************************
* Function Name  : init_spi
* Description    : Initialize STM32 SPI1 Port (comm to BL6810)
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void init_spi(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	SPI_InitTypeDef SPI_InitStructure;
	
	// 给时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);

	// 配IO
	GPIO_InitStructure.GPIO_Pin = PLC_SPI_SCK_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_Init(PLC_SPI_GPIO_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = PLC_SPI_MISO_PIN;
	GPIO_Init(PLC_SPI_GPIO_PORT, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = PLC_SPI_MOSI_PIN;
	GPIO_Init(PLC_SPI_GPIO_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = PLC_SPI_SCS_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(PLC_SPI_GPIO_PORT, &GPIO_InitStructure);

	PLC_SPI_SCS_HIGH();

	GPIO_PinAFConfig(PLC_SPI_GPIO_PORT, PLC_SPI_SCK_PIN_SOURCE, GPIO_AF_0);
	GPIO_PinAFConfig(PLC_SPI_GPIO_PORT, PLC_SPI_MISO_PIN_SOURCE, GPIO_AF_0);
	GPIO_PinAFConfig(PLC_SPI_GPIO_PORT, PLC_SPI_MOSI_PIN_SOURCE, GPIO_AF_0);

	
	// 配置SPI
	SPI_I2S_DeInit(PLC_SPI);
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_128;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(PLC_SPI, &SPI_InitStructure);

	 /* Initialize the FIFO threshold */
	SPI_RxFIFOThresholdConfig(PLC_SPI, SPI_RxFIFOThreshold_QF);

	/* Enable SPI1  */
	SPI_Cmd(PLC_SPI, ENABLE);

}

/*******************************************************************************
* Function Name  : check_spi()
* Description    : 检查SPI与三相BL6810的通信关系
					2014-08-07 更改成连读100遍稳定才认为电压稳定, 再往下走
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
RESULT check_spi()
{
	u8 i,j;
	
	for(j=0;j<100;j++)
	{
		for(i=0;i<CFG_PHASE_CNT;i++)
		{
			if(read_reg(i,0)!=0xA1)
			{
#ifdef DEBUG_DISP_INFO
				my_printf("SPI PHASE %bd CHECK FAILED\r\n",i);
#endif
				return WRONG;
			}
		}
	}
	return CORRECT;
}


/*******************************************************************************
* Function Name  : spi_switch
* Description    : Swith a 8-byte info with SPI Slave
* Input          : Byte which is to delivered to slave
* Output         : Byte which is returned from slave
* Return         : Byte which is returned from slave
*******************************************************************************/
u8 switch_spi(u8 value)
{
	u32 awd = 0;
	/* Loop while DR register in not emplty */
	do
	{
		if(awd++>50000UL)
		{
			my_printf("spi tx lock!\n");
			return 0;
		}
	}
	while(SPI_I2S_GetFlagStatus(PLC_SPI, SPI_I2S_FLAG_TXE) == RESET); 

	/* Send value through the SPI1 peripheral */	
	SPI_SendData8(PLC_SPI, value);	

	/* Wait to receive a value */
	awd = 0;
	do
	{
		if(awd++>50000UL)	//anti deadloop
		{
			my_printf("spi rx lock!\n");
			return 0;
		}
	}
	while(SPI_I2S_GetFlagStatus(PLC_SPI, SPI_I2S_FLAG_RXNE) == RESET);

	/* Return the value read from the SPI bus */  
	value = SPI_ReceiveData8(PLC_SPI); 

	return value;

}

/*******************************************************************************
* Function Name  : spi_write
* Description    : Write a byte to BL6810
* Input          : ch, address to wrote, byte to wrote in
* Output         : None
* Return         : None
*******************************************************************************/
void write_spi(u8 addr, u8 value)
{
	addr &= 0x7F;
//	__RESTORE_AND_SET_PRIMASK();
	ENTER_CRITICAL();

	PLC_SPI_SCS_LOW();
	switch_spi(addr);
	switch_spi(value);
	PLC_SPI_SCS_HIGH();

	EXIT_CRITICAL();
}




/*******************************************************************************
* Function Name  : spi_read
* Description    : Read a byte info from SPI Slave
* Input          : ch, addr
* Output         : None
* Return         : byte read from SPI Slave
*******************************************************************************/
u8 read_spi(u8 addr)
{
	u8 byte = 0;

//	__RESTORE_AND_SET_PRIMASK();
	ENTER_CRITICAL();					//2014-12-18 保证SPI操作为原子操作
	
	addr |= 0x80;
	{
		PLC_SPI_SCS_LOW();
		switch_spi(addr);
		byte = switch_spi(0xFF);
		PLC_SPI_SCS_HIGH();
	}


//	__RECOVER_PRIMASK();
	EXIT_CRITICAL();
	return byte;
}
/*******************************************************************************
* Function Name  : send_buf
* Description    : send a byte to plc send buf
* Input          : ch, send_byte
* Output         : None
* Return         : 
*******************************************************************************/
void send_buf(u8 phase, u8 send_byte)
{
	write_reg(phase,ADR_XMT_BUF,send_byte);
}

/*******************************************************************************
* Function Name  : tx_on
* Description    : tx on
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void tx_on(u8 phase)
{
	led_on(LED_PHY);
	GPIO_ResetBits(BL6810_CTRL_GPIO_PORT, BL6810_TXON_PIN);
}

/*******************************************************************************
* Function Name  : tx_off
* Description    : tx on
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void tx_off(u8 phase)
{
	led_off(LED_PHY);
	GPIO_SetBits(BL6810_CTRL_GPIO_PORT, BL6810_TXON_PIN);
}

/*******************************************************************************
* Function Name  : rs485_tx_on
* Description    : tx on
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void rs485_tx_on(void)
{
	GPIO_SetBits(RS485_TXEN_GPIO_PORT, RS485_TXEN_PIN);	
}

/*******************************************************************************
* Function Name  : rs485_tx_off
* Description    : tx off
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void rs485_tx_off(void)
{
	GPIO_ResetBits(RS485_TXEN_GPIO_PORT, RS485_TXEN_PIN);
}


/*=======================================================================================
*						Phy Circuit Related
*========================================================================================*/

/*******************************************************************************
* Function Name  : init_phy_hardware
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void init_phy_hardware(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);

	GPIO_InitStructure.GPIO_Pin = BL6810_TXON_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(BL6810_CTRL_GPIO_PORT, &GPIO_InitStructure);

	GPIO_SetBits(BL6810_CTRL_GPIO_PORT, BL6810_TXON_PIN);

	/* Enable EXTI  */
	init_exti();
}

/*******************************************************************************
* Function Name  : check_parity
* Description    : parity = [X X X X X P 0 1], and {byte P} should have an even number of "1" totally
* Input          : 
* Output         : 
* Return         : cs;
*******************************************************************************/
RESULT check_parity(u8 byte, u8 parity)
{
	u8 p = 0;
	u8 i;
	if((parity & 0x03) == 0x01)
	{
		for(i=0;i<=7;i++)
		{
			if(byte&0x01)
			{
				p = !p;
			}
			byte >>= 1;
		}
		parity>>=2;
		if(parity == p)
		{
			return CORRECT;
		}
	}
	return WRONG;
}

/*******************************************************************************
* Function Name  : phy_trans_sticks
* Description    : 计算数据包在物理层信道传输时间
* Input          : len为数据包长(编码前), rate: 传输速率; scan: scan模式
* Output         : 
* Return         : 
*******************************************************************************/
TIMING_CALCULATION_TYPE phy_trans_sticks(BASE_LEN_TYPE ppdu_len, u8 rate, u8 scan)
{
	TIMING_CALCULATION_TYPE timing;

	timing = packet_chips(ppdu_len,rate,0) * BPSK_BIT_TIMING;
	
	if(scan)
	{
		timing = (timing + XMT_SCAN_INTERVAL_STICKS) * 4;
	}
	return timing;
}

/*******************************************************************************
* Function Name  : srf_trans_sticks
* Description    : 
* Input          : 为保持phy_trans_sticks接口不变,做一个专门计算srf数据包长度的函数
* Output         : 
* Return         : 
*******************************************************************************/
TIMING_CALCULATION_TYPE srf_trans_sticks(u8 rate,u8 scan)
{
	TIMING_CALCULATION_TYPE timing;

	timing = packet_chips(4,rate,1) * BPSK_BIT_TIMING;
	
	if(scan)
	{
		timing = (timing + XMT_SCAN_INTERVAL_STICKS) * 4;
	}
	return timing;
}

/*******************************************************************************
* Function Name  : get_uid
* Description    : ARM: 不能访问芯片UID, 使用STM32的固定DBGMCU_IDCODE 32Bit + 0x00 + Phase组成的48bit 唯一ID代替
					STM的96位唯一ID从地址0x1FFF F7E8开始, 
					即[0x1FFFF7E8 - 0x1FFFF7F3], STM是低字节在前, 
					实验证实手里的芯片高位不同的概率更大, 故采用高5字节+相位组成唯一ID
					PHASE RELATED!!!

					2016-05-12 STM32 0x1FFF F7AC
					
* Input          : 相位, 首地址
* Output         : 
* Return         : 
*******************************************************************************/
void get_uid(u8 phase, ARRAY_HANDLE pt)
{
	u32 stm32ID;
	u32 stm32ID2;
	
	stm32ID = *(u32*)(0x1FFFF7AC);
	stm32ID2 = *(u32*)(0x1FFFF7B0);
	

	*(pt++) = (stm32ID2>>8);
	*(pt++) = (stm32ID2);
	*(pt++) = (stm32ID>>24);
	*(pt++) = (stm32ID>>16);
	*(pt++) = (stm32ID>>8);
	*(pt++) = (stm32ID);
}

/****************************************DMA Related *********************************************************/
/* Hardware Definition ---------------------------------------------------------*/
#if UART_PORT == UART_PORT_1
#define DEBUG_USART_TDR									USART1_BASE+0x28
#define DEBUG_DMA_CHANNEL								DMA1_Channel2
#define DEBUG_DMA_IRQ									DMA1_Channel2_3_IRQn
#define DEBUG_DMA_FLAG									(DMA1_FLAG_TC2|DMA1_FLAG_HT2|DMA1_FLAG_GL2)
#elif UART_PORT == UART_PORT_2
#define DEBUG_USART_TDR									USART2_BASE+0x28
#define DEBUG_DMA_CHANNEL								DMA1_Channel4
#define DEBUG_DMA_IRQ									DMA1_Channel4_5_IRQn
#define DEBUG_DMA_FLAG									(DMA1_FLAG_TC4|DMA1_FLAG_HT4|DMA1_FLAG_GL4)
#endif


/* Private Macron ------------------------------------------------------------*/
#define DMA_BUFFER_DEPTH 								512


/* Private variables ---------------------------------------------------------*/
#ifdef USE_DMA
u8 __dma_buffer[DMA_BUFFER_DEPTH];
u8 * __dma_output_tail_ptr;				//pointer point to the tail of current dma transmission
u8 * __dma_buffer_tail_ptr;				//pointer point to the tail of buffer


/*******************************************************************************
* Function Name  : init_dma
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void init_dma(void)
{
	DMA_InitTypeDef DMA_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	/* DMA1 channel4 configuration */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	
	DMA_DeInit(DEBUG_DMA_CHANNEL);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)DEBUG_USART_TDR;
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)__dma_buffer;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;//from memory read 
	DMA_InitStructure.DMA_BufferSize = 0;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DEBUG_DMA_CHANNEL, &DMA_InitStructure);

	DMA_ITConfig(DEBUG_DMA_CHANNEL, DMA_IT_TC, ENABLE);
	DMA_Cmd(DEBUG_DMA_CHANNEL, ENABLE); 

	/* Enable External Int */
	NVIC_InitStructure.NVIC_IRQChannel = DEBUG_DMA_IRQ;
	NVIC_InitStructure.NVIC_IRQChannelPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	__dma_buffer_tail_ptr = __dma_buffer;
	__dma_output_tail_ptr = __dma_buffer;

	enable_usart_dma(1);
}

/*******************************************************************************
* Function Name  : dma_buffer_append
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void dma_buffer_append(u8 byte)
{
	ENTER_CRITICAL();
	if(__dma_buffer_tail_ptr <= (__dma_buffer+DMA_BUFFER_DEPTH))
	{
		*__dma_buffer_tail_ptr++ = byte;
	}
	EXIT_CRITICAL();
}

/*******************************************************************************
* Function Name  : dma_buffer_fill
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void dma_buffer_fill(u8 * send_content_head, u16 len)
{
	if(len<=DMA_BUFFER_DEPTH)
	{
		ENTER_CRITICAL();
		if(!DMA_GetCurrDataCounter(DEBUG_DMA_CHANNEL))
		{
			__dma_buffer_tail_ptr = __dma_buffer;
			__dma_output_tail_ptr = __dma_buffer;
		}
		else
		{
			if(len>(__dma_buffer + DMA_BUFFER_DEPTH - __dma_buffer_tail_ptr))
			{
				while(DMA_GetCurrDataCounter(DEBUG_DMA_CHANNEL));				// if buffer is almost full, wait until sendover
				__dma_buffer_tail_ptr = __dma_buffer;
				__dma_output_tail_ptr = __dma_buffer;
			}
		}
		mem_cpy(__dma_buffer_tail_ptr,send_content_head,len);
		__dma_buffer_tail_ptr = __dma_buffer + len;

		EXIT_CRITICAL();
	}
}

/*******************************************************************************
* Function Name  : dma_uart_start
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void dma_uart_start()
{
	ENTER_CRITICAL();
	if(!DMA_GetCurrDataCounter(DEBUG_DMA_CHANNEL))
	{
		if(__dma_buffer_tail_ptr>__dma_output_tail_ptr)
		{
			rs485_tx_on();
			DMA_Cmd(DEBUG_DMA_CHANNEL, DISABLE);
			DEBUG_DMA_CHANNEL->CMAR = (u32)(__dma_output_tail_ptr);
			DEBUG_DMA_CHANNEL->CNDTR = (u16)(__dma_buffer_tail_ptr - __dma_output_tail_ptr);
			USART_DMACmd(DEBUG_UART_PORT, USART_DMAReq_Tx, ENABLE);
			DMA_Cmd(DEBUG_DMA_CHANNEL, ENABLE); 
			__dma_output_tail_ptr = __dma_buffer_tail_ptr;
		}
		else
		{
			__dma_buffer_tail_ptr = __dma_buffer;
			__dma_output_tail_ptr = __dma_buffer;
			
			while(USART_GetFlagStatus(DEBUG_UART_PORT, USART_FLAG_TC) == RESET);
			rs485_tx_off();
		}
	}
	EXIT_CRITICAL();
}

/*******************************************************************************
* Function Name  : dma_uart_send
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void dma_uart_send(u8 * send_content_head, u16 len)
{
	dma_buffer_fill(send_content_head, len);
	dma_uart_start();
}



/*******************************************************************************
* Function Name  : enable_usart_dma
* Description    : 是否使用dma输出打印信息, 对于HardFault等中断内信息打印无法使用dma, (因为已不能响应中断)
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void enable_usart_dma(u8 enable)
{
	_dma_uart_enable = enable;
}

#endif
/****************************************DMA Related End******************************************************/

/*******************************************************************************
* Function Name  : my_putchar
* Description    : 统一打印入口处理
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void my_putchar(u8 x)
{
	if(_uart_output_enable)
	{
#ifdef USE_DMA
		if(_dma_uart_enable)
		{
			dma_buffer_append(x);
		}
		else
#endif
		{
			uart_send8(x);
		}
	}
}

/*******************************************************************************
* Function Name  : get_sp_val
* Description    : 返回SP的数值
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
__asm u32 get_sp_val(void)
{
	MOV r0,sp
	BX  r14
}

/*******************************************************************************
* Function Name  : report_stack
* Description    : 输入堆栈地址,将当前堆栈区的PC值打印出来
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void report_stack(const char * str, u32 sp_val)
{
	u32* pt_stack_val;
//	u32 FaultHandlerSource;
//	u32 FaultHandlerAddress;

	pt_stack_val = (u32*)(sp_val);
#ifdef USE_DMA	
	enable_usart_dma(0);
#endif
	my_printf("%s\r\n",str);
	my_printf("R0: 0x%lx\r\n",*pt_stack_val++);
	my_printf("R1: 0x%lx\r\n",*pt_stack_val++);
	my_printf("R2: 0x%lx\r\n",*pt_stack_val++);
	my_printf("R3: 0x%lx\r\n",*pt_stack_val++);
	my_printf("R12:0x%lx\r\n",*pt_stack_val++);
	my_printf("LR: 0x%lx\r\n",*pt_stack_val++);
	my_printf("PC: 0x%lx\r\n",*pt_stack_val++);
	my_printf("PSR:0x%lx\r\n",*pt_stack_val++);
}



