/******************** (C) COPYRIGHT 2012 Belling Inc. ********************
* File Name          : network_optimization.c
* Author             : Lv Haifeng
* Version            : V 2.0
* Date               : 2013-03-28
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

/* Private define ------------------------------------------------------------*/
#define DEBUG_INDICATOR DEBUG_OPTIMIZATION

#ifdef RELEASE
#define OPTIMIZE_PROC_INTERVAL	60000
#else
#define OPTIMIZE_PROC_INTERVAL	1000
#endif

#define POWERMESH_OPTIMIZE_UPGRADE_THRESHOLD	1		//grade_drift_cnt�ķ�Χ��[-4,3],�ﵽ��threshold������
#define POWERMESH_OPTIMIZE_DEGRADE_THRESHOLD	-1		//grade_drift_cnt�ķ�Χ��[-4,3],�ﵽ��threshold������disconnect

/* Private typedef -----------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Extern variables ---------------------------------------------------------*/
extern NODE_HANDLE network_tree[];
extern LINK_STRUCT new_nodes_info_buffer[];
extern DST_CONFIG_STRUCT dst_config_obj;
extern u8 psr_explore_new_node_uid_buffer[];


/* Private variables ---------------------------------------------------------*/
NODE_HANDLE disconnect_list[CFG_MGNT_MAX_NEW_LINKS_BUFFER_CNT];
BOOL network_optimizing_switch = FALSE;							//�����Ż����̿���

/* Private function prototypes -----------------------------------------------*/
STATUS choose_optimized_link_by_scan_diag_data(DLL_SEND_HANDLE pds, ARRAY_HANDLE diag_buffer, LINK_HANDLE optimized_link);
STATUS judge_optimized_link(LINK_HANDLE optimized_link);
STATUS test_pipe_connectivity(u16 pipe_id);
STATUS get_path_script_from_dst_search_result(ARRAY_HANDLE nsdu, BASE_LEN_TYPE nsdu_len,ARRAY_HANDLE script_buffer, u8 * script_length);
BOOL is_network_optimizing_enable(void);
STATUS reevaluate_node_direct(NODE_HANDLE target_node, BOOL * pt_upgraded);
STATUS reevaluate_node_indirect(NODE_HANDLE target_node, BOOL * pt_upgraded);


/* Private functions ---------------------------------------------------------*/


/*******************************************************************************
* Function Name  : unified_diag (Revision 2014-09-05)
* Description    : 	������ʱ���ȷ��Ҫָ��pipe������uid(��repair_pipe, �����м�ڵ��ʱ��), ���ԭuid_diag_uid�Ļ�����
					������Ϊunified_diag, ͬʱ����Ĳ�����source_uid, pipe_id
					+ ��source_uid��ΪNULL, ��Ϊԭ����uid_diag_uid, 
						++ ��Դ�ڵ��Ǳ��ؽڵ�, ����dll_diag; ���õ���λ��Դ�ڵ���λΪ��, �����Դ����DLL_SEND�ṹ�������λΪ׼
						++ ����Զ�˽ڵ�, ����mgnt_diag;(��source_uid������һ��pipe)
					+ ��source_uidΪNULL, ��pipe_id��diag
					+ diag�ɹ�, ���ֽ�Ϊд���ֽ���, ����OKAY;
					+ ��diagʧ��:
						++ ��Դ�ڵ��Ǳ��ؽڵ�, ���ֽ�ΪDIAG_TIMEOUT, ����FAIL;
						++ ��Դ�ڵ���Զ�˽ڵ�:
							+++ ��ͨ��ʧ�ܵ���ʧ��, ���ֽ�ΪTIME_OUT, ����FAIL;
							+++ ��diagʧ��, ���ֽ�ΪDIAG_TIMEOUT, ����FAIL;
					
* Input          : None
* Output         : None
* Return         : OKAY/FAIL
*******************************************************************************/
STATUS unified_diag(UID_HANDLE source_uid, u16 pipe_id, DLL_SEND_HANDLE pds, u8 * diag_buffer)
{
	u8 i,j;
	STATUS status = FAIL;

	/* Դ�ڵ�Ϊ���ؽڵ㴦�� */
	for(i=0;i<CFG_PHASE_CNT;i++)
	{
		if(check_uid(i,source_uid)==CORRECT)
		{
			pds->phase = i;		//���õ���λ��Դ�ڵ���λΪ��, �����Դ����DLL_SEND�ṹ�������λΪ׼
			for(j=0;j<CFG_MAX_DIAG_TRY;j++)
			{
				status = dll_diag(pds, diag_buffer);
				if(status)
				{
					break;
				}
				else
				{
					diag_buffer[0] = DIAG_TIMEOUT;
				}
			}
			return status;
		}
	}

	/* ִ�е��˴�, Դ�ڵ㲻Ϊ���ؽڵ��ΪNULL, 
		��Դ�ڵ㲻ΪNULL, ��������Ӧ��pipe_id, ���ܴ����pipe_id�Ƿ���Ч
		��Դ�ڵ�ΪNULL, ���Դ����pipe_idΪ׼ */
	if(source_uid)
	{
		pipe_id = inquire_pipe_of_uid(source_uid);
	}
	
	if(pipe_id)
	{
		pds->phase = get_pipe_phase(pipe_id);
		status = mgnt_diag(pipe_id, pds, diag_buffer, AUTOHEAL_LEVEL1);
	}
	return status;
}

/*******************************************************************************
* Function Name  : node_diag_uid
* Description    : ָ���ڵ�diag uid
					��diagʧ��, diag_buffer���ֽ�Ϊ0xF5; ��ɹ�, ���ֽ�Ϊд���ֽ���
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
STATUS node_diag_uid(NODE_HANDLE source_node, DLL_SEND_HANDLE pds, u8 * diag_buffer)
{
	if(check_node_valid(source_node))
	{
		return unified_diag(source_node->uid,0,pds,diag_buffer);
	}
	else
	{
		return FAIL;
	}
}


/*******************************************************************************
* Function Name  : node_diag_node
* Description    : ָ���ڵ���diag
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
STATUS node_diag_node(NODE_HANDLE source_node, NODE_HANDLE target_node, DLL_SEND_HANDLE pds, u8 * diag_buffer)
{
	if(check_node_valid(source_node) && check_node_valid(target_node))
	{
		mem_cpy(pds->target_uid_handle, target_node->uid, 6);
		return unified_diag(source_node->uid,0,pds,diag_buffer);
	}
	else
	{
		return FAIL;
	}
}

/*******************************************************************************
* Function Name  : get_optimized_link (Revision 2014-09-05)
* Description    : 
					Ӧ�ù�������ʱ��ȷ��Ҫָ��pipe_id������source_uid(����repair�������漰���м�ڵ�)
					����޸�Ϊ����ָ��source_uid, Ҳ����ָ��pipe_id, ��������link����
					+ ��source_uidΪ�����ж�, ����ΪNULLʱ����Ϊ׼
					+ ��source_uidΪNULLʱ, ��pipe_idΪ׼
					+ initial_rate����, ����ʼ���Ե�����, �Լ��ٲ���Ҫ�ĸ�����ʱ���˷�
					+ ������ʧ��ʱ, ��err_code����ʧ��ԭ��(diag_time_out/psr_time_out)
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
STATUS get_optimized_link(UID_HANDLE source_uid, u16 pipe_id, UID_HANDLE target_uid, u8 initial_rate, LINK_HANDLE optimized_link, u8 * err_code)
{
	u8 diag_buffer[32];
	DLL_SEND_STRUCT dds;
	LINK_STRUCT temp_link;
	ARRAY_HANDLE pt;
	u8 rate;
	STATUS result = FAIL;
	u8 retreat = 0;						// 2013-08-08 �����н����źŵ��ӵ�Ӱ��, ��Щ�ڵ�Ĳ����������������������Եĵ�, �ĳ���������½�һ��, �Խ�ʡʱ��
	
	for(rate=initial_rate;rate<=CFG_MAX_LINK_CLASS;rate++) 
	{
		SET_BREAK_THROUGH("quit get_optimized_link()\r\n");
		mem_clr(&dds,sizeof(DLL_SEND_STRUCT),1);	//dds����λ����Ҫ����,���Դ�ڵ��Ǳ��ؽڵ�,�ᰴ�ձ��ؽڵ�������λ����,�������ؽڵ�,�ᰴ��pipe��λ����
		dds.xmode = 0x10 + rate;
		dds.rmode = 0x10 + rate;
		dds.prop = BIT_DLL_SEND_PROP_DIAG|BIT_DLL_SEND_PROP_SCAN|BIT_DLL_SEND_PROP_REQ_ACK;
		dds.target_uid_handle = target_uid;
		result = unified_diag(source_uid, pipe_id, &dds, diag_buffer);

		if(result)
		{
			pt = diag_buffer + 1;									// ���ֽ�Ϊ���ݳ���
			choose_optimized_link_by_scan_diag_data(&dds,pt,&temp_link);

#ifdef DEBUG_MGNT
			my_printf("DOWN:%bX, SNR:%bX, UP:%bX, SNR:%bX, PTRP:%d\r\n", 
			temp_link.downlink_mode, temp_link.downlink_snr, temp_link.uplink_mode, temp_link.uplink_snr, (u16)(temp_link.ptrp));
#endif

			if(judge_optimized_link(&temp_link))
			{
				break;
			}
			else
			{
				if(rate==CFG_MAX_LINK_CLASS || retreat>0)
				{
					break;
				}
				else
				{
#ifdef DEBUG_MGNT
					my_printf("PTRP TOO LOW, RECHOOSE.\r\n");
#endif
					retreat++;
				}
			}
		}
	}

	if(result)
	{
		mem_cpy((u8*)optimized_link, (u8*)&temp_link, sizeof(LINK_STRUCT));
		*err_code = NO_ERROR;
	}
	else
	{
		*err_code = diag_buffer[0];			
#ifdef DEBUG_INDICATOR
		if(diag_buffer[0] == DIAG_TIMEOUT)
		{
			my_printf("diag fail.\r\n");
		}
		else if(diag_buffer[0] == PSR_TIMEOUT)
		{
			my_printf("psr fail.\r\n");
		}
		else
		{
			my_printf("unknown problem.\r\n");			
		}
#endif
	}

	return result;		
}

//STATUS get_optimized_link(UID_HANDLE source_uid, UID_HANDLE target_uid, u8 initial_rate, LINK_HANDLE optimized_link, u8 * err_code)
//{
//	mem_cpy(optimized_link->uid, target_uid,6);
//	optimized_link->downlink_mode = 0xF1;
//	optimized_link->downlink_ss = 0;
//	optimized_link->downlink_snr = 0;
//	optimized_link->uplink_mode = 0xF1;
//	optimized_link->uplink_ss = 0;
//	optimized_link->uplink_snr = 0;
//	optimized_link->ptrp = 0;
//	return OKAY;
//}

/*******************************************************************************
* Function Name  : get_optimized_link_from_node_to_uid
* Description    : ����node, ��������ָ��uid֮�������link����, ������δ��Ŀ��uid��������ʱ
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
STATUS get_optimized_link_from_node_to_uid(NODE_HANDLE node, UID_HANDLE target_uid, u8 initial_rate, LINK_HANDLE optimized_link, u8 * err_code)
{
	if(check_node_valid(node))
	{
		return get_optimized_link(node->uid, 0, target_uid, initial_rate, optimized_link, err_code);
	}
	return FAIL;
}

/*******************************************************************************
* Function Name  : optimize_link_pipe_uid
* Description    : ��repair pipeʱ, ��Ҫָ��pipe, ������ָ���м�ڵ�, ��Ϊ�м�ڵ����Լ���pipe
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
STATUS get_optimized_link_from_pipe_to_uid(u16 pipe_id, UID_HANDLE target_uid_handle, u8 initial_rate, LINK_STRUCT * optimized_link, u8 * err_code)
{
	return get_optimized_link(NULL, pipe_id, target_uid_handle, initial_rate, optimized_link, err_code);
}


/*******************************************************************************
* Function Name  : choose_optimized_link_by_scan_diag_data
* Description    : ����scan diag���ѡ����������·����
					diag_buffer��ʽ:
					[downlink_ch0_ss downlink_ch0_snr downlink_ch1_ss downlink_ch1_snr downlink_ch2_ss downlink_ch2_snr downlink_ch3_ss downlink_ch3_snr 
					uplink_ch0_ss uplink_ch0_snr uplink_ch1_ss uplink_ch1_snr uplink_ch2_ss uplink_ch2_snr uplink_ch3_ss uplink_ch3_snr]
* Input          : None
* Output         : None
* Return         : PTRP of the link
*******************************************************************************/
STATUS choose_optimized_link_by_scan_diag_data(DLL_SEND_HANDLE pds, ARRAY_HANDLE diag_buffer, LINK_HANDLE optimized_link)
{
	u8 ch_down, ch_up;
	u8 downlink_mode, downlink_ss, downlink_snr;
	u8 uplink_mode, uplink_ss, uplink_snr;
	float ptrp;

	choose_optimized_ch_by_scan_diag_data(diag_buffer, &ch_down, &ch_up);
	downlink_mode = (pds->xmode & 0x0F) | (0x10<<ch_down);
	downlink_ss = diag_buffer[ch_down*2];
	downlink_snr = diag_buffer[ch_down*2+1];
	uplink_mode = (pds->rmode & 0x0F) | (0x10<<ch_up);
	uplink_ss = diag_buffer[(ch_up+4)*2];
	uplink_snr = diag_buffer[(ch_up+4)*2+1];
	ptrp = calc_link_ptrp(downlink_mode, downlink_snr, uplink_mode, uplink_snr, NULL);

	mem_cpy(optimized_link->uid, pds->target_uid_handle,6);
	optimized_link->downlink_mode = downlink_mode;
	optimized_link->downlink_ss = downlink_ss;
	optimized_link->downlink_snr = downlink_snr;
	optimized_link->uplink_mode = uplink_mode;
	optimized_link->uplink_ss = uplink_ss;
	optimized_link->uplink_snr = uplink_snr;
	optimized_link->ptrp = ptrp;

	return OKAY;
}

