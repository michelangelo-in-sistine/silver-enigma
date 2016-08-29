/******************** (C) COPYRIGHT 2016 ********************
* File Name          : _fraz.c
* Author             : Lv Haifeng
* Version            : V 2.0
* Date               : 2016-08-10
* Description        : �ṩһ����ȡ�������ݵ����ݿ�ӿ�
* 						�ɼ����ڹ㲥������������ʱ, �ḽ��һ�������������, SS�ڱ��浱ǰ��������ʱ, ͬʱ�洢��������
*						�ɼ�����ʹ�ô������������������ݲɼ�����
*						���涳������ʱ, ��Ѱ��ͬһ������Ĵ洢λ�ò����串��, ����, ����ѡ��δʹ�ù��Ĵ洢λ��; 
*						������λ�ö�ʹ�ù���, ѡ����¾����ݵĴ洢λ��
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
#include "_fraz.h"

/* Private define ------------------------------------------------------------*/
#define MAX_FREZ_DATA_POINTS	4
/* Private typedef -----------------------------------------------------------*/
typedef struct
{
	u8 feature_code;
	u8 mask;
	s16 fraz_u;
	s16 fraz_i;
	s16 fraz_t;
}FRAZ_RECORD_STRUCT;
typedef signed char FRAZ_ID_TYPE;

/* Private variables ---------------------------------------------------------*/
FRAZ_RECORD_STRUCT xdata fraz_data_db[MAX_FREZ_DATA_POINTS];

/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : init_fraz
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void init_fraz(void)
{
	mem_clr(fraz_data_db, sizeof(FRAZ_RECORD_STRUCT), MAX_FREZ_DATA_POINTS);
}


/*******************************************************************************
* Function Name  : inquire_fraz_record
* Description    : lookup fraz database and return storage index by feature code.
					if not found, return -1
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
FRAZ_ID_TYPE inquire_fraz_record(u8 feature_code)
{
	s8 i;

	if(feature_code != 0)
	{
		for(i=0;i<MAX_FREZ_DATA_POINTS;i++)
		{
			if(fraz_data_db[i].feature_code == feature_code)
			{
				return i;
			}
		}
	}
	return INVALID_RESOURCE_ID;
}


/*******************************************************************************
* Function Name  : req_fraz_record
* Description    : ���ɵ��������ڴ�ߵ�ַ��, ���ǽ���ַ0��ȥд�µ�����
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS req_fraz_record(u8 feature_code)
{
	FRAZ_ID_TYPE fid;
	u8 i;

	
	fid = inquire_fraz_record(feature_code);

	if(fid == INVALID_RESOURCE_ID)
	{
		fid = MAX_FREZ_DATA_POINTS - 1;
	}

	// fid point to the position that should be replaced

	for(i=fid;i>0;i--)
	{
		mem_cpy((ARRAY_HANDLE)&fraz_data_db[i],(ARRAY_HANDLE)&fraz_data_db[i-1], sizeof(FRAZ_RECORD_STRUCT));
	}

	mem_clr(&fraz_data_db[0],sizeof(FRAZ_RECORD_STRUCT),1);
	fraz_data_db[0].feature_code = feature_code;

	return OKAY;
}

/*******************************************************************************
* Function Name  : write_fraz_record
* Description    : �洢һ����������, maskָ���洢����������
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS write_fraz_record(u8 feature_code, u8 mask, u16 data)
{
	FRAZ_ID_TYPE fid;

	fid = inquire_fraz_record(feature_code);

	if(fid != INVALID_RESOURCE_ID)
	{
		if(!(fraz_data_db[fid].mask & mask))			//����������û�б�д��
		{
			fraz_data_db[fid].mask |= mask;
			switch(mask)
			{
				case(BIT_ACP_CMD_MASK_U):
				{
					fraz_data_db[fid].fraz_u = data;
					return OKAY;
				}
				case(BIT_ACP_CMD_MASK_I):
				{
					fraz_data_db[fid].fraz_i = data;
					return OKAY;
				}
				case(BIT_ACP_CMD_MASK_T):
				{
					fraz_data_db[fid].fraz_t = data;
					return OKAY;
				}
				default:
				{
				}
			}
		}
	}
	return FAIL;
}

/*******************************************************************************
* Function Name  : read_fraz_record
* Description    : ��ȡһ����������, maskָ���洢����������, ���ȡ����, ����0xFFFF
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS read_fraz_record(u8 feature_code, u8 mask, s16 * pt_data)
{
	FRAZ_ID_TYPE fid;
	
	fid = inquire_fraz_record(feature_code);

	if(fid != INVALID_RESOURCE_ID)
	{
		if(fraz_data_db[fid].mask & mask)
		{
			switch(mask)
			{
				case(BIT_ACP_CMD_MASK_U):
				{
					*pt_data = fraz_data_db[fid].fraz_u;
					return OKAY;
				}
				case(BIT_ACP_CMD_MASK_I):
				{
					*pt_data = fraz_data_db[fid].fraz_i;
					return OKAY;
				}
				case(BIT_ACP_CMD_MASK_T):
				{
					*pt_data = fraz_data_db[fid].fraz_t;
					return OKAY;
				}
				default:
				{
				}
			}
		}
	}
	return FAIL;
}




