/******************** (C) COPYRIGHT 2016 ***************************************
* File Name          : app_nvr_data.c
* Author             : Lv Haifeng
* Version            : V 2.0
* Date               : 2016-05-20
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
#include "powermesh_include.h"
#include "_userflash.h"

/* Private Datatype-----------------------------------------------------------*/
typedef struct
{
	float u_k;
	float u_b;
	float i_k;
	float i_b;

	u16	domain_id;
	u16 vid;
	u16 gid;

	u8 calib_t[sizeof(CALIB_T_STRUCT)];		//为了节约内存, calib_t的矫正数据直接存放在nvr数据体里

	u8 crc_high;				//直接写, 在arm中由于大端表示, 将不能通过crc验证
	u8 crc_low;
}APP_DATA_STRUCT;

/* private variables ---------------------------------------------------------*/
APP_DATA_STRUCT xdata _app_nvr_data;


/* private functions ---------------------------------------------------------*/
void set_app_nvr_data_u(float k, float b)
{
	_app_nvr_data.u_k = k;
	_app_nvr_data.u_b = b;
}

float get_app_nvr_data_u_k(void)
{
	return _app_nvr_data.u_k;
}

float get_app_nvr_data_u_b(void)
{
	return _app_nvr_data.u_b;
}

void set_app_nvr_data_i(float k, float b)
{
	_app_nvr_data.i_k = k;
	_app_nvr_data.i_b = b;
}

float get_app_nvr_data_i_k(void)
{
	return _app_nvr_data.i_k;
}

float get_app_nvr_data_i_b(void)
{
	return _app_nvr_data.i_b;
}

u8 * get_app_nvr_data_t_data(void)
{
	return _app_nvr_data.calib_t;
}



void set_app_nvr_data_domain_id(u16 domain_id)
{
	_app_nvr_data.domain_id = domain_id;
}
u16 get_app_nvr_data_domain_id(void)
{
	return _app_nvr_data.domain_id;
}

void set_app_nvr_data_vid(u16 vid)
{
	_app_nvr_data.vid = vid;
}

u16 get_app_nvr_data_vid(void)
{
	return _app_nvr_data.vid;
}

void set_app_nvr_data_gid(u16 gid)
{
	_app_nvr_data.gid = gid;
}

u16 get_app_nvr_data_gid(void)
{
	return _app_nvr_data.gid;
}

u16 load_app_nvr_data(void)
{
	return read_user_storage((u8*)(&_app_nvr_data), sizeof(_app_nvr_data));
}

BOOL is_app_nvr_data_valid(void)
{
	if(check_crc((u8*)(&_app_nvr_data), sizeof(_app_nvr_data)))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

STATUS save_app_nvr_data(void)
{
	u16 crc;
	
	crc = calc_crc((u8*)(&_app_nvr_data), sizeof(_app_nvr_data)-2);
	_app_nvr_data.crc_high = (u8)(crc>>8);
	_app_nvr_data.crc_low = (u8)(crc);
	erase_user_storage();
	return (STATUS)write_user_storage((u8*)(&_app_nvr_data), sizeof(_app_nvr_data));
}

void init_app_nvr_data(void)
{
	load_app_nvr_data();
	if(!is_app_nvr_data_valid())
	{
		u16 i;
		u8 * pt;

		pt = (u8*)(&_app_nvr_data);
		for(i=0;i<sizeof(_app_nvr_data);i++)
		{
			*pt++ = 0xFF;
		}
	}
}