/*******************************************************************************
* Function Name  : judge_optimized_link
* Description    : ����ptrp�����ʶ���·�������о�,OKAY��pass,������ѡ
* Input          : None
* Output         : None
* Return         : PTRP of the link
*******************************************************************************/
STATUS judge_optimized_link(LINK_HANDLE optimized_link)
{
	u8 xmode;

	xmode = optimized_link->downlink_mode & 0x03;

//	if(xmode==0 && optimized_link->ptrp<(PTRP_DS15/2))
	if(xmode==0 && optimized_link->ptrp < CFG_BPSK_PTRP_GATE)				//2014-09-29 ���������BPSK�ż�
	{
		return FAIL;
	}
	else if(xmode==1 && optimized_link->ptrp<CFG_DS15_PTRP_GATE)
	{
		return FAIL;
	}
	return OKAY;
}

/*******************************************************************************
* Function Name  : refresh_pipe
* Description    : ����ԭ����Ϣ�ؽ�pipe
				2013-08-30 �ĳ��Ȳ鿴����, ���ж�Ӧ�ڵ�, ��������·���ؽ�pipe
							��û�ж�Ӧ�ڵ�, ��pipe�����pipe�ؽ�·��;
* Input          : pipe id
* Output         : 
* Return         : status
*******************************************************************************/
STATUS refresh_pipe(u16 pipe_id)
{
	u8 script_buffer[CFG_PIPE_STAGE_CNT * 8];
	u8 script_length;
	STATUS status;
	PIPE_REF pipe_ref;
	NODE_HANDLE node;
	u8 phase;

	/* �����Ӧ�Ľڵ��������д���, ��������·��Ϊ׼, ������pipe_mnpl������Ϊ׼ */

	/* 2014-04-29 �人���ֳ������ؽ�����, �ʺ��ֹ�����pipe, ���ֹ�����pipeΪ׼, ��˲�ʹ�����˲���pipe*/
	node = pipeid2node(pipe_id);
	if(node)
	{
		phase = get_node_phase(node);
		script_length = get_path_script_from_topology(node, script_buffer);
	}
	else
	{
		pipe_ref = inquire_pipe_mnpl_db(pipe_id);
		
		if(!pipe_ref)
		{
#ifdef DEBUG_INDICATOR
			my_printf("refresh_pipe(): no corresponding node for pipeid:%X\r\n",pipe_id);
#endif
			return FAIL;
		}
		
		phase = pipe_ref->phase;
		script_length = get_path_script_from_pipe_mnpl_db(pipe_id, script_buffer);
	}

	if((script_length<8) || (((script_length>>3)<<3) != script_length))
	{
#ifdef DEBUG_INDICATOR
		my_printf("refresh_pipe(): get wrong pipe script\r\n",pipe_id);
		uart_send_asc(script_buffer,script_length);
		my_printf("\r\n",pipe_id);
#endif	
		return FAIL;
	}

#ifdef DEBUG_INDICATOR
	my_printf("refresh_pipe(): refresh pipe, get pipe script\r\n",pipe_id);
	uart_send_asc(script_buffer,script_length);
	my_printf("\r\n",pipe_id);
#endif	

	status = psr_setup(phase,pipe_id,script_buffer,script_length);	//psr_setup will retry as many as CFG_MAX_SETUP_PIPE_TRY times

	return status;
}

