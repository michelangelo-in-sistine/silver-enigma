/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _GENERAL_H
#define _GENERAL_H

/* Includes ------------------------------------------------------------------*/
//#include "hardware.h"

/* Exported variants ------------------------------------------------------- */
extern u8 xdata SELF_UID[];

/* Exported functions ------------------------------------------------------- */

/* Powermesh External Interface */
void init_powermesh(void);
void powermesh_event_proc(void);
void powermesh_main_thread_useless_resp_clear(void);
void get_timing_version(TIMING_VERSION_HANDLE ver_handle);

/* System */
extern void reset_system(void);				//implemented in hardware();
u8 read_reg(u8 phase, u8 addr);

/* Mem Operation */
void mem_cpy(ARRAY_HANDLE pt_dest, ARRAY_HANDLE pt_src, BASE_LEN_TYPE len) reentrant;
BOOL mem_cmp(ARRAY_HANDLE pt1, ARRAY_HANDLE pt2, BASE_LEN_TYPE len) reentrant;
void mem_clr(void xdata * mem_block_addr, u16 mem_block_size, u16 mem_block_cnt) reentrant;
void mem_shift(ARRAY_HANDLE mem_addr, BASE_LEN_TYPE mem_len, BASE_LEN_TYPE shift_bytes) reentrant;

RESULT check_struct_array_handle(ARRAY_HANDLE handle, ARRAY_HANDLE struct_array_addr, u16 struct_size, u8 struct_cnt) reentrant;

/* Judgement Operation */
BOOL is_xmode_bpsk(u8 xmode);
BOOL is_xmode_dsss(u8 xmode);

RESULT check_ack_src(ARRAY_HANDLE pt_ack_src, ARRAY_HANDLE pt_dest);

/* CRC, CS Calculation */
RESULT check_crc(ARRAY_HANDLE pt, BASE_LEN_TYPE len);
u16 calc_crc(ARRAY_HANDLE pt, BASE_LEN_TYPE len); //calc_crc in MT use Hardware, in CC use software
u8 calc_cs(ARRAY_HANDLE pt, BASE_LEN_TYPE len) reentrant;
RESULT check_cs(ARRAY_HANDLE pt, BASE_LEN_TYPE len);
RESULT check_srf(ARRAY_HANDLE pt);


/* SNR Calculation */
s8 dbuv(u16 pos, u8 agc_value);
s8 ebn0(u16 pos, u16 pon);

/* Timing Related */
u32 packet_chips(BASE_LEN_TYPE ppdu_len, u8 rate, u8 srf) reentrant;


/* UID MID Operation */
extern void get_uid(u8 phase, ARRAY_HANDLE pt);	// function body in hardware.c
void set_meter_id(METER_ID_HANDLE meter_id_handle);
RESULT is_meter_id_valid(void);
RESULT check_meter_id(METER_ID_HANDLE meter_id_handle) ;

/* xmode encode - decode */
u8 encode_xmode(u8 xmode, u8 scan);
u8 decode_xmode(u8 xcode);

/* zx detection */
#if POWERLINE==PL_AC
BOOL is_zx_valid(u8 phase);
#else
#define is_zx_valid(phase) (1)
#endif


/* LED */
#ifdef DEBUG_MODE
//extern void debug_led_flash(void);
//extern void debug_led_on(void);
//extern void debug_led_off(void);
#endif

/* LOOPS CONTROL */
#if DEVICE_TYPE==DEVICE_CC
BOOL check_quit_loops(void);
void set_quit_loops(void);
void clr_quit_loops(void);
u8 get_noise_status(u8 phase);
void mfoi_report(u8 phase, u8 ch);

#ifdef DEBUG_DISP_INFO
	#define SET_BREAK_THROUGH(incication) if(check_quit_loops()){my_printf(incication);break;}
#else
	#define SET_BREAK_THROUGH(incication) if(check_quit_loops()){break;}
#endif

#endif

#if BRING_USER_DATA == 1
STATUS set_user_data(ARRAY_HANDLE user_data, u8 user_data_len);
#endif
void powermesh_clear_apdu_rcv(void);

#endif

