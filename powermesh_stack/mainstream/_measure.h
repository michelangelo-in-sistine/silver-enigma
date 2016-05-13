void init_measure(void);
u32 read_measure_reg(u8 addr);
void write_measure_reg(u8 addr, u32 dword_value);


void set_v_calib_point(u8 index, s32 v_real_value);
void set_i_calib_point(u8 index, s32 i_real_value);
s32 measure_current_v(void);
s32 measure_current_i(void);