/*******************************************************************************
* Function Name  : repair_pipe
* Description    : �޸�pipe, ���ڽ����I������
			2014-08-28 ȡ��refresh_pipe����޸�pipe, ��Ϊͨ��֮ǰ��refreshһ��
			2014-09-05 repairֻһ��һ�����޸�, ���޸�������������һ��ʧЧ��ֱ�ӷ���ʧ��, ������
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS repair_pipe(u16 pipe_id)
{
	STATUS status = FAIL;
	u8 phase;
	u8 stages;

	u8 script_buffer[CFG_PIPE_STAGE_CNT * 8];
	u8 script_length = 0;
	u8 patrol_buffer[CFG_PIPE_STAGE_CNT * 9 * 2];
	u16 patrol_buffer_len;

	u8 err_code;
	NODE_HANDLE node;
	BOOL use_topology = FALSE;
	LINK_STRUCT test_link;
	u8 i;

	/* patrol_buffer�ĵ�[N*9 + 7]�ֽڶ���SNR */
	u8 downlink_ss[CFG_PIPE_STAGE_CNT];
	u8 downlink_snr[CFG_PIPE_STAGE_CNT];
	u8 downlink_mode[CFG_PIPE_STAGE_CNT];
	u8 uplink_ss[CFG_PIPE_STAGE_CNT];
	u8 uplink_snr[CFG_PIPE_STAGE_CNT];
	u8 uplink_mode[CFG_PIPE_STAGE_CNT];

	/*********************************************************************
	���Ӧ�ڵ���������˽ṹ��, �����˵õ�script, �����pipe���еõ�script
	*********************************************************************/
	node = pipeid2node(pipe_id);
	if(node)
	{
#ifdef DEBUG_INDICATOR
		my_printf("repair_pipe(): correponding node exists for pipe %X, generate script by topology.\r\n",pipe_id);
#endif
	
		phase = get_node_phase(node);
		stages = inquire_pipe_stages(pipe_id);
		script_length = get_path_script_from_topology(node, script_buffer);
		use_topology = TRUE;
	}
	else
	{
		phase = get_pipe_phase(pipe_id);
		stages = inquire_pipe_stages(pipe_id);
		if(phase == (u8)INVALID_RESOURCE_ID)
		{
#ifdef DEBUG_INDICATOR
			my_printf("repair_pipe(): no corresponding node or pipe exists (pipe %X)\r\n",pipe_id);
#endif
			return FAIL;
		}
		else
		{
#ifdef DEBUG_INDICATOR
			my_printf("repair_pipe(): no correponding node exists for pipe %X, extract script from pipe mnpl database.\r\n",pipe_id);
#endif
			script_length = get_path_script_from_pipe_mnpl_db(pipe_id, script_buffer);
		}
		use_topology = FALSE;
	}

	/************************************************************************
	1. ����script_buffer����pipe_id��pipe, ���ڽ����һ���洢��pipe��ʧ�����
	2. ����patrol���ܼ��һ�¸���SNR��ԣ��, ����ԣ�ȵ�����ֵ�Ļ���, ��ǿ֮
	3. ���������Ϲ��ܽ���Ķ�����΢�����pipe, Ӧ���ܽ���󲿷�����, ������ܽ��, �������һ������
	************************************************************************/
	if(script_length)
	{
		BOOL patrol_buffer_valid = FALSE;
	
		status = psr_setup(phase, pipe_id, script_buffer, script_length);
		if(status)
		{
			/* ʹ��patrol�������PATH������� */
			patrol_buffer_len = psr_patrol(phase, pipe_id, patrol_buffer);
			if(patrol_buffer_len)
			{
				patrol_buffer_valid = TRUE;
			}
			else
			{
				status = FAIL;
			}
		}

		//����ܹ��õ�patrol���,����Կ��ٶ�λ�Ӵ�������·��, ��ǿ�������� */
		if(patrol_buffer_valid)
		{
			for(i=0;i<stages;i++)
			{
				downlink_ss[i] = patrol_buffer[i*9+6];
				downlink_snr[i] = patrol_buffer[i*9+7];
				downlink_mode[i] = script_buffer[i*8+6];
			}
			for(i=stages;i<stages*2;i++)
			{
				uplink_ss[2*stages-1-i] = patrol_buffer[i*9+6];
				uplink_snr[2*stages-1-i] = patrol_buffer[i*9+7];
				uplink_mode[i-stages] = script_buffer[(i-stages)*8+7];
			}
			
			my_printf("patrol return buffer:");
			uart_send_asc(patrol_buffer,patrol_buffer_len);
			my_printf("\r\ndownlink snr:");
			uart_send_asc(downlink_snr,stages);
			my_printf("\r\nuplink snr:");
			uart_send_asc(uplink_snr,stages);
			my_printf("\r\n");
		}

		for(i=0;i<stages;i++)
		{
			if(patrol_buffer_valid)
			{
				test_link.downlink_mode = downlink_mode[i];
				test_link.downlink_snr = downlink_snr[i];
				test_link.downlink_ss = downlink_ss[i];
				test_link.uplink_mode = uplink_mode[i];
				test_link.uplink_snr = uplink_snr[i];
				test_link.uplink_ss = uplink_ss[i];
				test_link.ptrp = calc_link_ptrp(downlink_mode[i],downlink_snr[i],uplink_mode[i],uplink_snr[i],NULL);
			}

			/*�����patrol���ݿɹ��ο�, ��ֻ�Ż������Ļ���, ��û��patrol���ݹ��ο�, ��һ��һ����ͨ*/
			if(!patrol_buffer_valid || (patrol_buffer_valid && !judge_optimized_link(&test_link)))
			{
				if(i==0)
				{
					status = get_optimized_link(network_tree[phase]->uid,0,&script_buffer[i*8],RATE_BPSK,&test_link,&err_code);
				}
				else
				{
					my_printf("setup a temporatary pipe\r\n");
					status = psr_setup(phase,TEST_PIPE_ID,script_buffer,i*8);
					if(status)
					{
						status = get_optimized_link(NULL,TEST_PIPE_ID,&script_buffer[i*8],RATE_BPSK,&test_link,&err_code);
					}
				}

				if(status)
				{
					// �ı�pipe script
					script_buffer[i*8+6] = test_link.downlink_mode;
					script_buffer[i*8+7] = test_link.uplink_mode;

					//���ʹ������, ����������е���·��Ϣ
					if(use_topology)
					{
						update_topology_link(uid2node(&script_buffer[i*8]),&test_link);
					}
				}
				else
				{
					break;
				}
			}
		}

		if(status)
		{
			status = psr_setup(phase, pipe_id, script_buffer, script_length);			//��ˢһ���µ�pipe
		}
	}

#ifdef DEBUG_INDICATOR
	if(status)
	{
		my_printf("repair success.\r\n");
	}
	else
	{
		my_printf("repair fail.\r\n");
	}
#endif

	return status;
}


/*******************************************************************************
* Function Name  : optimize_path_link
* Description    : �Ż�PATH��ÿһ����·, ���ڽ����II������
					����Ĵ��������path��·������,������pipe,��Ϊ��Ҫ�Ż���find�����õ����½ڵ�·��,��ʱ��û��pipe
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS optimize_path_link(u8 phase, ARRAY_HANDLE script_buffer, u8 script_length)
{
	u8 i;
	LINK_STRUCT test_link;
	STATUS status = FAIL;
	u8 err_code;

	/* check sript format */
	if((script_length<8) || (((script_length>>3)<<3) != script_length))
	{
#ifdef DEBUG_INDICATOR
		my_printf("error script format\r\n");
#endif
		return FAIL;
	}

	/* ���Ż���· */
	for(i=0;i<script_length/8;i++)
	{
		if(i==0)
		{
			status = get_optimized_link_from_node_to_uid(network_tree[phase], &script_buffer[i*8], RATE_BPSK, &test_link, &err_code);
		}
		else
		{
			status = get_optimized_link_from_pipe_to_uid(TEST_PIPE_ID, &script_buffer[i*8], RATE_BPSK, &test_link, &err_code);
		}

		
		if(status)
		{
			script_buffer[i*8+6] = test_link.downlink_mode;
			script_buffer[i*8+7] = test_link.uplink_mode;
			status = psr_setup(phase,TEST_PIPE_ID,script_buffer,(i+1)*8);
		}

		if(!status)
		{
			break;
		}
	}

#ifdef DEBUG_INDICATOR
	if(status)
	{
		my_printf("path optimized successfully. new path script is\r\n");
		uart_send_asc(script_buffer,script_length);
		my_printf("\r\n");
	}
	else
	{
		my_printf("path optimized fail.\r\n");
	}
#endif

	return status;
}

