#include "powermesh_include.h"
#include "math.h"
#include "_interface.h"

int main(void)
{
	init_powermesh();
	init_interface();

	while(1)
	{
		powermesh_event_proc();
		interface_proc();
	}
}
