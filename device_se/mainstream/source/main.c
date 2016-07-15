#include "powermesh_include.h"
#include "math.h"

u8 apdu[100];
u8 apdu_len;

int main(void)
{
	APP_RCV_STRUCT ars;
	
	init_powermesh();

	init_app_nvr_data();
	init_measure();
	init_app();

	while(1)
	{
		powermesh_event_proc();
		debug_uart_cmd_proc();

		ars.apdu = apdu;
		apdu_len = app_rcv(&ars);
		if(apdu_len)
		{
			my_printf("rcv comm_mode:%x\r\n",ars.comm_mode);
			acp_rcv_proc(&ars);
		}
	}
}