/*******************************************************************************
* Function Name  : optimize_pipe_link
* Description    : �Ż�PIPE��ÿһ����·, ���ڽ����II������
					·����������pipe_mnpl_db, ���������˿�
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
//STATUS optimize_pipe_link(u16 pipe_id)
//{
//	u8 script_buffer[64];
//	u8 script_length;
//	PIPE_REF pipe_ref;
////	ARRAY_HANDLE ptw;
//	STATUS status;
//
//	script_length = get_path_script_from_pipe_mnpl_db(pipe_id,script_buffer);
//	if(!script_length)
//	{
//		status = FAIL;
//	}
//	else
//	{
//#ifdef DEBUG_INDICATOR
//		my_printf("optimize_pipe_link():old pipe script:\r\n");
//		uart_send_asc(script_buffer, script_length);
//		my_printf("\r\n");
//#endif
//		status = optimize_path_link(pipe_ref->phase, script_buffer, script_length);
//
//		if(status)
//		{
//#ifdef DEBUG_INDICATOR
//			my_printf("optimize_pipe_link():new pipe script:\r\n");
//			uart_send_asc(script_buffer, script_length);
//			my_printf("\r\n");
//#endif
//			status = psr_setup(pipe_ref->phase, pipe_id, script_buffer, script_length);
//		}
//	}
//	
//	return status;
//}

/*******************************************************************************
* Function Name  : dst_find_xid
* Description    : ��dst flooding search��ʽ�����ڵ�, ֧�ֱ������,
*					�ڵ�����������д��ڵĽڵ�Ҳ�������½ڵ�
*					�������½ڵ���bpsk��ʼ����,������,Ȼ��ds15ֱ����,������,
*					��ʧ�ܺ�,bpskһ������
*					����������ʱ��6��������,�о�����̫����,����
* Author		 : Lv Haifeng
* Date			 : 2014-10-13
* Input          : 
* Output         : 
* Return         : �ɹ��򷵻ض�Ӧ pipe_id, ʧ���򷵻�0
*******************************************************************************/
u16 dst_find_xid(ARRAY_HANDLE target_xid, BOOL is_meter_id)
{
	NODE_HANDLE target_node;
	UID_HANDLE target_uid;
	u8 r,j,p;
	u8 phase;
	u8 rate;
	u8 jumps;
	u8 windows;
	u8 acps_constrained;
	u16 phase_nodes_cnt;
//	BOOL is_node_existed_in_topology = FALSE;
	u8 path_script[CFG_PIPE_STAGE_CNT * 8];
	u8 path_script_len;
	STATUS status = FAIL;
	u16 pipe_id = 0;

	/*** ȷ����ʼ��������:********************
	* meter_id: �ӻ�����ʼ����
	* ���������е�uid: �ӻ�����ʼ����
	* �������е�uid: ��������ԭ��������ʼ����
	*****************************************/
	if(is_meter_id)
	{
		phase = 0;
		rate = RATE_BPSK;
		jumps = 0;
		phase_nodes_cnt = get_topology_subnodes_cnt(phase);
		windows = get_dst_optimal_windows(phase_nodes_cnt);
		acps_constrained = 1;
	}
	else
	{
		target_node = uid2node(target_xid);
		if(!target_node)
		{
			phase = 0;
			rate = RATE_BPSK;
			jumps = 0;
			phase_nodes_cnt = get_topology_subnodes_cnt(phase);
			windows = get_dst_optimal_windows(phase_nodes_cnt);
			acps_constrained = 1;
		}
		else
		{
			phase = get_node_phase(target_node);
			rate = RATE_BPSK;
			jumps = get_node_depth(target_node);
			if(jumps > 0)
			{
				jumps -= 1;						//���ڵ����Ϊ0, һ���ڵ����Ϊ1, ����Ϊ0
			}
			phase_nodes_cnt = get_topology_subnodes_cnt(phase);
			windows = get_dst_optimal_windows(phase_nodes_cnt);
			if(jumps == 0)
			{
				acps_constrained = 1;
			}
			else
			{
				acps_constrained = 1;
			}

//			is_node_existed_in_topology = TRUE;
		}
	}

#ifdef DEBUG_INDICATOR
	my_printf("dst_find_xid():phase%bu,rate%bu,jumps%bu,windows%bu,acps%bu\r\n",phase,rate,jumps,windows,acps_constrained);
	if(is_meter_id)
	{
		my_printf("dst_find_xid():target_mid");
	}
	else
	{
		my_printf("dst_find_xid():target_uid");
	}
	uart_send_asc(target_xid,6);
	my_printf("\r\n");
	
#endif

	// �������ԵĲ���: ��������>��������>����
	for(j=0;j<2;j++)
	{
		//����
		for(r=0;r<2;r++)
		{
			// ÿ����������, ���ʧ��, ��������
			for(p=0; p<CFG_PHASE_CNT; p++)
			{
				u8 use_phase = (phase+p) % CFG_PHASE_CNT;
				u8 use_rate = rate + r;
				u8 use_jumps = jumps + j;

				//Ŀǰ��ֹDS15���ϵ�ͨ�ŷ�ʽ�Ժ鷺����
				if(!(use_rate>RATE_BPSK && use_jumps>=1))
				{
					status = flooding_search(use_phase, target_xid, is_meter_id, use_rate, use_jumps, windows, acps_constrained, path_script, &path_script_len);
					if(!status)
					{
						status = flooding_search(use_phase, target_xid, is_meter_id, use_rate, use_jumps, windows, acps_constrained, path_script, &path_script_len);
					}
				}

				if(status)
				{
					status = optimize_path_link(phase,path_script,path_script_len);
					if(status)
					{
						break;
					}
					else
					{
						if(!acps_constrained)
						{
							phase = (phase+1)%CFG_PHASE_CNT;		//���������·�����Ż�ʧ��,�����acpsû��Լ��,���������λ���Ի�������
						}
					}
				}
			}

			if(status)
			{
				break;
			}
		}
		if(status)
		{
			break;
		}
	}

	/* Ϊָ��·������pipe */
	if(status)
	{
		target_uid = &path_script[path_script_len-8];
		if(!is_meter_id)
		{
			if(!mem_cmp(target_xid, target_uid, 6))
			{
#ifdef DEBUG_INDICATOR
				my_printf("dst_find_xid():destination of returned path script error.\r\n");
#endif
				return 0;
			}
		}

		status = sync_path_script_to_topology(phase, path_script, path_script_len);

		if(status)
		{
			pipe_id = uid2pipeid(target_uid);
			if(!pipe_id)
			{
				pipe_id = get_available_pipe_id();
			}

			status = psr_setup(phase, pipe_id, path_script, path_script_len);

			if(status)
			{
#ifdef DEBUG_INDICATOR
				my_printf("target find successfully. pipe: %x\r\n",pipe_id);	
#endif
			}
		}
	}

	if(!status)
	{
#ifdef DEBUG_INDICATOR
		my_printf("search fail.\r\n");
#endif
	}
	return pipe_id;
}


/*******************************************************************************
* Function Name  : rebuild_path
* Description    : ��ȷ��ԭ��·����ʧЧʱ, ����������·��, �����III������
*					������������ԭ��λ��BPSKģʽ��������
*					�����Ե�ǰ��λ�Ͻڵ�����, ����BPSK & ������32 & ��Լ����λ & �����PIPEһ��
*					��һ������ʧ�������+1��������;
*					�ڶ�������ʧ�������+1��������;
*					����������ʧ������λ��������;
*					���Ĵ�����ʧ�����ٻ���λ��������;
*					һ�������ɹ�, ����ȷ����λ, �������Ϊ1����ֱ�ӵ�Ե��ж�. ����������1�ø�PIPE�㲥��ýڵ�, ���ݽڵ����λ�жϸýڵ���λ
*					����ýڵ���λ����, ��
* Author		 : Lv Haifeng
* Date			 : 2014-10-13
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS rebuild_pipe(u16 pipe_id, u8 config)
{
	PIPE_REF pipe_ref;
	u8 target_uid[6];
	u8 script_buffer[CFG_PIPE_STAGE_CNT * 8];
	u8 script_length;
	u8 phase;
	u8 jumps;
	u8 rate;
	u8 window;
	STATUS status;

#ifdef DEBUG_INDICATOR
	my_printf("rebuild pipe %x\r\n",pipe_id);
#endif

	pipe_ref = inquire_pipe_mnpl_db(pipe_id);
	if(!pipe_ref)
	{
#ifdef DEBUG_INDICATOR
		my_printf("Try to rebuild a pipe (%x) not existed.\r\n",pipe_id);
#endif
		return FAIL;
	}

	phase = get_pipe_phase(pipe_id);
	acquire_target_uid_of_pipe(pipe_id, target_uid);



	/* ����ؽ����������ԭpipe������ͬ, Ҳ���Լ�1 */
	if(config & REBUILD_PATH_CONFIG_LONGER_JUMPS)
	{
		jumps = inquire_pipe_stages(pipe_id) + 1;
	}
	else
	{
		jumps = inquire_pipe_stages(pipe_id);
	}
#ifdef DEBUG_INDICATOR
	my_printf("dst jump:%bu",jumps);
#endif

	/* ʹ�õ�����������·���ϵ�����·�����ʾ��� */
	{
		u8 i;
		u8 max_rate = 0;
		for(i=0;i<pipe_ref->pipe_stages;i++)
		{
			u8 xmode;
			u8 rmode;
			u8 temp;

			temp = pipe_ref->xmode_rmode[i];
			xmode = decode_xmode(temp>>4);
			rmode = decode_xmode(temp&0x0F);
			if(max_rate<(xmode&0x0F))
			{
				max_rate = (xmode&0x0F);
			}

			if(max_rate<(rmode&0x0F))
			{
				max_rate = (rmode&0x0F);
			}
			
		}

		rate = max_rate;
		if((config & REBUILD_PATH_CONFIG_SLOWER_RATE) && (max_rate<RATE_DS63))
		{
			rate = max_rate + 1;
		}
	}
#ifdef DEBUG_INDICATOR
		my_printf("dst rate:%bu\r\n",rate);
#endif


	window = dst_config_obj.forward_prop & BIT_DST_FORW_WINDOW;
#ifdef DEBUG_INDICATOR
	my_printf("dst window:%bu\r\n",window);
#endif	
	status = flooding_search(phase, target_uid, 0, rate, jumps, window, 1, script_buffer, &script_length);

	/* if search failed, seach the node in other phase */
#if CFG_PHASE_CNT>1
	if(!status && (config & REBUILD_PATH_CONFIG_TRY_OTHER_PHASE))
	{
		u8 i;
		
		if(CFG_PHASE_CNT>1)
		{
			for(i=0;i<CFG_PHASE_CNT-1;i++)
			{
				if(phase>=(CFG_PHASE_CNT-1))
				{
					phase = 0;
				}
				else
				{
					phase++;
				}
	#ifdef DEBUG_INDICATOR
				my_printf("try phase %bd\r\n",phase);
	#endif
				status = flooding_search(phase, target_uid, 0, rate, jumps, dst_config_obj.forward_prop & BIT_DST_FORW_WINDOW, 0, script_buffer, &script_length);
				if(status)
				{
					break;
				}
			}
		}
	}
#endif
	/*  */
	if(status)
	{
		status = optimize_path_link(phase, script_buffer, script_length);
	}

	if(status)
	{
		status = psr_setup(phase, pipe_id, script_buffer, script_length);
	}

	if(status && (config & REBUILD_PATH_CONFIG_SYNC_TOPOLOGY))
	{
		status = sync_path_script_to_topology(phase, script_buffer, script_length);
	}

	return status;
}


