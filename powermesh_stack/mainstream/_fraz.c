/******************** (C) COPYRIGHT 2016 ********************
* File Name          : _fraz.c
* Author             : Lv Haifeng
* Version            : V 2.0
* Date               : 2016-08-10
* Description        : 提供一个存取冻结数据的数据库接口
* 						采集器在广播冻结数据命令时, 会附带一个非零的特征码, SS在保存当前冻结数据时, 同时存储该特征码
*						采集器会使用此特征码做后续的数据采集任务
*						保存冻结数据时, 会寻找同一特征码的存储位置并将其覆盖, 如无, 则优选尚未使用过的存储位置; 
*						当所有位置都使用过后, 选择最陈旧数据的存储位置
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
* Description    : 将旧的数据向内存高地址移, 总是将地址0送去写新的数据
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
* Description    : 存储一条冻结数据, mask指定存储的数据类型
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
		if(!(fraz_data_db[fid].mask & mask))			//必须从申请后没有被写过
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
* Description    : 读取一条冻结数据, mask指定存储的数据类型, 如读取错误, 返回0xFFFF
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




