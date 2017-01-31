/******************** (C) COPYRIGHT 2016 ***************************************
* File Name          : app_nvr_data.h
* Author             : Lv Haifeng
* Version            : V 1.6.0
* Date               : 2016-05-16
* Description        : 
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODECTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, BELLING INC. SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAINMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/
#ifndef _USER_APP_NVR_DATA_H
#define _USER_APP_NVR_DATA_H

/* Exported Datatype-----------------------------------------------------------*/
typedef struct
{
	float u_k;
	float u_b;
	float i_k;
	float i_b;

	float t_k;					//温度校准
	float t_b;

	u16	domain_id;
	u16 vid;
	u16 gid;

	u8 crc_high;				//直接写, 在arm中由于大端表示, 将不能通过crc验证
	u8 crc_low;
}APP_DATA_STRUCT;

void init_app_nvr_data(void);
BOOL is_app_nvr_data_valid(void);
STATUS save_app_nvr_data(void);

void set_app_nvr_data_u(float k, float b);
float get_app_nvr_data_u_k(void);
float get_app_nvr_data_u_b(void);

void set_app_nvr_data_i(float k, float b);
float get_app_nvr_data_i_k(void);
float get_app_nvr_data_i_b(void);

void set_app_nvr_data_t(float k, float b);
float get_app_nvr_data_t_k(void);
float get_app_nvr_data_t_b(void);


void set_app_nvr_data_domain_id(u16 domain_id);
u16 get_app_nvr_data_domain_id(void);

void set_app_nvr_data_vid(u16 vid);
u16 get_app_nvr_data_vid(void);

void set_app_nvr_data_gid(u16 gid);
u16 get_app_nvr_data_gid(void);

#endif