/*******************************************************************************
* Function Name  : test_pipe_connectivity
* Description    : pipe������ͨ��, ������ping 2�γɹ�Ϊ׼
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS test_pipe_connectivity(u16 pipe_id)
{
	u8 i;
	u8 return_buffer[64];
	STATUS status;

	for(i=0;i<2;i++)
	{
		status = mgnt_ping(pipe_id, return_buffer);
		if(!status)
		{
			break;
		}
		SET_BREAK_THROUGH("quit test_pipe_connectivity()\r\n");
	}

	return status;
}

/*******************************************************************************
* Function Name  : flooding_search
* Description    : ��dst_search��������uid
* Input          : 
* Output         : 
* Return         : OKAY/FAIL
*******************************************************************************/
STATUS flooding_search(u8 phase, ARRAY_HANDLE target_xid, u8 is_target_mid, u8 rate, u8 max_jumps, u8 max_window_index, u8 acps_constrained, ARRAY_HANDLE script_buffer, u8 * script_length)
{
	DST_SEND_STRUCT dst;
	DST_CONFIG_STRUCT dst_backup;
	u8 dst_buffer[32];
	u32 expiring_sticks;
	TIMER_ID_TYPE tid;
	STATUS status = FAIL;
	SEND_ID_TYPE sid;

	if(acps_constrained)
	{
		if(!is_zx_valid(phase))
		{
#ifdef DEBUG_INDICATOR
			my_printf("phase %bu zx invalid\r\n",phase);
#endif
			return FAIL;
		}
	}

	tid = req_timer();
	if(tid==INVALID_RESOURCE_ID)
	{
		return FAIL;
	}


	mem_cpy((u8*)&dst_backup,(u8*)&dst_config_obj,sizeof(DST_CONFIG_STRUCT));	//����dstȫ������,�Ա���Ӱ��APP���DST
	config_dst_flooding(rate, max_jumps, max_window_index,acps_constrained,0);					//����ͬ��λ, ����������

	mem_cpy(dst_buffer,target_xid,6);
	dst.phase = phase;
	dst.search = 1;
	dst.forward = 0;
#if DEVICE_TYPE==DEVICE_CC || DEVICE_TYPE==DEVICE_CV
	dst.search_mid = is_target_mid?1:0;
#endif
	dst.nsdu = dst_buffer;
	dst.nsdu_len = 6;
#ifdef DEBUG_INDICATOR
	my_printf("dst search,phase=%bx,mode=%bx,jumps=%bx,window=%bx\r\n",phase,dst_config_obj.comm_mode,dst_config_obj.jumps&0x0F,(dst_config_obj.forward_prop&BIT_DST_FORW_WINDOW));
#endif

	expiring_sticks = dst_flooding_search_sticks();
#ifdef USE_MAC
	/* �鷺ռ��ʱ��̫��, ��ò�Ҫʹ��nav, ����Ч��̫�� */
	//declare_channel_occupation(dst.phase,phy_trans_sticks(dst.nsdu_len + LEN_TOTAL_OVERHEAD_BEYOND_LSDU, dst.xmode & 0x03, ds.prop & BIT_DLL_SEND_PROP_SCAN) + expiring_sticks);
#endif
	
	sid = dst_send(&dst);
	wait_until_send_over(sid);
	set_timer(tid,expiring_sticks);
#ifdef DEBUG_INDICATOR
	my_printf("to wait=%ld sticks\r\n",expiring_sticks);
#endif

	do
	{
		u8 i;
		DST_STACK_RCV_HANDLE pdst;
		DLL_RCV_HANDLE pd;

		SET_BREAK_THROUGH("quit flooding_search()\r\n");
		for(i=0;i<CFG_PHASE_CNT;i++)
		{
			pdst = &_dst_rcv_obj[i];
			pd = GET_DLL_HANDLE(pdst);
			
			if(pdst->dst_rcv_indication)
			{
				u8 dst_conf;

				dst_conf = pd->lpdu[SEC_LPDU_DST_CONF];
			
				if((pdst->phase == phase) && pdst->uplink && (dst_conf & BIT_DST_CONF_SEARCH))
				{
					
					get_path_script_from_dst_search_result(pdst->apdu,pdst->apdu_len,script_buffer,script_length);
					if(!is_target_mid && (!mem_cmp(pdst->apdu,&script_buffer[*script_length-8],6)))
					{
						status = FAIL;
#ifdef DEBUG_INDICATOR
						my_printf("search uid not match\r\n");
						uart_send_asc(pdst->apdu,pdst->apdu_len);
						my_printf("\r\n");
#endif
					}
					else
					{
						status = OKAY;
					}
#ifdef DEBUG_INDICATOR
					{
						u32 left_sticks;

						left_sticks = get_timer_val(tid);
						my_printf("got search return, left sticks:%ld, wait until flooding is over...\r\n",left_sticks);
					}
#endif
				}
				dst_rcv_resume(pdst);
			}
		}
		FEED_DOG();
	}while(!is_timer_over(tid) /*&& !status*/);		//�ȴ�ֱ���鷺����

	mem_cpy((u8*)&dst_backup,(u8*)&dst_config_obj,sizeof(DST_CONFIG_STRUCT));	//��ԭdstȫ������

	delete_timer(tid);

#ifdef DEBUG_INDICATOR
	if(status)
	{
		my_printf("search successfully,searched path:\r\n");
		uart_send_asc(script_buffer,*script_length);
		my_printf("\r\n");
	}
	else
	{
		my_printf("search fail\r\n");
	}
#endif
	
	return status;
}

/*******************************************************************************
* Function Name  : find_uid
* Description    : ��������Ѱ��uid�ڵ�/mid�ڵ�, ���ص����uid��pipe
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u16 dst_find(u8 phase, u8 * target_xid, u8 find_mid)
{
	PIPE_REF pipe_ref;
	u16 pipe_id = 0;
	u8 script_buffer[64];
	u8 script_length;
	u8 jumps;
	u8 rate;
	u8 window;
	STATUS status;
	ARRAY_HANDLE target_uid;

	rate = dst_config_obj.comm_mode;
	jumps = dst_config_obj.jumps & 0x0F;
	window = dst_config_obj.forward_prop & BIT_DST_FORW_WINDOW;

	/* ��ָ����λѰ��Ŀ��ڵ� */
#ifdef DEBUG_INDICATOR
	my_printf("dst_find():find uid/mid with dst rate:%bx, jump:%bx, window:%bx\r\n",rate,jumps,window);
#endif
	status = flooding_search(phase, target_xid, find_mid, rate, jumps, window, 1, script_buffer, &script_length);

	/* ��ɹ�, �Ż�·�� */
	if(status)
	{
		status = optimize_path_link(phase,script_buffer,script_length);
	}

	/* Ϊָ��·������pipe */
	if(status)
	{
		target_uid = &script_buffer[script_length-8];
		if(!find_mid)
		{
			if(!mem_cmp(target_xid, target_uid, 6))
			{
#ifdef DEBUG_INDICATOR
				my_printf("dst_find():destination of returned path script error.\r\n");
#endif
				return 0;
			}
		}

		pipe_ref = inquire_pipe_mnpl_db_by_uid(target_uid);
		
		if(!pipe_ref || ((pipe_ref->pipe_info & 0x0FFF) == TEST_PIPE_ID))
		{
			pipe_id = get_available_pipe_id();
#ifdef DEBUG_INDICATOR
			my_printf("dst_find():assign new pipe id %x to target.\r\n", pipe_id);
#endif
		}
		else
		{
			pipe_id = pipe_ref->pipe_info & 0x0FFF;
#ifdef DEBUG_INDICATOR
			my_printf("dst_find():exist pipe id %x to target.\r\n", pipe_id);
#endif
		}
#ifdef DEBUG_INDICATOR
		my_printf("dst_find():setup pipe\r\n");
#endif
		status = psr_setup(phase, pipe_id, script_buffer, script_length);
	}

	if(status)
	{
#ifdef DEBUG_INDICATOR
		my_printf("target find successfully pipe: %x\r\n",pipe_id);
#endif
		return pipe_id;
	}
	else
	{
#ifdef DEBUG_INDICATOR
		my_printf("target find fail\r\n");
#endif
		return 0;
	}
}

/*******************************************************************************
* Function Name  : get_path_script_from_dst_search_result
* Description    : dst flooding search���صĸ�ʽ
					[target_uid(target_mid) conf1 relay_uid1 conf2 relay_uid2 ... confi target_uid conf(i+1) relay_uid(n+1)... confn relay_uidn]
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS get_path_script_from_dst_search_result(ARRAY_HANDLE nsdu, BASE_LEN_TYPE nsdu_len,ARRAY_HANDLE script_buffer, u8 * script_length)
{
	UID_HANDLE pt_search;
	UID_HANDLE ptw,ptr;
	u8 down_stage;
	u8 up_stage;
	u8 i;

	if((nsdu_len<13)||((nsdu_len%7)!=6))
	{
#ifdef DEBUG_INDICATOR
		my_printf("wrong dst return format\r\n");
#endif
		*script_length=0;
		return FAIL;
	}

	// ��Я����relay conf���ж�Ŀ��ڵ������λ��
	for(pt_search = nsdu + 6; (pt_search-nsdu)<nsdu_len; pt_search+=7)
	{
		if(*pt_search & BIT_DST_CONF_UPLINK)
		{
			pt_search-=6;	//ָ��target uid
			break;
		}
	}

	// ���Ҳ���uplink����ʼ��, ��Ϊ���һ���ڵ�uid����target, ��Ӧ�������о�һ�������
	if((pt_search-nsdu)>=nsdu_len)
	{
		pt_search = &nsdu[nsdu_len-6];
	}	

	// pipeֻ֧��˫��, ��search ������·���������п��ܲ��Գ�, ѡ��϶̵�һ��
	down_stage = ((u8)(pt_search-1-nsdu)-6)/7 + 1;
	up_stage = (nsdu_len-(u8)((u32)pt_search-1-(u32)nsdu))/7;
	if(down_stage<=up_stage)
	{
		ptw = script_buffer;
		ptr = nsdu+7;

		for(i=0;i<down_stage;i++)
		{
			mem_cpy(ptw,ptr,6);
			ptw += 6;
			ptr += 7;
			*ptw++ = 0x00;
			*ptw++ = 0x00;
		}
	}
	else
	{
		ptw = script_buffer;
		ptr = nsdu + nsdu_len - 6;

		for(i=0;i<up_stage;i++)
		{
			mem_cpy(ptw,ptr,6);
			ptw += 6;
			ptr -= 7;
			*ptw++ = 0x00;
			*ptw++ = 0x00;
		}
	}
	*script_length = (u8)(ptw-script_buffer);
	return OKAY;
}

#define OPTIMIZING_PROC_DISCONNECT	0
#define OPTIMIZING_PROC_UPGRADE		1

/*******************************************************************************
* Function Name  : powermesh_optimizing_proc
* Author		 : Lv Haifeng
* Date			 : 2014-10-16
* Description    : �����Ż����ܺ���, ���ڿ���ʱ����������, ��ҪӦ�ò��ڿ���ʱѭ������
					ÿ��ִֻ��һ�β���.

					2015-09-30
					����upgrade�Ż�, ÿ�ε��û��ߴ���disconncet, ���ߵ���upgrade
					�������ʲô���������㷨
					ÿ�θ��ݲ�ͬ���������ֵ��Сȷ������˭,��Ӧ��������0,û����Ӧ����������N
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void powermesh_optimizing_proc(void)
{
	u8 i;
	u16 disconnect_usage;
	u16 j,k;
	static u16 desire_disconnect_proc = 0;
	static u16 desire_upgrade_proc = 0;
	u8 proc_decision;

	/* disconnect node proc */
	disconnect_usage = 0;
	for(i=0;i<CFG_PHASE_CNT;i++)
	{
		disconnect_usage += get_list_from_branch(network_tree[i],ALL,NODE_STATE_DISCONNECT, NODE_STATE_EXCLUSIVE, disconnect_list, disconnect_usage, CFG_MGNT_MAX_NEW_LINKS_BUFFER_CNT);
	}

	// 2015-09-25 ���˵�����޸�ʧ�ܵĽڵ�
	for(j=0;j<disconnect_usage;j++)
	{
		if(disconnect_list[j]->alternative_progress_fail)
		{
			for(k=j; k<disconnect_usage-1; k++)
			{
				disconnect_list[k] = disconnect_list[k+1];
			}
			disconnect_usage--;
		}
	}

	// 2015-09-30 ʹ�������㷨,ȷ��ִ�е�����
	desire_disconnect_proc += disconnect_usage;			//disconnect���������ֵÿ�����ӵ��ǵ�ǰʧ���ڵ�����
	desire_upgrade_proc += 1;							//upgrade����ÿ������ֵ��1

	if(desire_disconnect_proc>=desire_upgrade_proc)
	{
		proc_decision = OPTIMIZING_PROC_DISCONNECT;
		desire_disconnect_proc = 0;
	}
	else
	{
		proc_decision = OPTIMIZING_PROC_UPGRADE;
		desire_upgrade_proc = 0;
	}

	// ִ����Ӧ����
	switch(proc_decision)
	{
		case(OPTIMIZING_PROC_DISCONNECT):
		{
			powermesh_optimizing_disconnect_proc(disconnect_usage);		//��û��ʧ���ڵ���Ҫ����, ����ڵ��Ż���������
			break;
		}
		case OPTIMIZING_PROC_UPGRADE:
		{
			powermesh_optimizing_update_proc();		//��û��ʧ���ڵ���Ҫ����, ����ڵ��Ż���������
			break;
		}
	}
}

