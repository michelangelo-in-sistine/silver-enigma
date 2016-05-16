#include "powermesh_include.h"

int main(void)
{
	init_powermesh();

	while(1)
	{
		powermesh_event_proc();
//{
//					DLL_SEND_STRUCT xdata dds;
//					ARRAY_HANDLE buffer;
//					u8 target_uid[6];
//					u8 i;

//					for(i=0;i<6;i++)
//					{
//						target_uid[i] = 0xff;
//					}
//					
//	
//					mem_clr(&dds,sizeof(DLL_SEND_STRUCT),1);

//					dds.target_uid_handle = target_uid;

//					dds.xmode = 0x10;
//					dds.rmode = 0x10;
//					dds.prop = BIT_DLL_SEND_PROP_DIAG | BIT_DLL_SEND_PROP_REQ_ACK;
//					dds.prop |= BIT_DLL_SEND_PROP_SCAN;
//	
//					buffer = OSMemGet(MINOR);
//					if(buffer != NULL)
//					{
//						if(dll_diag(&dds, buffer))
//						{
//							u8 i;
//	//						uart_send_asc(buffer+1,buffer[0]);
//							my_printf("\r\nTarget:");
//							uart_send_asc(dds.target_uid_handle,6);
//							my_printf("\r\n------------------------------------------\r\n\tDownlink\t\tUplink\t\r\n");
//							my_printf("Diag\tSS(dbuv)\tSNR(dB)\tSS(dbuv)\tSNR(dB)\r\n");
//							for(i=0;i<4;i++)
//							{
//								my_printf("CH%bu\t%bd\t%bd",i,buffer[i*2+1],buffer[i*2+2]);
//								my_printf("\t%bd\t%bd\r\n",buffer[i*2+9],buffer[i*2+10]);
//							}
//							
//							
//							my_printf("\r\n");
//						}
//						else
//						{
//							my_printf("Diag Fail!\r\n");
//						}
//						OSMemPut(MINOR,buffer);
//					}

//}









	
		debug_uart_cmd_proc();
	}
}
