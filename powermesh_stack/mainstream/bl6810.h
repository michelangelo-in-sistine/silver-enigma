/******************************************************************************
File:			BL6810.H
Author:			Lv Haifeng	
Date:			2012-03-20
Version: 		v1.0
******************************************************************************/
#ifndef _BL6810_H
#define _BL6810_H

/*----------------------------------------------------------------------------
Communications System Registers Definations
----------------------------------------------------------------------------*/

/****REG ADDR****/
#define ADR_VERSION	    0x00
#define ADR_ENABLE		0x01
#define ADR_ACPS_PERIOD	0x02
#define ADR_ACPS_VALID	0x03
#define ADR_AGCWORD		0x09
#define ADR_ADDA		0x0B
#define ADR_ACPS		0x0C
#define ADR_RCV_ACPS0	0x26
#define ADR_RCV_ACPS1	0x2A
#define ADR_RCV_ACPS2	0x2E
#define ADR_RCV_ACPS3	0x32


#define ADR_XMT_RCV		0x10
#define ADR_XMT_CTRL	0x11
#define ADR_XMT_AMP		0x12
#define ADR_XMT_BUF		0x13
#define ADR_XMT_STATUS	0x14

#define ADR_SYNC_TRHD	0x20
#define ADR_SYNC_ENDR	0x21
#define ADR_SYNC_DS15	0x22
#define ADR_SYNC_DS63	0x23


#define ADR_RCV_MFOS	0x24		// [3:2] 00:ch1,01:ch2...11:ch4, [1:0] 00:bpsk,01:ds15,02:ds63
#define ADR_RCV_INFO	0x25

#define ADR_RCV_MSG0	0x27
#define ADR_RCV_MSG1	0x2b
#define ADR_RCV_MSG2	0x2f
#define ADR_RCV_MSG3	0x33
#define ADR_RCV_STA_MSK	0x36
#define ADR_RCV_INFO2	0x37		// For FireUpdate Rcv Info

#define ADR_RCV_BYTE0	0x28
#define ADR_RCV_CMPT0	0x29
#define ADR_RCV_BYTE1	0x2C
#define ADR_RCV_CMPT1	0x2D
#define ADR_RCV_BYTE2	0x30
#define ADR_RCV_CMPT2	0x31
#define ADR_RCV_BYTE3	0x34
#define ADR_RCV_CMPT3	0x35


#define ADR_AGC_SAGLINE	0x06
#define ADR_AGC_SAGLINE2 0x04
#define ADR_AGC_OVERLINE 0x05
#define ADR_AGC_SAGGATE	0x08
#define ADR_AGC_OVERGATE 0x07
#define ADR_AGC_CWORD	0x09

#define ADR_SNR_CTRL	0x40
#define ADR_SNR_POSH	0x41
#define ADR_SNR_POSL	0x42
#define ADR_SNR_PONH	0x43
#define ADR_SNR_PONL	0x44

//----- CRC --------------//
#define ADR_CRC_INIT	0x45
#define ADR_CRC_DATA	0x46
#define ADR_CRC_HIGH	0x47
#define ADR_CRC_LOW		0x48

//----- RS CODEC ---------//
#define ADR_RS_DATA0	0x4A
#define ADR_RS_REDU0	0x54
#define ADR_RS_CTRL		0x5E

//----- NVR & IFP REGS ---//
#define ADR_NVR_ADDRL	0xF0
#define ADR_NVR_ADDRH	0xF1
#define ADR_NVR_WD		0xF2
#define ADR_NVR_RD		0xF3
#define ADR_NVR_CTRL	0xF4

#define ADR_FLA_ADDRL	0xF8
#define ADR_FLA_ADDRH	0xF9
#define ADR_FLA_WD		0xFA
#define ADR_FLA_RD		0xFB
#define ADR_FLA_CTRL	0xFC
#define ADR_FLA_EN		0xFD
#define ADR_FLA_RETI	0xFE

#define ADR_WPTD		0xFF
#endif