/*******************************************************************************
* Function Name  : enable_network_optimizing
* Description    : ��������ʼ����ʱ(·����ͣ), ��ֹ�����Ż�, ������ֹͣ����ʱ(·������), ʹ�������Ż�
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void enable_network_optimizing(u8 enable_option)
{
	if(enable_option)
	{
#ifdef DEBUG_INDICATOR
		my_printf("enable powermesh network optimizing.\r\n");
#endif
		network_optimizing_switch = TRUE;
	}
	else
	{
#ifdef DEBUG_INDICATOR
		my_printf("enable powermesh network optimizing.\r\n");
#endif
		network_optimizing_switch = FALSE;
		set_quit_loops();			//ֹͣһ�к�ʱ����, �ص����߳�
	}
}

/*******************************************************************************
* Function Name  : is_network_optimizing_enable
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
BOOL is_network_optimizing_enable(void)
{
	return (network_optimizing_switch);
}

/*******************************************************************************
* Function Name  : psr_find()
* Description    : 
* Input          : 
* Output         : 
* Return         : ͨ�ųɹ�����SUCCESS, pipe_id��ָ�뷵��
*******************************************************************************/
STATUS psr_find_xid(NODE_HANDLE exe_node, ARRAY_HANDLE target_xid, BOOL is_meter_id, u8 use_rate, u16 * return_pipe_id)
{
	u8 phase;
	EBC_BROADCAST_STRUCT es;
	STATUS status = FAIL;
	u8 resp_num;
	u8 target_uid[6];
	u16 pipe_id;

	phase = get_node_phase(exe_node);
	if((u8)INVALID_RESOURCE_ID == phase)
	{
#ifdef DEBUG_INDICATOR
		my_printf("exe_node not existed.\r\n");
#endif
		return FAIL;
	}
#ifdef DEBUG_INDICATOR
	my_printf("psr_find_xid:finding [");
	uart_send_asc(target_xid,6);
	my_printf("]");
	if(is_meter_id)
	{
		my_printf("(mid)");
	}
	my_printf(" around [");
	uart_send_asc(exe_node->uid,6);
	my_printf("]...\r\n");
#endif

	*return_pipe_id = 0;
	mem_clr(&es, sizeof(es), 1);
	es.phase = phase;
	es.bid = 0;
	es.xmode = use_rate|0x10;
	es.rmode = use_rate|0x10;
	es.scan = 1;
	es.window = 0;				//�����ض�id, ֻ��һ�����ڼ���
	es.mask = is_meter_id?(BIT_NBF_MASK_METERID|BIT_NBF_MASK_ACPHASE):(BIT_NBF_MASK_UID|BIT_NBF_MASK_ACPHASE);
	es.max_identify_try = 2;
	if(is_meter_id)
	{
		mem_cpy(es.mid,target_xid,6);
	}
	else
	{
		mem_cpy(es.uid,target_xid,6);
	}

	/******************************
	* step1.
	* ʹ��EBC�㲥�ٻ��ڵ�, ������1������
	******************************/
	if(is_root_node(exe_node))
	{
		resp_num = root_explore(&es);
		if(resp_num == 0)
		{
#ifdef DEBUG_INDICATOR
			my_printf("root find no resp.\r\n");
#endif
			return OKAY;
		}
	}
	else
	{
		u16 exe_pipe_id;

		exe_pipe_id = node2pipeid(exe_node);
		if(exe_pipe_id)
		{
			status = psr_explore(exe_pipe_id, &es, &resp_num);

			if(status == FAIL)
			{
				status = psr_explore(exe_pipe_id, &es, &resp_num);
				if(status == FAIL)						//psrͨ��ʧ��, ����һ��
				{
#ifdef DEBUG_INDICATOR
					my_printf("psr comm fail.\r\n");
#endif
					set_node_state(exe_node,NODE_STATE_DISCONNECT);
					return FAIL;
				}
			}

			if((status == OKAY) && resp_num == 0)		//ͨ�ųɹ�, ȷ��û�ҵ�
			{
#ifdef DEBUG_INDICATOR
				my_printf("exe node find no resp.\r\n");
#endif
				return OKAY;
			}
		}
		else											// ��Щ�ڵ�������ʱ���ܽ��Ǳ�����,��û�н�������pipe
		{
#ifdef DEBUG_INDICATOR
			my_printf("no available comm pipe.\r\n");
#endif
			return FAIL;
		}
	}

	/**************************************************
	* step2.
	* ѵ���ŵ�
	**************************************************/
	if(is_root_node(exe_node))
	{
		status = inquire_neighbor_uid_db(0, target_uid);
	}
	else
	{
		mem_cpy(target_uid,&psr_explore_new_node_uid_buffer[2],6);
	}

	if(is_meter_id)
	{
#ifdef DEBUG_INDICATOR
		my_printf("psr_find_xid():target meter located in node with uid [");
		uart_send_asc(target_uid,6);
		my_printf("]\r\n");
#endif
	}
	else
	{
		if(!mem_cmp(target_uid,target_xid,6))
		{
#ifdef DEBUG_INDICATOR
			my_printf("fatal error: target xid is not match\r\n");
#endif
			return FAIL;
		}
	}
	
	// add new nodes, setup pipe, ping, set build_id...
	if(status)
	{
		status = optimization_process_explored_uid(exe_node, target_uid);	//2015-10-29 ����optimization_process_explored_uid()����ԭ����process_explored_uid(),��ping,��setup_buildid
	}

	if(status)
	{
		pipe_id = uid2pipeid(target_uid);
		*return_pipe_id = pipe_id;
	}

	return status;
}


