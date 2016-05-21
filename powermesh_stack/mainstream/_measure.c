#include "compile_define.h"
#include "powermesh_include.h"


#define MEASURE_UNPROTECT()	write_measure_reg(0x3E, 0x00000055)
#define MEASURE_PROTECT()	write_measure_reg(0x3E, 0x00000000)
#define MEASURE_DISABLE_HPF()		write_measure_reg(0x14, 0x0000001C)

#define MEASURE_RESET()		write_measure_reg(0x3F, 0x005A5A5A)
#define MEASURE_PGA1()		write_measure_reg(0x15, 0x00000000)
#define MEASURE_PGA2()		write_measure_reg(0x15, 0x00000111)
#define MEASURE_PGA4()		write_measure_reg(0x15, 0x00000222)
#define MEASURE_PGA8()		write_measure_reg(0x15, 0x00000333)
#define MEASURE_PGA16()		write_measure_reg(0x15, 0x00000404)
#define MEASURE_PGA24()		write_measure_reg(0x15, 0x00000555)
#define MEASURE_PGA32()		write_measure_reg(0x15, 0x00000666)

#define MEASURE_REG_I		0x01		//电流测量通道IA
#define MEASURE_REG_T		0x02		//温度测量通道IB
#define MEASURE_REG_V		0x03		//电压测量通道V


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
	MEASURE_UNPROTECT();
	MEASURE_DISABLE_HPF();
	MEASURE_PGA16();
	MEASURE_PROTECT();
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

#define MEASURE_POINTS_CNT	2
#define CALIB_MEAN_TIME		4
#define MEASURE_MEAN_TIME	4


typedef struct
{
	s32 x[MEASURE_POINTS_CNT];
	s32 y[MEASURE_POINTS_CNT];
	float k;
	float b;
}CALIB_STRUCT;

CALIB_STRUCT xdata calib_v, calib_i;


//void set_calib_point_test(u8 index, s32 reg_value, s32 real_value)
//{
//	calib_v.x[index] = reg_value;
//	calib_v.y[index] = real_value;

//	if(index == MEASURE_POINTS_CNT - 1)
//	{
//		calib_v.k = (float)(calib_v.y[1] - calib_v.y[0])/(float)(calib_v.x[1] - calib_v.x[0]);
//		if(calib_v.y[0] > calib_v.y[1])
//		{
//			calib_v.b = calib_v.y[0] - calib_v.k * calib_v.x[0];
//		}
//		else
//		{
//			calib_v.b = calib_v.y[1] - calib_v.k * calib_v.x[1];
//		}
//	}
//}

//s32 measure_test(s32 reg_value)
//{
//	return (s32)(calib_v.k * reg_value + calib_v.b);
//}

void set_calib_point(u8 index, CALIB_STRUCT xdata * calib, u8 measure_reg_addr, s16 real_value)
{
	s32 reg_value = 0;
	u8 i;

	if(index>=MEASURE_POINTS_CNT)
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

my_printf("index:%d,reg_value:%d,real_value:%d\n",index,reg_value,real_value);

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
}

s16 measure_current_param(CALIB_STRUCT xdata * calib, u8 measure_reg_addr)
{
	s32 reg_value = 0;
	u8 i;

	for(i=0;i<MEASURE_MEAN_TIME;i++)
	{
		reg_value += convert_uint24_to_int24(read_measure_reg(measure_reg_addr));

	}
	reg_value /= MEASURE_MEAN_TIME;
#ifdef DEBUG
	my_printf("measure reg value%d\n",reg_value);
#endif

	return (s16)(calib->k * reg_value + calib->b);
}


void set_v_calib_point(u8 index, s16 v_real_value)
{
	set_calib_point(index, &calib_v, MEASURE_REG_V, v_real_value);
}

void set_i_calib_point(u8 index, s16 i_real_value)
{
	set_calib_point(index, &calib_i, MEASURE_REG_I, i_real_value);
}




s16 measure_current_v(void)
{
	return measure_current_param(&calib_v, MEASURE_REG_V);
}

s16 measure_current_i(void)
{
	return measure_current_param(&calib_i, MEASURE_REG_I);
}



