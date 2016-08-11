/******************** (C) COPYRIGHT 2012 Belling Inc. ********************
* File Name          : compile define.h
* Author             : Lv Haifeng
* Version            : V2.0
* Date               : 2013-03-08
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
/* Pragma ------------------------------------------------------------------------*/
#pragma pack(1)						//1字节数据对齐

/* Basic Type Define ------------------------------------------------------------*/
#define DEVICE_CC	1				//Concentrator
#define DEVICE_MT	2				//Meter Module
#define DEVICE_DC	3				//Data Collector

#define SOC_MODE	1
#define SPI_MODE	2

#define CPU_STM32F103ZE			1
#define CPU_BL6810				2
#define CPU_STM32F103RC			3


/* Device Type Define ------------------------------------------------------------*/
#define DEVICE_TYPE	DEVICE_CC	

/* Device Hardware Option ------------------------------------------------------------*/
#if DEVICE_TYPE==DEVICE_CC
	#define CPU_TYPE CPU_STM32F103ZE
	#define PLC_CONTROL_MODE	SPI_MODE
#elif DEVICE_TYPE==DEVICE_MT
	#define CPU_TYPE CPU_BL6810
	#define PLC_CONTROL_MODE	SOC_MODE
#elif DEVICE_TYPE==DEVICE_DC
	#define CPU_TYPE CPU_STM32F103RC
	#define PLC_CONTROL_MODE	SPI_MODE
#endif

#if CPU_TYPE==CPU_STM32F103ZE || CPU_TYPE==CPU_STM32F103RC
	#define CPU_STM32
#endif

#if CPU_TYPE==CPU_BL6810
	#define HARD_RSCODEC
#endif

/* Compile Option-----------------------------------------------------------------*/
//#define RELEASE						//固件发布版

#define CODE_SAVE					//为节约代码空间, 必要功能之外的功能可以不加入编译
#define USE_RSCODEC
#define USE_DATA_COMPRESSION		//拓扑信息保存在flash中使用压缩算法

#define DISABLE_IFP
#define CHECK_PARAMETER
#define RESET_ALLOWED				//允许系统错误时自复位
#define PIPE_UNACK					//PIPE通信使用UNACK方案
#define FIX_AGC_WHEN_RCV			//接收时固定RCV
#define FLOODING_NO_FORWARD			//集中器不转发洪泛帧

#define SEARCH_BY_PHASE 			//测试按相位发送搜索请求
#define CHECK_NODE_VERSION			//检查节点固件版本
#define IAP_SUPPORT				//是否使用boot loader

//#define USE_IWDG					//使用硬件独立看门狗

#ifdef RELEASE
	#define USE_IWDG					//使用硬件独立看门狗
	#define SEARCH_BY_PHASE			//按相位发送搜索请求
#else
	#define CRC_ISOLATED			//测试版本, 与发布版的CRC校验不同, 用于隔离测试系统
	//#define CRC_DISTURB	0x55		//CRC最后一字节加扰
	#define CRC_DISTURB	0x00		//CRC最后一字节加扰
	#define DISABLE_IFP
#endif

#ifdef IAP_SUPPORT
	#define IAP_NVIC_OFFSET		0x3000
#endif

#define APP_RCV_SS_SNR 1

/* Module Option ------------------------------------------------------------*/
#define USE_MAC

#define USE_PSR
#ifdef USE_PSR
	#define USE_EBC
	#define USE_ADDR_DB
	#define USE_DIAG
#endif
#define USE_FLOODING_SEARCH
#define TEST_TRANSACTION
/* Debug Compile Switch-----------------------------------------------------------*/
/* DEBUG_LEVEL: 
*	0: NO Debug Output; 
*	1: Basic Debug Output; 
*	2: Basic Module; 
*	3: High-Level Module Debug Output */

#define DEBUG_MAC_COMMAND				//uart.c commander code switch, for user lib, these codes should be removed
#define USE_DMA

#define DEBUG_MODE
#define DEBUG_UART_PROC 1
#ifdef DEBUG_MODE
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
//		#define DEBUG_DST
		#define DEBUG_MGNT
		#define DEBUG_MANIPULATION
		#define DEBUG_APP
		#define DEBUG_TOPOLOGY
		#define DEBUG_OPTIMIZATION
		#define DEBUG_APP_TRANSACTION
	#endif
#endif

#ifdef USE_UART 
	#ifdef RELEASE
		#define RELEASE_UART_DEBUG			//in realease version, use uart1 as the debug inout port.
	#endif
#endif

/* Compatibility Defines -----------------------------------------------------*/
#if CPU_TYPE==CPU_STM32F103ZE || CPU_TYPE==CPU_STM32F103RC
	#define idata
	#define xdata					// keep compatibility with 8051 code
	#define code
	#define reentrant
#endif

#endif