/*******************************************************************************
* Function Name  : powermesh_optimizing_disconnect_proc()
* Description    : ����ʧ���ڵ�
					ִ�в���:
					1. ��������һ���޸���ͨ�Ľڵ�, Ҫ����Ľڵ����;
					2. ���������޸����ɹ��Ľڵ��ʱ��Ͳ�����Ľڵ��Ż��Ļ���; ���Կ��ǽ�������ѡ������
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void powermesh_optimizing_disconnect_proc(u16 disconnect_usage)
{
	NODE_HANDLE node_proc;
	STATUS status;
	u16 pipe_id;

	if(disconnect_usage)
	{
		randomize_list(disconnect_list,disconnect_usage);		//ÿ�����ִ��, ������һ���ڵ�������. ȱ��: �п��ܸı�������, ���»��ݴ���������
		node_proc = disconnect_list[0];

#ifdef DEBUG_INDICATOR
		my_printf("powermesh_optimizing_proc(): deal disconnect node:");
		uart_send_asc(node_proc->uid, 6);
		my_printf("\r\n");
#endif

		/********************************************************
		����ڵ����Ǳ�����, ���Ǳ����ó�disconnect, Ҳ����ؽ�·��, �����Ǽ�������
		2015-04-21 �������, �ڵ����һ���޶���, �����������нڵ㶼�ܲ�����̫��, �����ǲ��Ե�ʱ��, ϲ���ѽڵ�Ų��Ųȥ, ���п��ܱ�����������
					ֻ��2��, �����ǽ�һ�����ӽڵ�Ľڵ�Ų����2��, �����㷨, ���ӽڵ�ͳ���3���ڵ�, �Ͳ�����Ҫ����.
					����Ĵ�����, �������ֱ�Ų��ȥ���ӽڵ��Ѿ��������ó���disconnect,��ô���Ż�ʱ,������ȴ���Ҫ��Ľڵ�,�Ͳ���repair��,��������ѡ���ؽ�·��
					ͬʱ,�����ؽ�·���ֲ��ı���ԭ���Ľڵ��ϵ,���ԭ3��·��������,ͬʱ��ȡ�ҵ����̵�·��.
		********************************************************/
		if((node_proc->set_disconnect_cnt < CFG_MAX_DISCONNECT_REPAIR) &&
			(node_proc->repair_fail_cnt < CFG_MAX_OPTIMAL_PROC_REPAIR) &&
			(get_node_depth(node_proc) <= get_max_build_depth()))				//2015-04-21 ��������, ���"����ƿ"�ڵ㳬�����������������
		{
			node_proc->alternative_progress_phase = get_node_phase(node_proc);
			status = repair_pipe(node2pipeid(node_proc));
			if(status)
			{
				clr_node_state(node_proc, NODE_STATE_DISCONNECT);
				set_node_state(node_proc, NODE_STATE_RECONNECT);	//2015-10-20 ����Ҫ�������޸�״̬
				node_proc->grade_drift_cnt = 0;						//2015-11-11 �����������ͻؿ���
				node_proc->repair_fail_cnt = 0;
			}
			else
			{
#ifdef DEBUG_INDICATOR
				my_printf("repair failed\r\n"); 	
#endif
				node_proc->repair_fail_cnt++;
			}
		}

		/*
			���ڶ�α���,����һֱ�����õĵ�pipe,�ؽ�·��
		*/
		else
		{
			NODE_HANDLE exe_node;
#ifdef DEBUG_INDICATOR
			my_printf("the node has been marked disconnect %bu and repair fail %bu, rebuild its path\r\n", 
							node_proc->set_disconnect_cnt, node_proc->repair_fail_cnt); 
#endif
my_printf("proc_phase:%bu,proc_cnt:%bu\r\n",node_proc->alternative_progress_phase,node_proc->alternative_progress_cnt);
			if(node_proc->alternative_progress_phase == get_node_phase(node_proc))
			{
				exe_node = back_trace(NULL, node_proc, node_proc->alternative_progress_cnt++);
			}
			else
			{
				exe_node = back_trace(NULL, network_tree[node_proc->alternative_progress_phase], node_proc->alternative_progress_cnt++);
			}

			if(!exe_node)
			{
				node_proc->alternative_progress_phase = (node_proc->alternative_progress_phase+1) % CFG_PHASE_CNT;
				node_proc->alternative_progress_cnt = 0;

				/* 2015-09-25  */
				if(node_proc->alternative_progress_phase == get_node_phase(node_proc))	//���������λת��һȦ��ת����,���עfail
				{
					node_proc->alternative_progress_fail = 1;	my_printf("refound fail\r\n");					
				}
			}
			else
			{
				if(!is_node_in_branch(exe_node, node_proc) && (get_node_depth(exe_node) < get_max_build_depth()))	/* 2015-04-17 Լ��ִ�нڵ����� */
				{
					status = psr_find_xid(exe_node, node_proc->uid, FALSE, RATE_BPSK, &pipe_id);
					if(!status)
					{
						status = psr_find_xid(exe_node, node_proc->uid, FALSE, RATE_DS15, &pipe_id);
					}

					if(status && pipe_id)
					{
#ifdef DEBUG_INDICATOR
						my_printf("new path build for uid[");
						uart_send_asc(node_proc->uid,6);
						my_printf("] by pipe id 0x%x.\r\n",pipe_id);
#endif
						sync_pipe_to_topology(pipe_id);						// ��ʵpsr_find_xid�Ѿ�����
						clr_node_state(node_proc, NODE_STATE_DISCONNECT);
						set_node_state(node_proc, NODE_STATE_RECONNECT);	// 2015-10-20 ����Ҫ�������޸�״̬
						node_proc->repair_fail_cnt = 0;
						node_proc->set_disconnect_cnt = 0;
						node_proc->grade_drift_cnt = 0; 					//2015-11-11 �����������ͻؿ���
					}
				}
			}
		}
	}
}

/*******************************************************************************
* Function Name  : powermesh_optimizing_update_proc()
* Description    : �����Ż����ٽڵ�,��������·���������
					����:
					* һ�δ���һ���ڵ�,
					* һ���ڵ����ȳ��Ա��bpskһ���ڵ�,���ds15һ���ڵ�,���ټ���Ѱ��
					* �༶�ڵ����ȳ��Ա��һ���ڵ�,������������ķ�ʽ�ҵ����ŵ��ϼ��ڵ�
					* һֱ��setup new pipe�ɹ�, �Ÿ�������
					except����:
					* �������setup pipe���ڵ�֮ǰ�κ�һ������ʧ��,�����ı��κζ���,����������disconnect
					* �κ�һ���ڵ㴦��ʧ��,������
					
					2015-11-11
					* ԭ�ȵĵ�����������,���������,���ܼ�ʱ����,����Ӧ�ò�ɹ��ʽ���
					* �ı����,upgrade��Ϊupdate,����������Ҳ���Խ���,ʼ�ձ�֤�������ͨ��
					* Ϊ����żȻ�Ե��µ�ϵͳƵ��������,����,���������ͻػ���,����N����ʾ���������������,����N����ʾ����ʧ��������disconnect
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void powermesh_optimizing_update_proc(void)
{
	static u8 optimizing_phase = 0;
	static u16 optimizing_step = 1;				//0�򷵻ظ�Ŀ¼, ���back_step��1��ʼ
	STATUS status;
	BOOL update;

	NODE_HANDLE node_proc;

	node_proc = back_trace(NULL, network_tree[optimizing_phase], optimizing_step++);
	if(node_proc == NULL)
	{
		optimizing_phase = (optimizing_phase + 1) % CFG_PHASE_CNT;
		optimizing_step = 1;
#ifdef DEBUG_INDICATOR
		my_printf("powermesh_optimizing_update_proc(): proc phase change to %bu\r\n",optimizing_phase);
#endif
		return;
	}
	else if(node_proc->state & NODE_STATE_EXCLUSIVE)
	{
#ifdef DEBUG_INDICATOR
		my_printf("powermesh_optimizing_update_proc():node[");
		uart_send_asc(node_proc->uid, 6);
		my_printf("] is exclusive\r\n");
#endif
		return;
	}

	status = reevaluate_node_direct(node_proc, &update);			//�����ǲ���һ���ڵ�,���Ȳ���һ���ܷ���һ���ڵ�
	if(!status && (get_node_depth(node_proc) > 1))
	{
		status = reevaluate_node_indirect(node_proc, &update);	//��һ���ڵ�, ��Ѱ���õ��ϼ��ڵ�
	}

	//if((get_node_depth(node_proc) > 1))
	//{
	//	status = reevaluate_node_indirect(node_proc);
	//}
	//else
	//{
	//	status = reevaluate_node_direct(node_proc);
	//}

	if(status)
	{
		// ����addr db
		update_pipe_db_in_branch(node_proc);					//����Ż��иı�, ������ı�Ľڵ����ӽڵ�, ���¸�branch�ϵ����нڵ��pipe��
		if(update)												// 2015-10-28 ֻ�����˻򷽰�ʵ�ʷ����䶯�Ĳű�עUPGRADE
		{
			set_node_state(node_proc, NODE_STATE_UPGRADE);		//2015-10-20 ���������ڵ��־
		}
		clr_node_state(node_proc, NODE_STATE_DISCONNECT);		//2015-10-27 ȥ���ڵ�ʧ������
	}



#ifdef DEBUG_INDICATOR
	my_printf("node[");
	uart_send_asc(node_proc->uid, 6);
	if(status)
	{
		if(update)
		{
			my_printf("] is updated\r\n");
		}
		else
		{
			my_printf("] remains\r\n");
		}
	}
	else
	{
		my_printf("] opmize fail\r\n");
	}
#endif		

}

/*******************************************************************************
* Function Name  : judge_link_upgraded()
* Description    : 2015-10-28 �ж���·�Ƿ�仯
					1. �жϸ��ڵ��Ƿ�ı�
					2. �ж��븸�ڵ�֮��������з����Ƿ�䶯
					2015-11-05 ����log���е�ʱ�������з��������б䶯,��û��save,���¸�λ��,�������ķ�����Ϣ��ڵ�ķ�����Ϣ����,��ɳ���ʱ��׼����
					�ĳ�: ���ڵ�仯 �������������б䶯, ����Ϊ�ڵ������, �ۻ�һ���̶Ⱥ����
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
BOOL judge_link_upgraded(NODE_HANDLE target_node, NODE_HANDLE temp_parent_node, LINK_HANDLE test_link)
{
	if((target_node->parent_node == temp_parent_node) 
		&& ((target_node->downlink_plan & 0x03) == (test_link->downlink_mode & 0x03))
		&& ((target_node->uplink_plan & 0x03) == (test_link->uplink_mode & 0x03)))
	{
		my_printf("link remains\r\n");
		return FALSE;
	}
	else
	{
		my_printf("link upgrades\r\n");
		return TRUE;
	}
}

/*******************************************************************************
* Function Name  : reevaluate_node_direct()
* Description    : �������������нڵ�ĵ�Ե�ͨ�����, ʹ��diag�ж����������з���
					�����ֱͨ, �������˺�pipe
					�粻����, ���ı�ԭ���˺�pipe
					p.s. �����ڵ㻻�����
					2015-10-28
					���ص�statusֻ��ϵ�ж��� diag, psr_setup, ���˸��¶���ɵı�־
					����������ԭ�����û�б仯, �Ƿ�䶯��Ҫ��pt_upgraded
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS reevaluate_node_direct(NODE_HANDLE target_node, BOOL * pt_upgraded)
{
	STATUS status = FAIL;
	u8 phase;
	LINK_STRUCT test_link;
	u8 err_code;
	BOOL link_upgraded = FALSE;
	s16 grade_drift_cnt;


#ifdef DEBUG_INDICATOR
	my_printf("reevaluate_node_direct(): proc node[");
	uart_send_asc(target_node->uid, 6);
	my_printf("]\r\n");
#endif


	phase = get_node_phase(target_node);
	if(phase == (u8)INVALID_RESOURCE_ID)
	{
#ifdef DEBUG_INDICATOR
		my_printf("target node is not existed in topology\r\n");
#endif
		return status;
	}

	// 1. diag target node directly, try to update it
	status = get_optimized_link_from_node_to_uid(network_tree[phase], target_node->uid, RATE_BPSK, &test_link, &err_code);
	*pt_upgraded = FALSE;
	grade_drift_cnt = u2s3b(target_node->grade_drift_cnt);
	
	// 2. if diag directly successfully, setup up a new pipe
	if(status)
	{
		link_upgraded = judge_link_upgraded(target_node, network_tree[phase], &test_link);	//2015-10-28 �ж���·�Ƿ�䶯

		if(link_upgraded)
		{
			
			if(grade_drift_cnt < POWERMESH_OPTIMIZE_UPGRADE_THRESHOLD)
			{
#ifdef DEBUG_INDICATOR
				my_printf("target grade drift cnt:%d\r\n",grade_drift_cnt);
#endif
				grade_drift_cnt++;
				target_node->grade_drift_cnt = s2u3b(grade_drift_cnt);
			}
			else
			{
				status = add_pipe_under_node(network_tree[phase],&test_link);

				// 3. if pipe setup successfully, update topology
				if(status)
				{
					target_node = add_link_to_topology(network_tree[phase],&test_link);
					// 4. 2015-10-28 judge whether topology upgraded
					target_node->grade_drift_cnt = 0;
					*pt_upgraded = TRUE;
				}
			}
		}//if diag�ɹ�,����·û����,�򲻸ı�grade_driftֵ
	}
	else
	{
		if(get_node_depth(target_node) == 1)	//if diagʧ��,һ���ڵ������ﴦ��, �༶�ڵ㽵������ŵ�indirect�д���
		{
			if(grade_drift_cnt > POWERMESH_OPTIMIZE_DEGRADE_THRESHOLD)
			{
#ifdef DEBUG_INDICATOR
				my_printf("target grade drift cnt:%d\r\n",grade_drift_cnt);
#endif				
				grade_drift_cnt--;
				target_node->grade_drift_cnt = s2u3b(grade_drift_cnt);
			}
			else
			{
#ifdef DEBUG_INDICATOR
				my_printf("set disconnect\r\n");
#endif	
				set_uid_disconnect(target_node->uid);
			}
		}
	}

	return status;
}

/*******************************************************************************
* Function Name  : reevaluate_node_indirect()
* Description    : 2015-10-28
					���ص�statusֻ��ϵ�ж��� diag, psr_setup, ���˸��¶���ɵı�־
					����������ԭ�����û�б仯, �Ƿ�䶯��Ҫ��pt_upgraded
* Input          : pt_upgraded: �Ƿ���·�����˱䶯
* Output         : 
* Return         : 
*******************************************************************************/
extern u8 psr_explore_new_node_uid_buffer[CFG_EBC_ACQUIRE_MAX_NODES*6];

