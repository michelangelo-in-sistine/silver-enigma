#include "compile_define.h"
#include "powermesh_include.h"
#include "math.h"


#define MEASURE_UNPROTECT()	write_measure_reg(0x3E, 0x00000055)
#define MEASURE_PROTECT()	write_measure_reg(0x3E, 0x00000000)
#define MEASURE_DISABLE_HPF()		write_measure_reg(0x14, 0x0000001C)
#define CHECK_HPF_REG()			(read_measure_reg(0x14)==0x0000001C)

#define MEASURE_RESET()		write_measure_reg(0x3F, 0x005A5A5A)
#define MEASURE_PGA1()		write_measure_reg(0x15, 0x00000000)
#define MEASURE_PGA2()		write_measure_reg(0x15, 0x00000101)
#define MEASURE_PGA4()		write_measure_reg(0x15, 0x00000202)
#define MEASURE_PGA8()		write_measure_reg(0x15, 0x00000303)
#define MEASURE_PGA16()		write_measure_reg(0x15, 0x00000304)		//2016-08-31 V通道使用8倍放大倍数, 避免40V超过输入ADC量程
#define MEASURE_PGA24()		write_measure_reg(0x15, 0x00000505)
#define MEASURE_PGA32()		write_measure_reg(0x15, 0x00000606)

#define MEASURE_REG_I		0x01		//电流测量通道IA
#define MEASURE_REG_T		0x02		//温度测量通道IB
#define MEASURE_REG_V		0x03		//电压测量通道V

#define MEASURE_POINTS_CNT	2
#define CALIB_MEAN_TIME		64
#define CALIB_MEAN_SHIFT	6			//log2(CALIB_MEAN_TIME), 用于四舍五入快速计算

#define MEASURE_MEAN_TIME	4

#define integer_round_shift(value, shift_bits)	 ((value & (0x01<<(shift_bits-1))) ? ((value >> shift_bits) + 1) : (value >> shift_bits))
//#define integer_round_shift(value, shift_bits)	 (value >> shift_bits)

//#define float_to_round_int(f)	((s16)((f>=0) ? (f+0.5) : (f-0.5)))
#define float_to_round_int(f)	((s16)(f))			

typedef struct
{
	s32 x[MEASURE_POINTS_CNT];
	s32 y[MEASURE_POINTS_CNT];
	float k;
	float b;
}LINEAR_CALIB_STRUCT;

typedef struct
{
	float t0;
}EXP_CALIB_STRUCT;

LINEAR_CALIB_STRUCT xdata calib_v;
LINEAR_CALIB_STRUCT xdata calib_i;
EXP_CALIB_STRUCT xdata calib_t;


/*******************************************************************************
* Function Name  : init_measure
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void init_measure(void)
{
	u16 i;

	init_measure_hardware();
	reset_measure_device();

	for(i=0;i<10;i++)
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
#ifdef DEBUG_MODE		
			my_printf("measure reg fail\r\n");
#endif
		}
	}

	if(is_app_nvr_data_valid())
	{
#ifdef DEBUG_MODE
		my_printf("calib data valid\r\n");
#endif
		calib_v.k = get_app_nvr_data_u_k();
		calib_v.b = get_app_nvr_data_u_b();
		calib_i.k = get_app_nvr_data_i_k();
		calib_i.b = get_app_nvr_data_i_b();
		calib_t.t0 = get_app_nvr_data_t0();
	}
	else
	{
#ifdef DEBUG_MODE
		my_printf("no valid calib data\r\n");
#endif
	}
}



/*******************************************************************************
* Function Name  : convert_uint24_to_int24
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
s32 convert_uint24_to_int24(u32 value)
{
#if MEASURE_DEVICE == BL6523GX
	if(value & 0xFF800000)
	{
		return (s32)((value&0x00FFFFFF) - 0x01000000);
	}
#elif MEASURE_DEVICE == BL6523B					//6523B有效数字22位,高2位都是符号位,所以可以看成是21位
	if(value & 0x200000)
	{
		return (s32)((value&0x003FFFFF) - 0x00400000);
	}
#endif
	else
	{
		return (s32)value;
	}
}


/*******************************************************************************
* Function Name  : read_mean_measure_reg
* Description    : 6810+BL6523B 一次电压电流SPI测量时间为400us
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
s32 read_mean_measure_reg(u8 measure_reg_addr)
{
	u8 i;
	s32 reg_value = 0;
	
	for(i=0;i<CALIB_MEAN_TIME;i++)
	{
		reg_value += convert_uint24_to_int24(read_measure_reg(measure_reg_addr));	//串口读一次时间很长,远低于刷新率
		FEED_DOG();
	}

	reg_value = integer_round_shift(reg_value,CALIB_MEAN_SHIFT);

	return reg_value;
}


/*******************************************************************************
* Function Name  : set_linear_calib_point
* Description    : 
* Input          : 
* Output         : 
* Return         : reg_value
*******************************************************************************/
s32 set_linear_calib_point(u8 index, LINEAR_CALIB_STRUCT xdata * calib, u8 measure_reg_addr, s16 real_value)
{
	s32 reg_value = 0;

	if(index>=MEASURE_POINTS_CNT)
	{
#ifdef DEBUG_MODE
		my_printf("calib point index error\n");
#endif
		return 0;
	}

	reg_value = read_mean_measure_reg(measure_reg_addr);

	calib->x[index] = reg_value;		//reg值作为x, 计算k时分母够大
	calib->y[index] = real_value;

my_printf("index:%bu,reg_value:%ld,real_value:%d\n",index,reg_value,real_value);

	if(index == MEASURE_POINTS_CNT - 1)
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

	return reg_value;
}

