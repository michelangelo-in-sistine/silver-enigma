#include "_powermesh_datatype.h"
#include "_fraz.h"
#include "_app.h"
#include "test_func.h"
#include "stdio.h"
//#include "assert.h"

int main(void)
{
	s16 u,i,t;

	init_fraz();
	
	/* Basic Function */
	assert(write_fraz_record(0x91,BIT_ACP_CMD_MASK_U,100)== FAIL);	//禁止未req就写入
	assert(read_fraz_record(0x91,BIT_ACP_CMD_MASK_U,&u) == FAIL);
	
	req_fraz_record(0x91);
	write_fraz_record(0x91,BIT_ACP_CMD_MASK_U,100);
	assert(write_fraz_record(0x91,BIT_ACP_CMD_MASK_U,1000)==FAIL);	//禁止重复写入
	req_fraz_record(0x92);
	write_fraz_record(0x92,BIT_ACP_CMD_MASK_U,200);
	req_fraz_record(0x93);
	write_fraz_record(0x93,BIT_ACP_CMD_MASK_U,300);
	req_fraz_record(0x94);
	write_fraz_record(0x94,BIT_ACP_CMD_MASK_U,400);
	
	assert(read_fraz_record(0x91,BIT_ACP_CMD_MASK_U,&u)==OKAY);
	assert(u==100);
	
	req_fraz_record(0x92);										// same feature code will NOT flush oldest record
	assert(read_fraz_record(0x91,BIT_ACP_CMD_MASK_U,&u)==OKAY);
	assert(u==100);
	
	req_fraz_record(0x95);										// new feature code will flush oldest record
	assert(read_fraz_record(0x91,BIT_ACP_CMD_MASK_U,&u)==FAIL);
	
	assert(read_fraz_record(0x92,BIT_ACP_CMD_MASK_U,&u)==FAIL);		// 0x92重req过,已被清除
	
	assert(read_fraz_record(0x93,BIT_ACP_CMD_MASK_U,&u)==OKAY);
	assert(u==300);

	assert(read_fraz_record(0x94,BIT_ACP_CMD_MASK_U,&u)==OKAY);
	assert(u==400);
	
	assert(read_fraz_record(0x95,BIT_ACP_CMD_MASK_U,&u)==FAIL);	// 0x95仅申请空间, 未加数据, 此时 mask为0
	
	
	// 测试电流,温度
	req_fraz_record(0x93);
	write_fraz_record(0x93,BIT_ACP_CMD_MASK_U,100);
	write_fraz_record(0x93,BIT_ACP_CMD_MASK_I,1000);
	write_fraz_record(0x93,BIT_ACP_CMD_MASK_T,2300);

	assert(read_fraz_record(0x93,BIT_ACP_CMD_MASK_U,&u)==OKAY);
	assert(read_fraz_record(0x93,BIT_ACP_CMD_MASK_I,&i)==OKAY);
	assert(read_fraz_record(0x93,BIT_ACP_CMD_MASK_T,&t)==OKAY);
	assert(read_fraz_record(0x93,0x80,&u)==FAIL);
	
	assert(u==100);
	assert(i==1000);
	assert(t==2300);
	
	
	
	printf("unit test finish");
	
	
}