STATUS reevaluate_node_indirect(NODE_HANDLE target_node, BOOL * pt_upgraded)
{
	STATUS status = FAIL;
	u8 phase;
	LINK_STRUCT test_link;
	u8 err_code;
	u16 pipe_id;
	EBC_BROADCAST_STRUCT es;
	u8 neighbors_num = 0;
	float node_ptrp,node_ppcr;
	BOOL link_upgraded = FALSE;
	s16 grade_drift_cnt;

	// 0. basic check: node valid & pipe valid
#ifdef DEBUG_INDICATOR
	my_printf("reevaluate_node_direct(): proc node[");
	uart_send_asc(target_node->uid, 6);
	my_printf("]\r\n");
#endif

	phase = get_node_phase(target_node);
	if(phase == (u8)INVALID_RESOURCE_ID)
	{
#ifdef DEBUG_INDICATOR
		my_printf("target node is not existed in topology\r\n");
#endif
		return FAIL;
	}

	pipe_id = node2pipeid(target_node);
	if(!pipe_id)
	{
#ifdef DEBUG_INDICATOR
		my_printf("no valid pipe for target node\r\n");
#endif
		return FAIL;
	}

	node_ptrp = calc_node_ptrp(target_node,&node_ppcr);
	if(node_ptrp == INVALID_PTRP)
	{
#ifdef DEBUG_INDICATOR
		my_printf("error:current node ptrp is 0\r\n");
#endif
		return FAIL;
	}
	else
	{
#ifdef DEBUG_INDICATOR
		my_printf("current node ptrp :%lu\r\n",(u32)(node_ptrp));
#endif
	}

	// 1. psr explore
	set_broadcast_struct(phase, &es, 0, target_node->downlink_plan & 0x03);		//set es as basic configuration
	es.xmode = target_node->downlink_plan;	//use original plan to broadcast
	es.rmode = target_node->uplink_plan;
	es.scan = 0;					// avoid using scan to speed up
	es.bid = 0;						// all nodes will response
	es.mask = BIT_NBF_MASK_ACPHASE;	// no build id


	status = psr_explore(pipe_id, &es, &neighbors_num);	
	grade_drift_cnt = u2s3b(target_node->grade_drift_cnt);

	// 2. find better path
	if(!status)
	{
		/* 2015-11-11
		// ���ӽڵ㽵���Ŀ���,��������,�����ͻػ���,����N�β��Կ�������������,N������̽�ⷢ����·ʧ��������diconnect
		*/
		if(grade_drift_cnt > POWERMESH_OPTIMIZE_DEGRADE_THRESHOLD)
		{
#ifdef DEBUG_INDICATOR
			my_printf("target grade drift cnt:%d\r\n",grade_drift_cnt);
#endif
			grade_drift_cnt--;
			target_node->grade_drift_cnt = s2u3b(grade_drift_cnt);
		}
		else
		{
#ifdef DEBUG_INDICATOR
			my_printf("set disconnect\r\n");
#endif
			set_uid_disconnect(target_node->uid);		//����N����·ʧ��
		}
	}
	else if(neighbors_num)
	{
		u8 i;
		NODE_HANDLE neighbor_node;
		float neighbor_ptrp, neighbor_ppcr;
		float new_ptrp;
		u8 neighbor_uid[6];

		for(i=0; i<neighbors_num; i++)
		{
			SET_BREAK_THROUGH("OPTIMIZING ABORT.\r\n");

			// get target uid
			if(((i+1)*6+2) <= sizeof(psr_explore_new_node_uid_buffer))
			{
				mem_cpy(neighbor_uid,&psr_explore_new_node_uid_buffer[i*6+2],6);
				neighbor_node = uid2node(neighbor_uid);
				if(neighbor_node)				//����ǰ���ڵ�Ҳ�����Ż�����
				{
					if(neighbor_node->state & (NODE_STATE_DISCONNECT | NODE_STATE_EXCLUSIVE | NODE_STATE_ZX_BROKEN
												/*| NODE_STATE_PIPE_FAIL*/ | NODE_STATE_AMP_BROKEN))
					{
#ifdef DEBUG_INDICATOR
						my_printf("neighbor node[");
						uart_send_asc(neighbor_node->uid,6);
						my_printf("] excluded because of state:%bX\r\n",neighbor_node->state);
#endif
						continue;			//�ų����ϸ�Ľڵ�
					}
					else
					{
						neighbor_ptrp = calc_node_ptrp(neighbor_node,&neighbor_ppcr);
						if(neighbor_ptrp < node_ptrp)
						{
#ifdef DEBUG_INDICATOR
							my_printf("neighbor node[");
							uart_send_asc(neighbor_node->uid,6);
							my_printf("] excluded because of too low ptrp %lu\r\n",(u32)(neighbor_ptrp));
#endif
							continue;
						}
						else
						{
#ifdef DEBUG_INDICATOR
							my_printf("test neighbor node[");
							uart_send_asc(neighbor_node->uid,6);
							my_printf("]\r\n");
#endif						
							status = get_optimized_link_from_node_to_uid(neighbor_node, target_node->uid, RATE_BPSK, &test_link, &err_code);
							if(status)
							{
								new_ptrp = calc_path_add_link_ptrp(neighbor_ptrp, neighbor_ppcr, &test_link, NULL);
#ifdef DEBUG_INDICATOR
								my_printf("current ptrp %lu, new ptrp %lu\r\n", (u32)node_ptrp, (u32)new_ptrp);
#endif
								if(new_ptrp > node_ptrp)
								{
#ifdef DEBUG_INDICATOR
									my_printf("choose it\r\n");
#endif
									link_upgraded = judge_link_upgraded(target_node, neighbor_node, &test_link);	//2015-10-28 �ж���·�Ƿ�䶯

									if(link_upgraded)
									{
										if(grade_drift_cnt < POWERMESH_OPTIMIZE_UPGRADE_THRESHOLD)
										{
#ifdef DEBUG_INDICATOR
											my_printf("target grade drift cnt:%d\r\n",grade_drift_cnt);
#endif
											grade_drift_cnt++;
											target_node->grade_drift_cnt = s2u3b(grade_drift_cnt);
										}
										else
										{
											status = add_pipe_under_node(neighbor_node,&test_link);
											if(status)
											{
												add_link_to_topology(neighbor_node,&test_link);
#ifdef DEBUG_INDICATOR
												my_printf("update successfully\r\n");			//�ҵ�һ���ȵ�ǰ·�����ŵļ��˳�ѭ��, ��ʹ·��û�б�, ֻ������������ȱ���
#endif
												*pt_upgraded = TRUE;
												target_node->grade_drift_cnt = 0;
												return OKAY;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	*pt_upgraded = FALSE;
	return FAIL;
}

/******************* (C) COPYRIGHT 2012 Belling *****END OF FILE****/
