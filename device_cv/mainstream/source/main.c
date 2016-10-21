#include "powermesh_include.h"
#include "math.h"
#include "_interface.h"

int main(void)
{
	init_powermesh();
	init_interface();

	while(1)
	{
//		powermesh_event_proc();
		interface_proc();
//		my_printf("test");
//		{
//			u16 i,j;
//			for(i=0;i<100;i++)
//			{
//				for(j=0;j<1000;j++);
//			}
//		}
	}
}
