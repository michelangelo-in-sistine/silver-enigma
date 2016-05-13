#include "compile_define.h"
#include "powermesh_include.h"

int main(void)
{
	u32 cnt = 0;
    
	init_powermesh();


	init_timer();
	init_uart();

	init_measure();

	my_printf("/**************************\n");
	my_printf("*****STM32F030 PV Demo*****\n");
	my_printf("**************************/\n");

	while(1)
	{
		if(cnt++>50000UL)
		{
			cnt = 0;
		}
	
		debug_uart_cmd_proc();
	}
}
