#include "compile_define.h"
#include "powermesh_include.h"


#define MEASURE_UNPROTECT()	write_measure_reg(0x3E, 0x00000055)
#define MEASURE_PROTECT()	write_measure_reg(0x3E, 0x00000000)
#define MEASURE_DISABLE_HPF()		write_measure_reg(0x14, 0x0000001C)
#define CHECK_HPF_REG()			(read_measure_reg(0x14)==0x0000001C)

#define MEASURE_RESET()		write_measure_reg(0x3F, 0x005A5A5A)
#define MEASURE_PGA1()		write_measure_reg(0x15, 0x00000000)
#define MEASURE_PGA2()		write_measure_reg(0x15, 0x00000101)
#define MEASURE_PGA4()		write_measure_reg(0x15, 0x00000202)
#define MEASURE_PGA8()		write_measure_reg(0x15, 0x00000303)
#define MEASURE_PGA16()		write_measure_reg(0x15, 0x00000404)		//温度通道不放大
#define MEASURE_PGA24()		write_measure_reg(0x15, 0x00000505)
#define MEASURE_PGA32()		write_measure_reg(0x15, 0x00000606)

#define MEASURE_REG_I		0x01		//电流测量通道IA
#define MEASURE_REG_T		0x02		//温度测量通道IB
#define MEASURE_REG_V		0x03		//电压测量通道V


#define CALIB_MEAN_TIME		4
#define MEASURE_MEAN_TIME	4



CALIB_VI_STRUCT xdata calib_v, calib_i;
CALIB_T_STRUCT xdata * p_calib_t;


