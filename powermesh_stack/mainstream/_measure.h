#ifndef _MEASURE_H
#define _MEASURE_H

#define MEASURE_VI_POINTS_CNT	2
#define MEASURE_T_POINTS_CNT	8		//{-40 -30 -20 -10 0 10 20 30 40 50 60 70 80 90 100 110}

typedef struct
{
	s32 x[MEASURE_VI_POINTS_CNT];
	s32 y[MEASURE_VI_POINTS_CNT];
	float k;
	float b;
}CALIB_VI_STRUCT;


typedef struct
{
	s16 t[MEASURE_T_POINTS_CNT];	//低8位为小数点
	s32 reg[MEASURE_T_POINTS_CNT];
	u8	points;
}CALIB_T_STRUCT;


void init_measure(void);
u32 read_measure_reg(u8 addr);
void write_measure_reg(u8 addr, u32 dword_value);

void set_v_calib_point(u8 index, s16 v_real_value);
void set_i_calib_point(u8 index, s16 i_real_value);
//void set_t_calib_point(u8 index, s16 t_real_value);
void set_t_calib_point(u8 index, s16 real_t, s32 reg_value);


s16 measure_current_v(void);
s16 measure_current_i(void);
s16 measure_current_t(s32 reg_value);

STATUS save_calib_into_app_nvr(void);



#endif
