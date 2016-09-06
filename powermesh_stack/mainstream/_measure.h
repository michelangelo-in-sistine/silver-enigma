void init_measure(void);
u32 read_measure_reg(u8 addr);
void write_measure_reg(u8 addr, u32 dword_value);


s32 set_v_calib_point(u8 index, s16 v_real_value);
s32 set_i_calib_point(u8 index, s16 i_real_value);
s16 measure_current_v(void);
s16 measure_current_i(void);

STATUS save_calib_into_app_nvr(void);

float set_exp_calib_point(s16 t_real_value, s32 reg_value);
void set_t_calib_point(s16 t_real_value);
s16 calc_temperature(s32 reg_value);
s16 measure_current_t(void);


