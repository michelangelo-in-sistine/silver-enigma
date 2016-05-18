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

#define POWERMESH_OPTIMIZE_UPGRADE_THRESHOLD	1		//grade_drift_cnt的范围是[-4,3],达到此threshold即升级
#define POWERMESH_OPTIMIZE_DEGRADE_THRESHOLD	-1		//grade_drift_cnt的范围是[-4,3],达到此threshold即设置disconnect

/* Private typedef -----------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Extern variables ---------------------------------------------------------*/
extern NODE_HANDLE network_tree[];
extern LINK_STRUCT new_nodes_info_buffer[];
extern DST_CONFIG_STRUCT dst_config_obj;
extern u8 psr_explore_new_node_uid_buffer[];


/* Private variables ---------------------------------------------------------*/
NODE_HANDLE disconnect_list[CFG_MGNT_MAX_NEW_LINKS_BUFFER_CNT];
BOOL network_optimizing_switch = FALSE;							//网络优化进程开关

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
* Description    : 	由于有时候的确需要指定pipe而不是uid(在repair_pipe, 利用中间节点的时候), 因此原uid_diag_uid的基础上
					升级改为unified_diag, 同时传入的参数有source_uid, pipe_id
					+ 当source_uid不为NULL, 则为原来的uid_diag_uid, 
						++ 如源节点是本地节点, 调用dll_diag; 采用的相位以源节点相位为主, 而不以传入的DLL_SEND结构体里的相位为准
						++ 如是远端节点, 调用mgnt_diag;(以source_uid反向查找获得pipe)
					+ 当source_uid为NULL, 则按pipe_id做diag
					+ diag成功, 首字节为写入字节数, 返回OKAY;
					+ 如diag失败:
						++ 如源节点是本地节点, 首字节为DIAG_TIMEOUT, 返回FAIL;
						++ 如源节点是远端节点:
							+++ 如通信失败导致失败, 首字节为TIME_OUT, 返回FAIL;
							+++ 如diag失败, 首字节为DIAG_TIMEOUT, 返回FAIL;
					
