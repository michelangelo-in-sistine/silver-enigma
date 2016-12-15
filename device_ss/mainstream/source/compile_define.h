/******************** (C) COPYRIGHT 2015 *****************************************
* File Name          : compile define.h
* Author             : Lv Haifeng
* Version            : V3.0
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
#ifndef _COMPILE_DEFINE_H
#define _COMPILE_DEFINE_H

/*========================== Common Define ===================================*/
/* Device Type Define ----------------------------------------*/
#define DEVICE_CC				1				//Concentrator
#define DEVICE_MT				2				//Meter Module
#define DEVICE_DC				3				//Data Collector
#define DEVICE_CV				4				//PV Data Converger
#define DEVICE_SE				5				//PV Sensor Debug Version
#define DEVICE_SS				6				//PV Sensor (6810 Version)

/* Mode Define -----------------------------------------------*/
#define SOC_MODE				1
#define DEVICE_MODE				2

/* CPU Define ------------------------------------------------*/
#define CPU_STM32F103ZE			1
#define CPU_BL6810				2
#define CPU_STM32F103RC			3
#define CPU_STM32F030C8			4

/* MEASURE Device Devine -------------------------------------*/
#define BL6523GX				1
#define BL6523B					2

/* POWER LINE Define -----------------------------------------*/
#define PL_AC					1				//220 AV Power
#define PL_DC					2				//DC Power

/* NODE Type Define ------------------------------------------*/
#define NODE_MASTER				1				//主节点类型,可主动发起组网,通信
#define NODE_SLAVE				2
#define NODE_MODEM				3				//CV,作为PLC和串口的转换器


/*========================== Individual Define ====================*/
/* Hardware Define ------------------------------------------------*/
#define DEVICE_TYPE 		DEVICE_SS
#define CPU_TYPE 			CPU_BL6810
#define MEASURE_DEVICE		BL6523B

#if DEVICE_TYPE==DEVICE_CC || DEVICE_TYPE==DEVICE_CV
	#define NODE_TYPE 			NODE_MASTER
#else
	#define NODE_TYPE			NODE_SLAVE
#endif

#if CPU_TYPE==CPU_STM32F103ZE || CPU_TYPE==CPU_STM32F103RC || CPU_TYPE==CPU_STM32F030C8
	#define CPU_STM32
	#define PLC_CONTROL_MODE	DEVICE_MODE
#else
	#define PLC_CONTROL_MODE	SOC_MODE
#endif

#define USE_MEASURE

/* Application Environment Define ---------------------------------*/
#define POWERLINE 			PL_DC


/*========================== Debug/Release Switch  ================*/
/* Release Define ----------------------------------------*/
//#define RELEASE					//发布版, 无Debug串口
#ifdef RELEASE
//	#define USE_IWDG				//使用硬件独立看门狗
//	#define SEARCH_BY_PHASE			//按相位发送搜索请求
#else
	#define CRC_ISOLATED			//测试版本, 与发布版的CRC校验不同, 用于隔离测试系统
	//#define CRC_DISTURB	0x55		//CRC最后一字节加扰
	#define CRC_DISTURB	0x00		//CRC最后一字节加扰
	#define DISABLE_IFP
#endif

/* Firmware Feature Define ----------------------------------------*/
#ifdef RELEASE
	#define BRING_USER_DATA		0	// if defined, bring back user custom data while buiding network
	//#define USE_RSCODEC
	#define USE_ADDR_DB
	#define USE_DIAG
	#define USE_EBC
	#define USE_PSR
	//#define USE_MAC
	#define USE_PTP
	//#define USE_DST
#else
	#define BRING_USER_DATA		0	// if defined, bring back user custom data while buiding network
	//#define USE_RSCODEC
	#define USE_DIAG
	#define USE_PTP

#endif


/* Debug Output Define ----------------------------------------*/
#ifndef RELEASE
	#if MEASURE_DEVICE == BL6523B
		#define DEBUG_MODE
	#endif
#endif

#ifdef DEBUG_MODE
	#define DEBUG_UART_PROC 1	

	//#define USE_DMA
	#define DEBUG_LEVEL 3

	#if DEBUG_LEVEL >= 1
		#define USE_UART
		#define DEBUG_DISP_INFO
	#endif
	
	#if DEBUG_LEVEL >= 2
		#define DEBUG_TIMER
		#define DEBUG_MEM
		#define DEBUG_RSCODEC
		#define DEBUG_SPI
	#endif
	
	#if DEBUG_LEVEL >= 3
		#define DEBUG_PHY
		#define DEBUG_DLL
		#define DEBUG_MAC
		#define DEBUG_DISP_DLL_RCV
		#define DEBUG_EBC
		#define DEBUG_PSR
		#define DEBUG_DST
		#define DEBUG_MGNT
		#define DEBUG_MANIPULATION
		#define DEBUG_APP
		#define DEBUG_MEASURE
		#define DEBUG_TOPOLOGY
		#define DEBUG_OPTIMIZATION
		#define DEBUG_APP_TRANSACTION
	#endif
#else
	#define DEBUG_UART_PROC 0
#endif

#endif

