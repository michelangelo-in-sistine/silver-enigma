void init_uart(void);
void my_printf(const char code * fmt, ...) reentrant;
void uart_rcv_int_svr(u8 rec_data);

void debug_uart_cmd_proc(void);
void uart_send_asc(ARRAY_HANDLE pt, u16 len) reentrant;

void print_phy_rcv(PHY_RCV_HANDLE pp) reentrant;