/*******************************************************************************
* Function Name  : measure_current_param
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
s16 measure_current_param(LINEAR_CALIB_STRUCT xdata * calib, u8 measure_reg_addr)
{
	s32 reg_value;

	reg_value = read_mean_measure_reg(measure_reg_addr);
#ifdef DEBUG_MODE
	my_printf("measure reg value %ld\n",reg_value);
#endif

	return float_to_round_int(calib->k * reg_value + calib->b);
}


/*******************************************************************************
* Function Name  : set_v_calib_point
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
s32 set_v_calib_point(u8 index, s16 v_real_value)
{
	return set_linear_calib_point(index, &calib_v, MEASURE_REG_V, v_real_value);
}

/*******************************************************************************
* Function Name  : set_i_calib_point
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
s32 set_i_calib_point(u8 index, s16 i_real_value)
{
	return set_linear_calib_point(index, &calib_i, MEASURE_REG_I, i_real_value);
}


/*******************************************************************************
* Function Name  : measure_current_v
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
s16 measure_current_v(void)
{
	s16 current_v;

#ifdef DEBUG_MODE
	my_printf("calib_v, k:%ld, b:%ld\r\n", (s32)(calib_v.k*100000), (s32)(calib_v.b*100000));
#endif
	
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
* Function Name  : measure_current_i
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
s16 measure_current_i(void)
{
#ifdef DEBUG_MODE
	my_printf("calib_i, k:%ld, b:%ld\r\n", (s32)(calib_i.k*100000), (s32)(calib_i.b*100000));
#endif
	return measure_current_param(&calib_i, MEASURE_REG_I);
}

/*******************************************************************************
* Function Name  : save_calib_into_app_nvr
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
STATUS save_calib_into_app_nvr(void)
{
	set_app_nvr_data_u(calib_v.k, calib_v.b);
	set_app_nvr_data_i(calib_i.k, calib_i.b);
	set_app_nvr_data_t0(calib_t.t0);
	return save_app_nvr_data();
}

#define ADC_OFFSET		(-3.3)
#define ADC_MV_PER_STEP (8.2244e-004)			//mv per step
#define CONST_B 			(3950)

/*******************************************************************************
* Function Name  : calc_exp_index
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
float calc_exp_index(s32 reg_value)
{
	float v;
	float r;
	float index;

	v = (reg_value - ADC_OFFSET) * ADC_MV_PER_STEP;			//voltage in theromal resistor, unit: mv
	r = (5/v*1000 - 11);									//resistor value, unit: k ohm
	//index = log(r/10)/CONST_B;
	//index = lg(r/10)*(2.3026)/CONST_B;						//lg为10为底
	index = lg(r/10)/(1715.5);							// const_b/log(10) = 1715.5
	
	return index;
}

/*******************************************************************************
* Function Name  : set_exp_calib_point
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
float set_exp_calib_point(s16 t_real_value, s32 reg_value)
{
	float index;
	float t;
	
	index = calc_exp_index(reg_value);
	t = (float)(t_real_value)/100 + 273.15;
	t = t/(1-index*t);

	calib_t.t0 = t;

	return t;
}

/*******************************************************************************
* Function Name  : set_t_calib_point
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
void set_t_calib_point(s16 t_real_value)
{
	s32 reg_value;
	float t;
	
	reg_value = read_mean_measure_reg(MEASURE_REG_T);
	t = set_exp_calib_point(t_real_value, -reg_value);

	calib_t.t0 = t;
	return;
}

/*******************************************************************************
* Function Name  : calc_temperature
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
s16 calc_temperature(s32 reg_value)
{
	float xdata index;
	float xdata t;

	index = calc_exp_index(reg_value);
	t = (calib_t.t0/(1+index*calib_t.t0) - 273.15)*100;
	return (s16)t;
}

/*******************************************************************************
* Function Name  : measure_current_t
* Description    : 
* Input          : 
* Output         : 
* Return         : 
*******************************************************************************/
s16 measure_current_t(void)
{
	s32 xdata reg_value;
	
	reg_value = read_mean_measure_reg(MEASURE_REG_T);
#ifdef DEBUG_MODE
	my_printf("reg_value:%d,",reg_value);
#endif
	return calc_temperature(-reg_value);
}

