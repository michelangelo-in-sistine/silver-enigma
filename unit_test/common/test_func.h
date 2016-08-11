void mem_clr(void xdata * mem_block_addr, u16 mem_block_size, u16 mem_block_cnt) reentrant;
void mem_cpy(ARRAY_HANDLE pt_dest, ARRAY_HANDLE pt_src, BASE_LEN_TYPE len) reentrant;
void my_printf(const char code * fmt, ...) reentrant;

//#define assert(e,str) ((e) ? 0 : printf(str))
//#define assert(e,str) ((e) ? 0 : (printf(str);printf("at %s,%d",__FILE__,__LINE__)))
#define assert(e) ((e) ? 0 : (printf("ASSERT VOILATED! FILE:%s,LINE:%d,\"%s\"\n",__FILE__,__LINE__,#e)))

