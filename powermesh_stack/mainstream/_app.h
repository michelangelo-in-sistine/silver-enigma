/******************** (C) COPYRIGHT 2016 ********************
* File Name          : app.h
* Author             : Lv Haifeng
* Version            : V1.0.3
* Date               : 2016-05-23
* Description        : 
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODECTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, BELLING INC. SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAINMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _APP_H
#define _APP_H

/* Exported defines ------------------------------------------------------------*/
#define BIT_ACP_CMD_MASK_T		0x04
#define BIT_ACP_CMD_MASK_U		0x02
#define BIT_ACP_CMD_MASK_I		0x01


/* Exported types ------------------------------------------------------------*/
typedef enum
{
	ACP_IDTP_DB=0,				//domain id broadcast addressing
	ACP_IDTP_VC,				//vid communication
	ACP_IDTP_MC,				//multiple communication
	ACP_IDTP_GB					//group broadcast
}ACP_IDTP_ENUM;


typedef struct
{
	UID_HANDLE	target_uid;
	ACP_IDTP_ENUM	acp_idtp;
	BOOL		follow;
	u8 			tran_id;
	u16			domain_id;		//using in every type
	u16			vid;			//using in vc
	u16			gid;			//using in gb

	u16			start_vid;		//using in mc
	u16			end_vid;
	u16			self_vid;

	ARRAY_HANDLE acp_body;
	u8			acp_body_len;

	ARRAY_HANDLE resp_buffer;
	u8			expected_acp_res_bytes;
	u8			actual_acp_res_bytes;
}ACP_SEND_STRUCT, *ACP_SEND_HANDLE;


/* Exported functions ------------------------------------------------------- */
void init_app(void);
STATUS save_id_info_into_app_nvr(void);

void set_comm_mode(u8 comm_mode);
STATUS call_vid_for_current_parameter(UID_HANDLE target_uid, u8 mask, s16* return_parameter);
void acp_rcv_proc(APP_RCV_HANDLE pt_app_rcv);


u8 acp_req_by_uid(UID_HANDLE target_uid, ARRAY_HANDLE acp_command, u8 acp_len, u8 * resp_buffer, u8 expected_res_bytes);
u8 acp_req_by_vid(UID_HANDLE target_uid, u16 domain_id, u16 vid, ARRAY_HANDLE acp_command, u8 acp_len, u8 * resp_buffer, u8 expected_res_bytes);
STATUS acp_domain_broadcast(u16 domain_id, ARRAY_HANDLE acp_command, u8 acp_len);
STATUS acp_req_mc(u16 domain_id, u16 start_vid, u16 end_vid, ARRAY_HANDLE acp_command, u8 acp_len, ARRAY_HANDLE resp_buffer, u8 expected_res_bytes);


STATUS set_se_node_id_by_uid(ARRAY_HANDLE target_uid, u16 domain_id, u16 vid, u16 gid);



#endif
