#include "powermesh_include.h"
#include "math.h"

u8 xdata apdu[100];
u8 xdata apdu_len;

int main(void)
{
	APP_RCV_STRUCT ars;
	TIMER_ID_TYPE xdata tid;
	
	init_powermesh();

	init_app_nvr_data();
	init_measure();
	init_app();

//	set_v_calib_point(0, 0);
//	set_i_calib_point(0, 0);
//	set_t_calib_point(0);
//	save_calib_into_app_nvr();
//	measure_current_t();

	tid = req_timer();
	set_timer(tid,200);

	while(1)
	{
		if(is_timer_over(tid))
		{
			led_d_flash();
			set_timer(tid,200);
		}

		powermesh_event_proc();

		ars.apdu = apdu;
		apdu_len = app_rcv(&ars);
		if(apdu_len)
		{
			acp_rcv_proc(&ars);
		}
	}
}