* Input          : None
* Output         : None
* Return         : OKAY/FAIL
*******************************************************************************/
STATUS unified_diag(UID_HANDLE source_uid, u16 pipe_id, DLL_SEND_HANDLE pds, u8 * diag_buffer)
{
	u8 i,j;
	STATUS status = FAIL;

	/* 源节点为本地节点处理 */
	for(i=0;i<CFG_PHASE_CNT;i++)
	{
		if(check_uid(i,source_uid)==CORRECT)
		{
			pds->phase = i;		//采用的相位以源节点相位为主, 而不以传入的DLL_SEND结构体里的相位为准
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

	/* 执行到此处, 源节点不为本地节点或为NULL, 
		如源节点不为NULL, 则查找其对应的pipe_id, 不管传入的pipe_id是否有效
		如源节点为NULL, 则以传入的pipe_id为准 */
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
* Description    : 指定节点diag uid
					如diag失败, diag_buffer首字节为0xF5; 如成功, 首字节为写入字节数
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
* Description    : 指定节点间的diag
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
					应用过程中有时的确需要指定pipe_id而不是source_uid(例如repair过程中涉及到中间节点)
					因此修改为可以指定source_uid, 也可以指定pipe_id, 返回最优link方案
					+ 以source_uid为优先判断, 当不为NULL时以其为准
					+ 当source_uid为NULL时, 以pipe_id为准
					+ initial_rate参数, 即初始测试的速率, 以减少不必要的高速率时间浪费
					+ 当返回失败时, 用err_code区分失败原因(diag_time_out/psr_time_out)
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
	u8 retreat = 0;						// 2013-08-08 由于有交流信号叠加的影响, 有些节点的测量信噪比正常的信噪比明显的低, 改成速率最多下降一次, 以节省时间
	
	for(rate=initial_rate;rate<=CFG_MAX_LINK_CLASS;rate++) 
	{
		SET_BREAK_THROUGH("quit get_optimized_link()\r\n");
		mem_clr(&dds,sizeof(DLL_SEND_STRUCT),1);	//dds的相位不需要设置,如果源节点是本地节点,会按照本地节点所属相位设置,如果是异地节点,会按照pipe相位设置
		dds.xmode = 0x10 + rate;
		dds.rmode = 0x10 + rate;
		dds.prop = BIT_DLL_SEND_PROP_DIAG|BIT_DLL_SEND_PROP_SCAN|BIT_DLL_SEND_PROP_REQ_ACK;
		dds.target_uid_handle = target_uid;
		result = unified_diag(source_uid, pipe_id, &dds, diag_buffer);

		if(result)
		{
			pt = diag_buffer + 1;									// 首字节为数据长度
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
* Description    : 给定node, 返回其与指定uid之间的最优link方案, 用于尚未将目的uid加入网络时
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
* Description    : 当repair pipe时, 需要指定pipe, 而不能指定中间节点, 因为中间节点有自己的pipe
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
* Description    : 根据scan diag结果选择上下行链路方案
					diag_buffer格式:
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
* Description    : 根据ptrp和速率对链路质量做判决,OKAY则pass,否则再选
* Input          : None
* Output         : None
* Return         : PTRP of the link
*******************************************************************************/
STATUS judge_optimized_link(LINK_HANDLE optimized_link)
{
	u8 xmode;

	xmode = optimized_link->downlink_mode & 0x03;

//	if(xmode==0 && optimized_link->ptrp<(PTRP_DS15/2))
	if(xmode==0 && optimized_link->ptrp < CFG_BPSK_PTRP_GATE)				//2014-09-29 林洋建议提高BPSK门槛
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
* Description    : 根据原有信息重建pipe
				2013-08-30 改成先查看拓扑, 如有对应节点, 则用拓扑路径重建pipe
							如没有对应节点, 用pipe库里的pipe重建路径;
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

	/* 如果对应的节点在拓扑中存在, 则以拓扑路径为准, 否则以pipe_mnpl库描述为准 */

	/* 2014-04-29 武汉的现场拓扑重建不易, 适合手工输入pipe, 以手工输入pipe为准, 因此不使用拓扑产生pipe*/
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
* Description    : 修复pipe, 用于解决第I类问题
			2014-08-28 取消refresh_pipe解决修复pipe, 因为通信之前都refresh一次
			2014-09-05 repair只一级一级的修复, 如修复过程中遇到上一级失效则直接返回失败, 简化流程
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

	/* patrol_buffer的第[N*9 + 7]字节都是SNR */
	u8 downlink_ss[CFG_PIPE_STAGE_CNT];
	u8 downlink_snr[CFG_PIPE_STAGE_CNT];
	u8 downlink_mode[CFG_PIPE_STAGE_CNT];
	u8 uplink_ss[CFG_PIPE_STAGE_CNT];
	u8 uplink_snr[CFG_PIPE_STAGE_CNT];
	u8 uplink_mode[CFG_PIPE_STAGE_CNT];

	/*********************************************************************
	如对应节点存在与拓扑结构中, 用拓扑得到script, 否则从pipe库中得到script
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
	1. 利用script_buffer建立pipe_id的pipe, 用于解决下一级存储的pipe丢失的情况
	2. 利用patrol功能检查一下各级SNR的裕度, 如有裕度低于阈值的环节, 加强之
	3. 能利用以上功能解决的都是轻微问题的pipe, 应该能解决大部分问题, 如果不能解决, 则进入下一个流程
	************************************************************************/
	if(script_length)
	{
		BOOL patrol_buffer_valid = FALSE;
	
		status = psr_setup(phase, pipe_id, script_buffer, script_length);
		if(status)
		{
			/* 使用patrol获得整条PATH的信噪比 */
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

		//如果能够得到patrol结果,则可以快速定位接触不良的路径, 加强薄弱环节 */
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

			/*如果有patrol数据可供参考, 则只优化薄弱的环节, 如没有patrol数据供参考, 则一级一级打通*/
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
					// 改变pipe script
					script_buffer[i*8+6] = test_link.downlink_mode;
					script_buffer[i*8+7] = test_link.uplink_mode;

					//如果使用拓扑, 则更新拓扑中的链路信息
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
			status = psr_setup(phase, pipe_id, script_buffer, script_length);			//重刷一遍新的pipe
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
* Description    : 优化PATH的每一级链路, 用于解决第II类问题
					这里的传入参数是path的路径描述,而不是pipe,因为需要优化用find方法得到的新节点路径,此时并没有pipe
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

	/* 逐级优化链路 */
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
* Description    : 优化PIPE的每一级链路, 用于解决第II类问题
					路径描述来自pipe_mnpl_db, 而不是拓扑库
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
* Description    : 用dst flooding search方式搜索节点, 支持表号搜索,
*					节点可以是拓扑中存在的节点也可以是新节点
*					策略是新节点以bpsk开始搜索,换相搜,然后ds15直连搜,换相搜,
*					都失败后,bpsk一级搜索
*					最大搜索完成时间6分钟左右,感觉还是太慢了,慎用
* Author		 : Lv Haifeng
* Date			 : 2014-10-13
* Input          : 
* Output         : 
* Return         : 成功则返回对应 pipe_id, 失败则返回0
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

	/*** 确定初始搜索条件:********************
	* meter_id: 从基本开始搜索
	* 不在拓扑中的uid: 从基本开始搜索
	* 在拓扑中的uid: 从拓扑中原有条件开始搜索
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
				jumps -= 1;						//根节点深度为0, 一级节点深度为1, 跳数为0
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

	// 搜索重试的策略: 增加跳数>降低速率>换相
	for(j=0;j<2;j++)
	{
		//降速
		for(r=0;r<2;r++)
		{
			// 每相搜索两次, 如果失败, 换相搜索
			for(p=0; p<CFG_PHASE_CNT; p++)
			{
				u8 use_phase = (phase+p) % CFG_PHASE_CNT;
				u8 use_rate = rate + r;
				u8 use_jumps = jumps + j;

				//目前禁止DS15以上的通信方式以洪泛搜索
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
							phase = (phase+1)%CFG_PHASE_CNT;		//如果搜索到路径但优化失败,如果是acps没有约束,则可能是相位不对换相搜索
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

	/* 为指定路径增加pipe */
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
* Description    : 当确定原有路径已失效时, 重新搜索新路径, 解决第III类问题
*					策略是首先在原相位以BPSK模式重新搜索
*					窗口以当前相位上节点数定, 速率BPSK & 窗口数32 & 不约束相位 & 深度与PIPE一致
*					第一次搜索失败则深度+1重新搜索;
*					第二次搜索失败则深度+1重新搜索;
*					第三次搜索失败则换相位重新搜索;
*					第四次搜索失败则再换相位重新搜索;
*					一旦搜索成功, 首先确定相位, 如果级别为1级则直接点对点判定. 如果级别大于1用该PIPE广播获得节点, 根据节点的相位判断该节点相位
*					如果该节点相位不对, 则换
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



	/* 最大重建级别可以与原pipe级别相同, 也可以加1 */
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

	/* 使用的搜索速率由路径上的最慢路径速率决定 */
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
* Description    : pipe测试连通性, 以连续ping 2次成功为准
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
* Description    : 以dst_search方法查找uid
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


	mem_cpy((u8*)&dst_backup,(u8*)&dst_config_obj,sizeof(DST_CONFIG_STRUCT));	//备份dst全局设置,以避免影响APP层的DST
	config_dst_flooding(rate, max_jumps, max_window_index,acps_constrained,0);					//限制同相位, 不限制网络

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
	/* 洪泛占据时间太长, 最好不要使用nav, 否则效率太低 */
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
	}while(!is_timer_over(tid) /*&& !status*/);		//等待直到洪泛结束

	mem_cpy((u8*)&dst_backup,(u8*)&dst_config_obj,sizeof(DST_CONFIG_STRUCT));	//还原dst全局设置

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
* Description    : 在网络中寻找uid节点/mid节点, 返回到达该uid的pipe
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

	/* 在指定相位寻找目标节点 */
#ifdef DEBUG_INDICATOR
	my_printf("dst_find():find uid/mid with dst rate:%bx, jump:%bx, window:%bx\r\n",rate,jumps,window);
#endif
	status = flooding_search(phase, target_xid, find_mid, rate, jumps, window, 1, script_buffer, &script_length);

	/* 如成功, 优化路径 */
	if(status)
	{
		status = optimize_path_link(phase,script_buffer,script_length);
	}

	/* 为指定路径增加pipe */
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
* Description    : dst flooding search返回的格式
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

	// 以携带的relay conf来判断目标节点的所在位置
	for(pt_search = nsdu + 6; (pt_search-nsdu)<nsdu_len; pt_search+=7)
	{
		if(*pt_search & BIT_DST_CONF_UPLINK)
		{
			pt_search-=6;	//指向target uid
			break;
		}
	}

	// 若找不到uplink的起始点, 认为最后一个节点uid就是target, 以应对上下行就一级的情况
	if((pt_search-nsdu)>=nsdu_len)
	{
		pt_search = &nsdu[nsdu_len-6];
	}	

	// pipe只支持双向, 而search 回来的路径上行下行可能不对称, 选择较短的一条
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
* Description    : 网络优化功能函数, 用于空闲时间整理网络, 需要应用层在空闲时循环调用
					每次只执行一次步骤.

					2015-09-30
					加入upgrade优化, 每次调用或者处理disconncet, 或者调用upgrade
					具体调用什么采用欲望算法
					每次根据不同任务的欲望值大小确定调用谁,响应的欲望归0,没有响应的欲望增加N
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

	// 2015-09-25 过滤掉多次修复失败的节点
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

	// 2015-09-30 使用欲望算法,确定执行的内容
	desire_disconnect_proc += disconnect_usage;			//disconnect处理的欲望值每次增加的是当前失联节点总数
	desire_upgrade_proc += 1;							//upgrade处理每次欲望值加1

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

	// 执行相应处理
	switch(proc_decision)
	{
		case(OPTIMIZING_PROC_DISCONNECT):
		{
			powermesh_optimizing_disconnect_proc(disconnect_usage);		//如没有失联节点需要处理, 进入节点优化处理流程
			break;
		}
		case OPTIMIZING_PROC_UPGRADE:
		{
			powermesh_optimizing_update_proc();		//如没有失联节点需要处理, 进入节点优化处理流程
			break;
		}
	}
}

/*******************************************************************************
* Function Name  : enable_network_optimizing
* Description    : 集中器开始抄表时(路由暂停), 禁止网络优化, 集中器停止抄表时(路由重启), 使能网络优化
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
		set_quit_loops();			//停止一切耗时操作, 回到主线程
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
* Return         : 通信成功返回SUCCESS, pipe_id用指针返回
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
	es.window = 0;				//搜索特定id, 只留一个窗口即可
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
	* 使用EBC广播召唤节点, 窗口留1个即可
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
				if(status == FAIL)						//psr通信失败, 重试一次
				{
#ifdef DEBUG_INDICATOR
					my_printf("psr comm fail.\r\n");
#endif
					set_node_state(exe_node,NODE_STATE_DISCONNECT);
					return FAIL;
				}
			}

			if((status == OKAY) && resp_num == 0)		//通信成功, 确定没找到
			{
#ifdef DEBUG_INDICATOR
				my_printf("exe node find no resp.\r\n");
#endif
				return OKAY;
			}
		}
		else											// 有些节点在组网时可能仅是被发现,但没有建立可用pipe
		{
#ifdef DEBUG_INDICATOR
			my_printf("no available comm pipe.\r\n");
#endif
			return FAIL;
		}
	}

	/**************************************************
	* step2.
	* 训练信道
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
		status = optimization_process_explored_uid(exe_node, target_uid);	//2015-10-29 创建optimization_process_explored_uid()代替原来的process_explored_uid(),不ping,不setup_buildid
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
* Description    : 处理失联节点
					执行策略:
					1. 不能死磕一个修复不通的节点, 要给别的节点机会;
					2. 不能在有修复不成功的节点的时候就不给别的节点优化的机会; 可以考虑进入后随机选择任务
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
		randomize_list(disconnect_list,disconnect_usage);		//每次随机执行, 避免在一个节点上死磕. 缺点: 有可能改变了拓扑, 导致回溯次序有问题
		node_proc = disconnect_list[0];

#ifdef DEBUG_INDICATOR
		my_printf("powermesh_optimizing_proc(): deal disconnect node:");
		uart_send_asc(node_proc->uid, 6);
		my_printf("\r\n");
#endif

		/********************************************************
		如果节点总是被修理, 总是被设置成disconnect, 也许该重建路径, 而不是继续修理
		2015-04-21 林洋提出, 节点深度一旦限定了, 尽量做到所有节点都能不至于太深, 而他们测试的时候, 喜欢把节点挪来挪去, 就有可能本来限制最深
					只有2级, 而他们将一个带子节点的节点挪到了2级, 按照算法, 其子节点就成了3级节点, 就不符合要求了.
					这里的处理是, 首先这种被挪过去的子节点已经都被设置成了disconnect,那么在优化时,凡是深度大于要求的节点,就不再repair了,而是首先选择重建路径
					同时,这种重建路径又不改变其原来的节点关系,因此原3级路径还能用,同时争取找到更短的路径.
		********************************************************/
		if((node_proc->set_disconnect_cnt < CFG_MAX_DISCONNECT_REPAIR) &&
			(node_proc->repair_fail_cnt < CFG_MAX_OPTIMAL_PROC_REPAIR) &&
			(get_node_depth(node_proc) <= get_max_build_depth()))				//2015-04-21 增加限制, 解决"拖油瓶"节点超过组网深度限制问题
		{
			node_proc->alternative_progress_phase = get_node_phase(node_proc);
			status = repair_pipe(node2pipeid(node_proc));
			if(status)
			{
				clr_node_state(node_proc, NODE_STATE_DISCONNECT);
				set_node_state(node_proc, NODE_STATE_RECONNECT);	//2015-10-20 林洋要求增加修复状态
				node_proc->grade_drift_cnt = 0;						//2015-11-11 增加升降级滞回控制
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
			对于多次报修,或者一直修理不好的的pipe,重建路径
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
				if(node_proc->alternative_progress_phase == get_node_phase(node_proc))	//如果搜索相位转了一圈又转回来,则标注fail
				{
					node_proc->alternative_progress_fail = 1;	my_printf("refound fail\r\n");					
				}
			}
			else
			{
				if(!is_node_in_branch(exe_node, node_proc) && (get_node_depth(exe_node) < get_max_build_depth()))	/* 2015-04-17 约束执行节点的深度 */
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
						sync_pipe_to_topology(pipe_id);						// 其实psr_find_xid已经做了
						clr_node_state(node_proc, NODE_STATE_DISCONNECT);
						set_node_state(node_proc, NODE_STATE_RECONNECT);	// 2015-10-20 林洋要求增加修复状态
						node_proc->repair_fail_cnt = 0;
						node_proc->set_disconnect_cnt = 0;
						node_proc->grade_drift_cnt = 0; 					//2015-11-11 增加升降级滞回控制
					}
				}
			}
		}
	}
}

/*******************************************************************************
* Function Name  : powermesh_optimizing_update_proc()
* Description    : 尝试优化提速节点,避免所有路径单向变慢
					策略:
					* 一次处理一个节点,
					* 一级节点首先尝试变成bpsk一级节点,其次ds15一级节点,不再继续寻找
					* 多级节点首先尝试变成一级节点,其次再用搜索的方式找到更优的上级节点
					* 一直到setup new pipe成功, 才更新拓扑
					except处理:
					* 如果包括setup pipe在内的之前任何一步处理失败,都不改变任何东西,包括不设置disconnect
					* 任何一个节点处理失败,都跳过
					
					2015-11-11
					* 原先的单向升级策略,当网络变差后,不能及时调整,导致应用层成功率降低
					* 改变策略,upgrade变为update,即可以升级也可以降级,始终保证网络的连通性
					* 为避免偶然性导致的系统频繁升降级,升级,降级采用滞回机制,连续N次显示网络可升级再升级,连续N次显示网络失败再设置disconnect
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void powermesh_optimizing_update_proc(void)
{
	static u8 optimizing_phase = 0;
	static u16 optimizing_step = 1;				//0则返回根目录, 因此back_step从1开始
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

	status = reevaluate_node_direct(node_proc, &update);			//无论是不是一级节点,都先测试一下能否变成一级节点
	if(!status && (get_node_depth(node_proc) > 1))
	{
		status = reevaluate_node_indirect(node_proc, &update);	//非一级节点, 找寻更好的上级节点
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
		// 更新addr db
		update_pipe_db_in_branch(node_proc);					//如果优化有改变, 如果被改变的节点有子节点, 更新该branch上的所有节点的pipe库
		if(update)												// 2015-10-28 只有拓扑或方案实际发生变动的才标注UPGRADE
		{
			set_node_state(node_proc, NODE_STATE_UPGRADE);		//2015-10-20 增加升级节点标志
		}
		clr_node_state(node_proc, NODE_STATE_DISCONNECT);		//2015-10-27 去掉节点失联属性
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
* Description    : 2015-10-28 判断链路是否变化
					1. 判断父节点是否改变
					2. 判断与父节点之间的上下行方案是否变动
					2015-11-05 发现log中有的时候上下行方案速率有变动,但没有save,导致复位后,集中器的方案信息与节点的方案信息不符,造成抄表定时不准现象
					改成: 父节点变化 或上下行速率有变动, 则认为节点更新了, 累积一定程度后存盘
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
* Description    : 测试网络中已有节点的点对点通信情况, 使用diag判断最优上下行方案
					如可以直通, 更新拓扑和pipe
					如不可以, 不改变原拓扑和pipe
					p.s. 不检查节点换相情况
					2015-10-28
					返回的status只是系列动作 diag, psr_setup, 拓扑更新都完成的标志
					可能拓扑与原来相比没有变化, 是否变动了要看pt_upgraded
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
		link_upgraded = judge_link_upgraded(target_node, network_tree[phase], &test_link);	//2015-10-28 判断链路是否变动

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
		}//if diag成功,但链路没升级,则不改变grade_drift值
	}
	else
	{
		if(get_node_depth(target_node) == 1)	//if diag失败,一级节点在这里处理, 多级节点降级处理放到indirect中处理
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
					返回的status只是系列动作 diag, psr_setup, 拓扑更新都完成的标志
					可能拓扑与原来相比没有变化, 是否变动了要看pt_upgraded
* Input          : pt_upgraded: 是否链路发生了变动
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
		// 增加节点降级的可能,增加升级,降级滞回机制,连续N次测试可以升级再升级,N次升级探测发现链路失败则设置diconnect
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
			set_uid_disconnect(target_node->uid);		//超过N次链路失败
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
				if(neighbor_node)				//允许当前父节点也参与优化过程
				{
					if(neighbor_node->state & (NODE_STATE_DISCONNECT | NODE_STATE_EXCLUSIVE | NODE_STATE_ZX_BROKEN
												/*| NODE_STATE_PIPE_FAIL*/ | NODE_STATE_AMP_BROKEN))
					{
#ifdef DEBUG_INDICATOR
						my_printf("neighbor node[");
						uart_send_asc(neighbor_node->uid,6);
						my_printf("] excluded because of state:%bX\r\n",neighbor_node->state);
#endif
						continue;			//排除不合格的节点
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
									link_upgraded = judge_link_upgraded(target_node, neighbor_node, &test_link);	//2015-10-28 判断链路是否变动

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
												my_printf("update successfully\r\n");			//找到一个比当前路径更优的即退出循环, 即使路径没有变, 只是上下行信噪比变了
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
