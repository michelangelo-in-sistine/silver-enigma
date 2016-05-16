/******************** (C) COPYRIGHT 2016 ***************************************
* File Name          : mem.c
* Author             : Lv Haifeng
* Version            : V 2.0
* Date               : 10/26/2012
* Description        : Dynamic RAM allocation scheme based on UCOS source code
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
//#include "compile_define.h"
//#include "powermesh_datatype.h"
//#include "powermesh_config.h"
//#include "hardware.h"
//#include "mem.h"
//#include "uart.h"


/* Private typedef -----------------------------------------------------------*/
typedef struct							  /* MEMORY CONTROL BLOCK                                      */
{                   
    void * OSMemAddr;            /* Pointer to beginning of memory partition                  */
    void * OSMemFreeList;        /* Pointer to list of free memory blocks                     */
    u16     OSMemBlkSize;                 /* Size (in bytes) of each block of memory                   */
    u16		OSMemNBlks;                   /* Total number of blocks in this partition                  */
    u16		OSMemNFree;                   /* Number of memory blocks remaining in this partition       */
    void * OSMemBound;					  /* Use to judge validality when OSMemPut */
} OS_MEM;

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
u8 xdata _dynamic_mem_minor[CFG_MEM_MINOR_BLOCKS_CNT][CFG_MEM_MINOR_BLOCK_SIZE];
u8 xdata _dynamic_mem_superior[CFG_MEM_SUPERIOR_BLOCKS_CNT][CFG_MEM_SUPERIOR_BLOCK_SIZE];

OS_MEM xdata _os_mem_tcb_minor;
OS_MEM xdata _os_mem_tcb_superior;


/* Private function prototypes -----------------------------------------------*/
void OSMemCreate (OS_MEM * os_mem, void * addr, u8 nblks, u16 blksize);

/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : init_mem
* Description    : 
* Input          : 
* Output         : None
* Return         : None
*******************************************************************************/
void init_mem(void)
{
	OSMemCreate(&_os_mem_tcb_minor, _dynamic_mem_minor, CFG_MEM_MINOR_BLOCKS_CNT, CFG_MEM_MINOR_BLOCK_SIZE);
	OSMemCreate(&_os_mem_tcb_superior, _dynamic_mem_superior, CFG_MEM_SUPERIOR_BLOCKS_CNT, CFG_MEM_SUPERIOR_BLOCK_SIZE);
}

/*******************************************************************************
* Function Name  : OSMemCreate
* Description    : Each block contains the next available block address in the head to form a chain data structure.
* Input          : Head address of the memory bank, number of blocks, block size;
* Output         : None
* Return         : None
*******************************************************************************/
void OSMemCreate (OS_MEM * os_mem, void * addr, u8 nblks, u16 blksize)
{
    void**		plink;
    u8*			pblk;
    u8			i;
	
	os_mem->OSMemAddr = addr;
	os_mem->OSMemBlkSize = blksize;
	os_mem->OSMemFreeList = addr;
	os_mem->OSMemNBlks = nblks;
	os_mem->OSMemNFree = nblks;
	os_mem->OSMemBound = (void *)((u32)addr + nblks * blksize - 1);
	
    plink = (void **)addr;                            /* Create linked list of free memory blocks      */
    pblk  = (u8*)((u32)addr + blksize);
    for (i = 0; i < (nblks - 1); i++) 
	{
       *plink = (void *)pblk;                         /* Save pointer to NEXT block in CURRENT block   */
        plink = (void **)pblk;                        /* Position to  NEXT      block                  */
        pblk  = (u8*)(pblk + blksize);    		  	  /* Point to the FOLLOWING block                  */
    }
    *plink              = (void *)0;                  /* Last memory block points to NULL              */
}

/*******************************************************************************
* Function Name  : OSMemGet()
* Description    : 
* Input          : Head address of the memory bank, number of blocks, block size;
* Output         : None
* Return         : None
*******************************************************************************/
void * OSMemGet(OS_MEM_TYPE type) reentrant
{
	OS_MEM * 	pmem = 0;
	void * 	pblk = 0;
	
	ENTER_CRITICAL();
	if(type == MINOR)
	{
		pmem = &_os_mem_tcb_minor;
	}
	else if(type == SUPERIOR)
	{
		pmem = &_os_mem_tcb_superior;
	}
	else
	{
#ifdef DEBUG_DISP_INFO
		my_printf("error mem type!\n");
#endif
	}

	if (pmem && pmem->OSMemNFree > 0)  					  	/* See if there are any free memory blocks	   */
	{
		pblk = pmem->OSMemFreeList;	  				  /* Yes, point to next free memory block		   */
		pmem->OSMemFreeList = *(void **)pblk;		  /*	  Adjust pointer to new free list		   */
		pmem->OSMemNFree--; 						  /*	  One less memory block in this partition  */
		EXIT_CRITICAL();
		return (pblk);								  /*	  Return memory block to caller 		   */
	}
	else
	{
#ifdef DEBUG_DISP_INFO
		my_printf("out of memory!\n");
#endif
	}
	EXIT_CRITICAL();
	return ((void *)0); 							  /*	  Return NULL pointer to caller 		   */
}

/*******************************************************************************
* Function Name  : OSMemPut()
* Description    : 
* Input          : 
* Output         : None
* Return         : None
*******************************************************************************/
void OSMemPut(OS_MEM_TYPE type, void * pblk) reentrant
{
	OS_MEM * pmem = 0;
	
	ENTER_CRITICAL();
	if(type == MINOR)
	{
		pmem = &_os_mem_tcb_minor;
	}
	else if(type == SUPERIOR)
	{
		pmem = &_os_mem_tcb_superior;
	}
	else
	{
#ifdef DEBUG_DISP_INFO
		my_printf("error mem type!\n");
#endif		
	}
	
	if(pmem &&((u32)pblk < (u32)(pmem->OSMemAddr)))				// check whether the returned memory belongs to this tcb region
	{
#ifdef DEBUG_DISP_INFO
		my_printf("put mem out of lower bound!\n");
#endif
		EXIT_CRITICAL();
		return;
	}
	else if((u32)pblk > (u32)(pmem->OSMemBound))
	{
#ifdef DEBUG_DISP_INFO
		my_printf("put mem out of upper bound!\n");
#endif
		EXIT_CRITICAL();
		return;
	}
	else												// check whether the returned memory has already been released.
	{
		void * pt = pmem->OSMemFreeList;
		while(pt != NULL)
		{
			if(pt == pblk)
			{
#ifdef DEBUG_DISP_INFO
				my_printf("release duplicated memory!\n");
#endif
				EXIT_CRITICAL();
				return;
			}
			else
			{
				pt = *(void **)pt;
			}
		}
	}
	
	if (pmem->OSMemNFree >= pmem->OSMemNBlks)	 /* Make sure all blocks not already returned		   */
	{
#ifdef DEBUG_DISP_INFO
			my_printf("memory full!\n");
#endif
	}
	*(void **)pblk		= pmem->OSMemFreeList;	 /* Insert released block into free block list		   */
	pmem->OSMemFreeList = pblk;
	pmem->OSMemNFree++; 						 /* One more memory block in this partition 		   */
	EXIT_CRITICAL();
}


#if BRING_USER_DATA == 0
/*******************************************************************************
* Function Name  : check_available_mem()
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
u8 check_available_mem(OS_MEM_TYPE type)
{
	if(type == MINOR)
	{
		return _os_mem_tcb_minor.OSMemNFree;
	}
	else
	{
		return _os_mem_tcb_superior.OSMemNFree;
	}
}
#endif