/*******************************************************************************
* Function Name  : measure_com_send
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void measure_com_send(u8 * head, u8 len)
{
	u8 i;
	for(i=0;i<len;i++)
	{
		measure_com_send8(*head++);
	}
}

/*******************************************************************************
* Function Name  : write_bl6523
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void write_measure_reg(u8 addr, u32 dword_value)
{
	u8 cs=0;
	u8 buffer[6];
	u8 i;
	
	buffer[0] = 0xCA;
	buffer[1] = addr;
	cs = addr;

	for(i=2;i<5;i++)
	{
		buffer[i] = (u8)dword_value;
		cs += buffer[i];
		dword_value>>=8;
	}
	buffer[5] = ~cs;

	measure_com_send(buffer,sizeof(buffer));
}

/*******************************************************************************
* Function Name  : read_bl6523
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
u32 read_measure_reg(u8 addr)
{
	u8 rec_byte;
	u8 i;
	u8 buffer[4];
	u32 value = 0;
	
	measure_com_send8(0x35);
	measure_com_send8(addr);

	for(i=0;i<4;i++)
	{
		if(measure_com_read8(&rec_byte))
		{
			buffer[i] = rec_byte;
		}
		else
		{
			my_printf("bl6532 return fail\n");
			return 0;
		}
		
	}

	for(i=0;i<4;i++)
	{
		addr += buffer[i];
	}

	if(addr!=255)
	{
		my_printf("bl6532 return bytes checked fail\n");
		uart_send_asc(buffer,4);
		return 0;
	}
	else
	{
		for(i=2;i!=0xFF;i--)
		{
			value <<= 8;
			value += buffer[i];
		}
		return value;
	}
}

/*******************************************************************************
* Function Name  : 
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void init_measure(void)
{
	u16 i;

	reset_measure_device();

	for(i=0;i<1000;i++)
	{
		MEASURE_UNPROTECT();
		MEASURE_DISABLE_HPF();
		MEASURE_PGA16();
		MEASURE_PROTECT();

		if(CHECK_HPF_REG())
		{
			break;
		}
		else
		{
			my_printf("measure reg fail\r\n");
		}
	}

	if(is_app_nvr_data_valid())
	{
		my_printf("calib data valid\r\n");
		calib_v.k = get_app_nvr_data_u_k();
		calib_v.b = get_app_nvr_data_u_b();
		calib_i.k = get_app_nvr_data_i_k();
		calib_i.b = get_app_nvr_data_i_b();
		p_calib_t = (CALIB_T_STRUCT xdata *)get_app_nvr_data_t_data();
	}
	else
	{
		mem_clr(&calib_v,sizeof(calib_v),1);
		mem_clr(&calib_i,sizeof(calib_i),1);
		p_calib_t = (CALIB_T_STRUCT xdata *)get_app_nvr_data_t_data();
		mem_clr(p_calib_t,sizeof(CALIB_T_STRUCT),1);
		my_printf("no valid calib data\r\n");
	}
}

s32 convert_uint24_to_int24(u32 value)
{
	if(value & 0xFF800000)
	{
		return (s32)((value&0x00FFFFFF) - 0x01000000);
	}
	else
	{
		return (s32)value;
	}
}

/*******************************************************************************
* Function Name  : 
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void set_calib_point(u8 index, CALIB_VI_STRUCT xdata * calib, u8 measure_reg_addr, s16 real_value)
{
	s32 reg_value = 0;
	u8 i;

	if(index>=MEASURE_VI_POINTS_CNT)
	{
		my_printf("calib point index error\n");
		return;
	}

	for(i=0;i<CALIB_MEAN_TIME;i++)
	{
		reg_value += read_measure_reg(measure_reg_addr);	//串口读一次时间很长,远低于刷新率
	}
	reg_value /= CALIB_MEAN_TIME;
	
	calib->x[index] = reg_value;		//reg值作为x, 计算k时分母够大
	calib->y[index] = real_value;

//my_printf("index:%d,reg_value:%d,real_value:%d\n",index,reg_value,real_value);

	if(index == MEASURE_VI_POINTS_CNT - 1)
	{
		calib->k = (float)(calib->y[1] - calib->y[0])/(float)(calib->x[1] - calib->x[0]);
		if(calib->y[0] > calib->y[1])
		{
			calib->b = calib->y[0] - calib->k * calib->x[0];
		}
		else
		{
			calib->b = calib->y[1] - calib->k * calib->x[1];
		}
	}
}

/*******************************************************************************
* Function Name  : 
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
s16 measure_current_param(CALIB_VI_STRUCT xdata * calib, u8 measure_reg_addr)
{
	s32 reg_value = 0;
	u8 i;

	for(i=0;i<MEASURE_MEAN_TIME;i++)
	{
		reg_value += convert_uint24_to_int24(read_measure_reg(measure_reg_addr));

	}
	reg_value /= MEASURE_MEAN_TIME;
//#ifdef DEBUG
	my_printf("measure reg value%d\n",reg_value);
//#endif

	return (s16)(calib->k * reg_value + calib->b);
}


/*******************************************************************************
* Function Name  : 
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void set_v_calib_point(u8 index, s16 v_real_value)
{
	set_calib_point(index, &calib_v, MEASURE_REG_V, v_real_value);
}

/*******************************************************************************
* Function Name  : 
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void set_i_calib_point(u8 index, s16 i_real_value)
{
	set_calib_point(index, &calib_i, MEASURE_REG_I, i_real_value);
}

/*******************************************************************************
* Function Name  : 温度校准使用分段线性
* Description    : if reg_value ==0 直接下分段表
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void set_t_calib_point(u8 index, s16 real_t, s32 reg_value)
{
	u8 i;

	if(index < MEASURE_T_POINTS_CNT)
	{
		if(reg_value==0)
		{
			for(i=0;i<CALIB_MEAN_TIME;i++)
			{
				reg_value += convert_uint24_to_int24(read_measure_reg(MEASURE_REG_T));	//串口读一次时间很长,远低于刷新率
			}
			reg_value /= CALIB_MEAN_TIME;
		}
		
		p_calib_t->t[index] = real_t;			//reg值作为x, 计算k时分母够大
		p_calib_t->reg[index] = reg_value;

		if(index>=p_calib_t->points)			//分段点必须连续, 否则会出错
		{
			p_calib_t->points = index + 1;
		}
		my_printf("index:%d,t:%d,reg_value:%d\r\n",index,real_t,reg_value);
	}
}


/*******************************************************************************
* Function Name  : 
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
s16 measure_current_v(void)
{
	s16 current_v;

	my_printf("calib_v, k:%d, b:%d\r\n", (u32)(calib_v.k*100000), (u32)(calib_v.b*100000));
	
	current_v = measure_current_param(&calib_v, MEASURE_REG_V);
	
	if(current_v>-500 && current_v<500)											//防止6523被复位, HPF关闭, 当检查此时测量电压小于5V, 则复位6523
	{
		init_app_nvr_data();
		init_measure();
		current_v = measure_current_param(&calib_v, MEASURE_REG_V);
	}
	
	return current_v;
}

/*******************************************************************************
* Function Name  : 
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
s16 measure_current_i(void)
{
	my_printf("calib_i, k:%d, b:%d\r\n", (u32)(calib_i.k*100000), (u32)(calib_i.b*100000));
	return measure_current_param(&calib_i, MEASURE_REG_I);
}

/*******************************************************************************
* Function Name  : 
* Description    : p_calib_t必须保证有MEASURE_T_POINTS_CNT个点,必须单调存储t与reg的对应关系
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
//s16 measure_current_t(s32 reg_value)
s16 measure_current_t(void)
{
	s32 reg_value = 0;
	u8 i;
	s16 t1,t2;
	s32 reg1,reg2;
	u8 points;

	for(i=0;i<MEASURE_MEAN_TIME;i++)
	{
		reg_value += convert_uint24_to_int24(read_measure_reg(MEASURE_REG_T));
	}
	reg_value /= MEASURE_MEAN_TIME;

	my_printf("reg_value:%d\r\n",reg_value);

	points = p_calib_t->points;

	if(reg_value < p_calib_t->reg[0])
	{
		t1 = p_calib_t->t[0];
		reg1 = p_calib_t->reg[0];
		t2 = p_calib_t->t[1];
		reg2 = p_calib_t->reg[1];
	}
	else if(reg_value >= p_calib_t->reg[points-1])
	{
		t1 = p_calib_t->t[points-2];
		reg1 = p_calib_t->reg[points-2];
		t2 = p_calib_t->t[points-1];
		reg2 = p_calib_t->reg[points-1];
	}
	else
	{
		for(i=0;i<points-1;i++)
		{
			if(reg_value >= p_calib_t->reg[i] && reg_value < p_calib_t->reg[i+1])
			{
				t1 = p_calib_t->t[i];
				reg1 = p_calib_t->reg[i];
				t2 = p_calib_t->t[i+1];
				reg2 = p_calib_t->reg[i+1];
				break;
			}
		}
	}

	return (s16)(t1 + (float)(t2-t1)*(reg_value-reg1)/(reg2-reg1));
}


/*******************************************************************************
* Function Name  : 
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS save_calib_into_app_nvr(void)
{
	set_app_nvr_data_u(calib_v.k, calib_v.b);
	set_app_nvr_data_i(calib_i.k, calib_i.b);
	return save_app_nvr_data();
}

