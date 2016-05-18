/******************** (C) COPYRIGHT 2012 Belling Inc. ********************
* File Name          : addr_db.c
* Author             : Lv Haifeng
* Version            : V2.0
* Date               : 2013-03-15
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
//#include "compile_define.h"
//#include "powermesh_datatype.h"
//#include "powermesh_config.h"
//#include "general.h"
//#include "addr_db.h"
#include "powermesh_include.h"

//extern notify_psr_expired_addr(ADDR_REF_TYPE expired_addr);

/* Private define ------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
ADDR_ITEM_STRUCT xdata _addr_item[CFG_ADDR_ITEM_CNT];
ADDR_CNT_TYPE xdata _addr_item_usage;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
* Function Name  : init_addr_db()
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void init_addr_db()
{
	mem_clr(_addr_item,sizeof(ADDR_ITEM_STRUCT),CFG_ADDR_ITEM_CNT);
	_addr_item_usage = 0;
}

#ifdef USE_ADDR_DB

/*******************************************************************************
* Function Name  : incre_all_items_expire_cnt()
* Description    : 所有地址项的过期指数加1
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void incre_all_items_expire_cnt()
{
	ADDR_CNT_TYPE i;
	
	for(i=0;i<_addr_item_usage;i++)
	{
		if(_addr_item[i].expire_cnt != CST_MAX_EXPIRE_CNT)
		{
			_addr_item[i].expire_cnt++;
		}
	}
}


/*******************************************************************************
* Function Name  : add_addr_db()
* Description    : 将6字节uid加入数据库里, 如数据库原来就有, 返回索引值, expire清零
					如果没有:
					1. 如数据库未满, 添到空白处, 返回索引值;
					2. 如数据库已满, 使用expire最大的条目, 并引用notify_expired_addr方法;
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
ADDR_REF_TYPE add_addr_db(UID_HANDLE uid)
{
	ADDR_CNT_TYPE i;
	ADDR_ITEM_STRUCT xdata * pt;

	pt = &_addr_item[0];
	for(i=0;i<_addr_item_usage;i++)
	{
		if(mem_cmp(uid,pt->uid,6))
		{
			break;
		}
		pt++;
	}

	if(i<_addr_item_usage)		// if found
	{
		pt->expire_cnt = 0;
		return i;
	}
	else
	{
		// 增加新的地址, 首先对原地址的年龄加1, 这样能区分出新加入的地址和老地址, 避免总是同一个位置的新地址被冲刷掉
	
		incre_all_items_expire_cnt();

		if(_addr_item_usage<CFG_ADDR_ITEM_CNT)
		{
			i = _addr_item_usage;
			pt = &_addr_item[i];
			mem_cpy(pt->uid, uid, 6);
			pt->xmode_rmode = 0;
			pt->expire_cnt = 0;
			_addr_item_usage++;
			return i;
		}

		/*******************************************************************
		如果地址库容量已满, 清除使用量最小的地址条目, 并通知引用对象地址过期
		*******************************************************************/
		else
		{
			u8 max_expire_cnt;
			ADDR_REF_TYPE max_expire_item;

			max_expire_cnt = 0;
			max_expire_item = 0;
			pt = &_addr_item[0];
			for(i=0;i<_addr_item_usage;i++)
			{
				if(pt->expire_cnt > max_expire_cnt)
				{
					max_expire_item = i;
					max_expire_cnt = pt->expire_cnt;
				}
				pt++;
			}

			notify_expired_addr(max_expire_item);		//通知引用对象地址过期;
			pt = &_addr_item[max_expire_item];
			mem_cpy(pt->uid, uid, 6);
			pt->xmode_rmode = 0;
			pt->expire_cnt = 0;
			return max_expire_item;
		}
	}
}

/*******************************************************************************
* Function Name  : update_addr_db
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
STATUS update_addr_db(ADDR_REF_TYPE addr_ref, u8 xmode, u8 rmode)
{
	if(addr_ref > (_addr_item_usage-1))
	{
		return FAIL;
	}

	_addr_item[addr_ref].expire_cnt = 0;
	_addr_item[addr_ref].xmode_rmode = (encode_xmode(xmode,0)<<4) | (encode_xmode(rmode,0));		// "先下后上"原则
	return OKAY;
}

/*******************************************************************************
* Function Name  : inquire_addr_db
* Description    : 返回索引的uid地址, 并对其余未索引的地址计数器加1
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
UID_HANDLE inquire_addr_db(ADDR_REF_TYPE addr_ref)
{
	if(addr_ref > (_addr_item_usage-1))
	{
		return NULL;
	}

	/* 所有地址索引计数器加1 */
	incre_all_items_expire_cnt();


	/* 被索引的地址索引计数器清零 */
	_addr_item[addr_ref].expire_cnt = 0;
	
	return _addr_item[addr_ref].uid;
}

/*******************************************************************************
* Function Name  : inquire_addr_db_xmode
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
u8 inquire_addr_db_xmode(ADDR_REF_TYPE addr_ref)
{
	if(addr_ref > (_addr_item_usage-1))
	{
		return 0;
	}

	return decode_xmode(_addr_item[addr_ref].xmode_rmode >> 4);
}

/*******************************************************************************
* Function Name  : inquire_addr_db_rmode
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
//u8 inquire_addr_db_rmode(ADDR_REF_TYPE addr_ref)
//{
//	if(addr_ref > (_addr_item_usage-1))
//	{
//		return 0;
//	}

//	return decode_xmode(_addr_item[addr_ref].xmode_rmode & 0x0F);
//}

/*******************************************************************************
* Function Name  : check_addr_ref_validity
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
RESULT check_addr_ref_validity(ADDR_REF_TYPE addr_ref)
{
	if(addr_ref<_addr_item_usage)
	{
		return CORRECT;
	}
	else
	{
		return WRONG;
	}
}

/*******************************************************************************
* Function Name  : inquire_addr_db_by_uid
* Description    : 
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
#if DEVICE_TYPE==DEVICE_CC || DEVICE_TYPE==DEVICE_CV
ADDR_REF_TYPE inquire_addr_db_by_uid(ARRAY_HANDLE uid)
{
	ADDR_CNT_TYPE i;
	
	for(i=0;i<_addr_item_usage;i++)
	{
		if(mem_cmp(_addr_item[i].uid,uid,6))
		{
			return i;
		}
	}
	return INVALID_RESOURCE_ID;
}
#endif


/*******************************************************************************
* Function Name  : notify_expired_addr
* Description    : 通知所有引用了地址库的对象地址过期的消息
					目前只有psr
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void notify_expired_addr(ADDR_REF_TYPE expired_addr)
{
	notify_psr_expired_addr(expired_addr);
}

#endif
/******************* (C) COPYRIGHT 2012 Belling *****END OF FILE****/

