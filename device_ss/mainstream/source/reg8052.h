/******************************************************************************
File:			verisilicon8052.H
Author:			Lv Haifeng	
Date:			2012-03-20
Version: 		v1.0
******************************************************************************/
#ifndef _REG8052_H
#define _REG8052_H
/*----------------------------------------------------------------------------
Verisilicon MCU8052 Core Definations
----------------------------------------------------------------------------*/
/*  BYTE Registers  */
sfr P0    = 0x80;
sfr P2    = 0xA0;
sfr P3    = 0xB0;
sfr PSW   = 0xD0;
sfr ACC   = 0xE0;
sfr B     = 0xF0;
sfr SP    = 0x81;
sfr DPL   = 0x82;
sfr DPH   = 0x83;
sfr DPL1  = 0x84;
sfr DPH1  = 0x85;
sfr PCON  = 0x87;
sfr TCON  = 0x88;
sfr TMOD  = 0x89;
sfr TMOD2 = 0xC9;
sfr TCON2 = 0xC8;
sfr TL0   = 0x8A;
sfr TL1   = 0x8B;
sfr TL2   = 0xCC;
sfr TH0   = 0x8C;
sfr TH1   = 0x8D;
sfr TH2	  = 0xCD;

sfr CKCON = 0x8E;
sfr P1    = 0x90;
sfr DPS   = 0x92;
sfr WDTCON= 0x95;
sfr WDTREL= 0x96;
sfr SCON  = 0x98;
sfr SBUF  = 0x99;
sfr IEN2  = 0x9A;
sfr IE    = 0xA8;
sfr EIE   = 0xA9;
sfr SRELL = 0xAA;
sfr IP    = 0xB8;
sfr SRELH = 0xBA;
sfr IRCON = 0xC0;
sfr MD0   = 0xE9;
sfr MD1   = 0xEA;
sfr MD2   = 0xEB;
sfr MD3   = 0xEC;
sfr MD4   = 0xED;
sfr MD5   = 0xEE;
sfr ARCON = 0xEF;

/*  BIT Registers  */
/*  PSW  */
sbit CY    = PSW^7;
sbit AC    = PSW^6;
sbit F0    = PSW^5;
sbit RS1   = PSW^4;
sbit RS0   = PSW^3;
sbit OV    = PSW^2;
sbit P     = PSW^0;

/*  TCON  */
sbit TF1   = TCON^7;
sbit TR1   = TCON^6;
sbit TF0   = TCON^5;
sbit TR0   = TCON^4;
sbit IE1   = TCON^3;	//Interrupt1 edge flag. Set by hardware when falling edge on external pin int1 is observed. Cleared when interrupt is processed.
sbit IT1   = TCON^2;	//Interrupt1 type control bit. Selects falling edge or low level on inut pin to activate interrupt. 
sbit IE0   = TCON^1;
sbit IT0   = TCON^0;

/* TCON2  */
sbit TF2   = TCON2^5;
sbit TR2   = TCON2^4;
sbit IE2   = TCON2^1;


/*  SCON  */
sbit SM0   = SCON^7;
sbit SM1   = SCON^6;
sbit SM20  = SCON^5;	//Enables multiprocessor communication feature
sbit REN   = SCON^4;	//Enables serial reception. Cleared by software to disable reception
sbit TB8   = SCON^3;	//9th transmitted data bit in Modes 2 and 3.
sbit RB8   = SCON^2;
sbit TI    = SCON^1;
sbit RI    = SCON^0;

/*  IE  */
sbit EA    = IE^7;
sbit ET2   = IE^6;
sbit EWDT  = IE^5;
sbit ES    = IE^4;
sbit ET1   = IE^3;
sbit EX1   = IE^2;
sbit ET0   = IE^1;
sbit EX0   = IE^0;

/*  IP  */
sbit PS    = IP^4;
sbit PT1   = IP^3;
sbit PX1   = IP^2;
sbit PT0   = IP^1;
sbit PX0   = IP^0;

/* 	P3	*/
sbit P30 = P3^0;
sbit P31 = P3^1;
sbit P32 = P3^2;
sbit P33 = P3^3;
sbit P34 = P3^4;
sbit P35 = P3^5;
sbit P36 = P3^6;
sbit P37 = P3^7;

/*  P3 ALT */
sbit RXD   = P3^0;
sbit TXD   = P3^1;

/* CSR Entry */
sfr DATA_BUF = 0xDA;
sfr	EXT_ADR = 0xD9;
sfr	EXT_DAT = 0xD8;
sfr IFP_ADR = 0xF9;
sfr IFP_DAT = 0xF8;

#endif
