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
* Description    : ���е�ַ��Ĺ���ָ����1
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
* Description    : ��6�ֽ�uid�������ݿ���, �����ݿ�ԭ������, ��������ֵ, expire����
					���û��:
					1. �����ݿ�δ��, ���հ״�, ��������ֵ;
					2. �����ݿ�����, ʹ��expire������Ŀ, ������notify_expired_addr����;
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
		// �����µĵ�ַ, ���ȶ�ԭ��ַ�������1, ���������ֳ��¼���ĵ�ַ���ϵ�ַ, ��������ͬһ��λ�õ��µ�ַ����ˢ��
	
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
		�����ַ����������, ���ʹ������С�ĵ�ַ��Ŀ, ��֪ͨ���ö����ַ����
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

			notify_expired_addr(max_expire_item);		//֪ͨ���ö����ַ����;
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
	_addr_item[addr_ref].xmode_rmode = (encode_xmode(xmode,0)<<4) | (encode_xmode(rmode,0));		// "���º���"ԭ��
	return OKAY;
}

/*******************************************************************************
* Function Name  : inquire_addr_db
* Description    : ����������uid��ַ, ��������δ�����ĵ�ַ��������1
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

	/* ���е�ַ������������1 */
	incre_all_items_expire_cnt();


	/* �������ĵ�ַ�������������� */
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
* Description    : ֪ͨ���������˵�ַ��Ķ����ַ���ڵ���Ϣ
					Ŀǰֻ��psr
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

