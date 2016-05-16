/******************** (C) COPYRIGHT 2012 Belling Inc. ********************
* File Name          : rscodec.c
* Author             : Lv Haifeng
* Version            : V1.0.3
						2008-08-28 Draft Ver. By Lv Haifeng
						2012-09-11 Imported to ARM STM32 Platform, By Lv Haifeng
* Date               : 10/29/2012
* Description        : USE FOR STM32
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODECTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, BELLING INC. SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAINMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/
	
/* Includes ------------------------------------------------------------------*/
//#include "compile_define.h"
//#include "powermesh_datatype.h"
//#include "powermesh_config.h"
#include "powermesh_include.h"
#ifdef USE_RSCODEC
/* Private function prototypes -----------------------------------------------*/
u8 rsdec(ARRAY_HANDLE inputadr, ARRAY_HANDLE outputadr, u8 datalen);
void rsenc(ARRAY_HANDLE inputadr, ARRAY_HANDLE outputadr, u8 datalen);

/* Private define ------------------------------------------------------------
M is 8, because the data we want to code is 8-bit data symbol
N is the coded block length, which must be less than 2^M
K is the original data length, which must be less than N
and (N-K)/2 decides how many error symbols can be contained in a coded block
------------------------------------------------------------------------------*/
#define M 8
#define BOUND 255		// bound = 2^M - 1
#define N 20
#define K 10
#define T2 10			// 2t = n - k, which is the redundancy of each encoded block
#define T 5				// t = (n - k)/2, which is the correct capacity of each coded block

/* Private macro & constant -------------------------------------------------------------*/
#define MODADD(k,i,j) if((u8)(i+j)<i) k=i+j+1; else k = i+j; if(k==255) k=0
#define MODSUB(k,i,j) if(i>=j) k=i-(j);else k=i-(j)-1

//genepoly is the generate polynomial of RS coding
//once the GF tables above has been decided, the genepoly here is certain
//it is actually expanded format of (x-a)(x-a^2)...(x-a^(2t))
//here 2t constant is g(0)x^(2t-1)+g(1)x^(2t-2)+...g(2t)
//the highest coefficient of genepoly is 1, so no need to list here
/*uint8 code genepoly[] = {173, 47, 140, 190, 197, 30, 188, 68, 212, 160};*/
//finally I choose a exponent format to restore genepoly, if genepoly has zero
//terms, here present 2^M-1, which is a number shouldn't exist in exponent format
u8 genepoly[] = {252, 69, 49, 65, 123, 76, 71, 102, 41, 55};
u8 norm2exp[] = {255,   0,   1,  25,   2,  50,  26, 198,   3, 223,  51, 238,  27, 104,
   199,  75,   4, 100, 224,  14,  52, 141, 239, 129,  28, 193, 105,
   248, 200,   8,  76, 113,   5, 138, 101,  47, 225,  36,  15,  33,
    53, 147, 142, 218, 240,  18, 130,  69,  29, 181, 194, 125, 106,
    39, 249, 185, 201, 154,   9, 120,  77, 228, 114, 166,   6, 191,
   139,  98, 102, 221,  48, 253, 226, 152,  37, 179,  16, 145,  34,
   136,  54, 208, 148, 206, 143, 150, 219, 189, 241, 210,  19,  92,
   131,  56,  70,  64,  30,  66, 182, 163, 195,  72, 126, 110, 107,
    58,  40,  84, 250, 133, 186,  61, 202,  94, 155, 159,  10,  21,
   121,  43,  78, 212, 229, 172, 115, 243, 167,  87,   7, 112, 192,
   247, 140, 128,  99,  13, 103,  74, 222, 237,  49, 197, 254,  24,
   227, 165, 153, 119,  38, 184, 180, 124,  17,  68, 146, 217,  35,
    32, 137,  46,  55,  63, 209,  91, 149, 188, 207, 205, 144, 135,
   151, 178, 220, 252, 190,  97, 242,  86, 211, 171,  20,  42,  93,
   158, 132,  60,  57,  83,  71, 109,  65, 162,  31,  45,  67, 216,
   183, 123, 164, 118, 196,  23,  73, 236, 127,  12, 111, 246, 108,
   161,  59,  82,  41, 157,  85, 170, 251,  96, 134, 177, 187, 204,
    62,  90, 203,  89,  95, 176, 156, 169, 160,  81,  11, 245,  22,
   235, 122, 117,  44, 215,  79, 174, 213, 233, 230, 231, 173, 232,
   116, 214, 244, 234, 168,  80,  88, 175};	 //norm2exp[n] convert n into exponent format
u8 exp2norm[] = {1,   2,   4,   8,  16,  32,  64, 128,  29,  58, 116, 232,
   205, 135,  19,  38,  76, 152,  45,  90, 180, 117, 234, 201,
   143,   3,   6,  12,  24,  48,  96, 192, 157,  39,  78, 156,
    37,  74, 148,  53, 106, 212, 181, 119, 238, 193, 159,  35,
    70, 140,   5,  10,  20,  40,  80, 160,  93, 186, 105, 210,
   185, 111, 222, 161,  95, 190,  97, 194, 153,  47,  94, 188,
   101, 202, 137,  15,  30,  60, 120, 240, 253, 231, 211, 187,
   107, 214, 177, 127, 254, 225, 223, 163,  91, 182, 113, 226,
   217, 175,  67, 134,  17,  34,  68, 136,  13,  26,  52, 104,
   208, 189, 103, 206, 129,  31,  62, 124, 248, 237, 199, 147,
    59, 118, 236, 197, 151,  51, 102, 204, 133,  23,  46,  92,
   184, 109, 218, 169,  79, 158,  33,  66, 132,  21,  42,  84,
   168,  77, 154,  41,  82, 164,  85, 170,  73, 146,  57, 114,
   228, 213, 183, 115, 230, 209, 191,  99, 198, 145,  63, 126,
   252, 229, 215, 179, 123, 246, 241, 255, 227, 219, 171,  75,
   150,  49,  98, 196, 149,  55, 110, 220, 165,  87, 174,  65,
   130,  25,  50, 100, 200, 141,   7,  14,  28,  56, 112, 224,
   221, 167,  83, 166,  81, 162,  89, 178, 121, 242, 249, 239,
   195, 155,  43,  86, 172,  69, 138,   9,  18,  36,  72, 144,
    61, 122, 244, 245, 247, 243, 251, 235, 203, 139,  11,  22,
    44,  88, 176, 125, 250, 233, 207, 131,  27,  54, 108, 216,
   173,  71,  142};      

/*******************************************************************************
* Function Name  : gfpolynomialcalc()
* Description    : calculation the p0 + p1*x^1 + p2*x^2 ... + pn*x^n
					substitute x for a^order
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
u8 gfpolynomialcalc(u8 *poly, u8 polylen, u8 order)
{
	u8 rslt;
	u8 i;
	u8 temp;
	u8 temporder;

	temporder = 0;
	rslt = poly[0];
	for(i=1;i<polylen;i++)
	{
		MODADD(temporder,temporder,order);
		temp = poly[i];
		if(temp!=0)
		{
			temp = norm2exp[temp];
			MODADD(temp,temp,temporder);
			temp = exp2norm[temp];
			rslt ^= temp;
		}
	}
	return rslt;
}

/*******************************************************************************
* Function Name  : rsdec()
* Description    : 	rsdec is used to decoding, inputadr is the first data address of 
					encoded data and the decoded data will be
					put into the buffer which is pointed out by outputadr
					the output buffer must be the same size with input data
					also you can set outputadr the same to inputadr
					so rsdec will correct the error symbol just on the original input data
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
u8 rsdec(ARRAY_HANDLE inputadr, ARRAY_HANDLE outputadr, u8 datalen)
{
	u8 syndromes[T2];			//substitue a^1,a^2,a^3...a^T2 for x in R_(n-1)*x^(n-1)+...R_1*x^1+R0
	u8 lamdax[T+1];			//the equation whose root is the errors' position
	u8 lastlamdax[T+1];		//variable for BM algorithm
	u8 delta;					//variable for BM algorithm
	u8 lx = 0;				//variable for BM algorithm
	u8 tx[T+1];				//variable for BM algorithm

	u8 root[T];				//
	u8 rootcnt=0;

	u8 omegax[T2+1];

	u8 exp_vec[N];			//exponent for data, reused by multiple times

	u8 i,j,temp,temp2,temp3; //temporary variables
	ARRAY_HANDLE ptrin;

	/****************************************************
	Step 1. get the lamda equation by BM algorithm
	****************************************************/
	// Syndromes calculation
	// first calculate the input data's exponents
	ptrin = inputadr;
	for(i=0; i<datalen; i++)
	{
		if(*ptrin != 0)
		{
			exp_vec[i] = norm2exp[*ptrin];
		}
		ptrin += 1;
	}
	// then calculate all the 2t syndromes
	temp2 = 0;
	for(i=0; i<T2; i++)
	{
		// syndromes calculation
		temp = 0;
		for(j=0; j<datalen; j++)
		{
			if(*(inputadr+j) != 0)
			{
				MODADD(temp2,exp_vec[j],datalen);
				MODSUB(temp2,temp2,j+1);
				exp_vec[j]=temp2;
				temp ^= exp2norm[temp2];
			}
		}
		temp2 = temp2 || (temp!=0);				// check if the syndrome is 0
		syndromes[i] = temp;
	}
	// temp2==0 means the received data is correct, and all the other calculation is not needed
	// so just copy the input data to the output data and quit
	if(temp2==0)
	{
		for(i=0;i<datalen;i++)
		{
			outputadr[i] = inputadr[i];
		}
		return 1;
	}

	//initialization lamda(x)
	for(i=0; i<T+1; i++)
	{
		lamdax[i] = 0;
		lastlamdax[i] = 0;
		tx[i] = 0;
	}
	lamdax[0] = 1;
	tx[1] = 1;

	//Main body of BM algorithm
	for(i=0; i<T2; i++)
	{
		//calculate delta
		delta = syndromes[i];
		for(j=1; j<=lx; j++)
		{
			temp = lamdax[j];
			temp2 = syndromes[i-j];
			if(temp!=0 && temp2!=0)
			{
				temp = norm2exp[temp];
				temp2 = norm2exp[temp2];
				MODADD(temp,temp,temp2);
				delta ^= exp2norm[temp];
			}
		}

		// update lamda(x)
		if(delta!=0)
		{
			for(j=0; j<=T; j++)
			{
				lastlamdax[j] = lamdax[j];		//make lamda(x) a backup
			}
			for(j=0; j<=T; j++)
			{
				if(tx[j]!=0)
				{
					temp = norm2exp[tx[j]];
					temp2 = norm2exp[delta];
					MODADD(temp,temp,temp2);
					lamdax[j] ^= exp2norm[temp];
				}
			}
			if(2*lx<(i+1))
			{
				lx = i+1-lx;
				for(j=0;j<=T;j++)
				{
					if(lastlamdax[j]==0)
					{
						tx[j] = 0;
					}
					else
					{
						temp = norm2exp[lastlamdax[j]];
						temp2 = norm2exp[delta];
						MODSUB(temp,temp,temp2);
						tx[j] = exp2norm[temp];
					}
				}
			}
		}

		// update T(x)
		for(j=T-1;j<T;j--)
		{
			tx[j+1] = tx[j];
		}
		tx[0] = 0;
	}
	/****************************************************
	Step 2. Resolve the lamda equation by Chien Search Algorithm
	****************************************************/
	for(i=0; i<=T; i++)
	{
		if(lamdax[i]!=0)
		{
			exp_vec[i] = norm2exp[lamdax[i]];	// get a exponent format of lamda(x)
		}
	}
	for(i=0; i<datalen; i++)
	{
		temp = 0;
		for(j=0; j<=T; j++)
		{
			if(lamdax[j]!=0)
			{
				temp ^= exp2norm[exp_vec[j]];
				MODSUB(temp2,exp_vec[j],j);
				exp_vec[j] = temp2;
			}
		}
		if(temp==0)
		{
			root[rootcnt] = i;				// here root is the exact position, not position^-1
			rootcnt++;
			if(rootcnt == T)
			{
				break;						// have found enough root
			}
		}
	}
	// root is empty means the lamda funciton can't be resolved, that means received data contains 
	// more errers than decoder can rectify, so just copy the wrong codes and quit
	if(rootcnt == 0)
	{
		for(i=0;i<datalen;i++)
		{
			outputadr[i] = inputadr[i];
		}
		return 0;								//can't found error
	}
	/****************************************************
	Step 3. Get the omega function
		omega(x) = lamda(x)*(1 + syndrome(1)*x^1 + ... syndromes(2t)*x^2t) mod x^(2t+1)
	****************************************************/
	for(i=0; i<=T2; i++)
	{
		if(i>T)
		{
			temp = 0;
		}
		else
		{
			temp = lamdax[i];
		}
		for(j=0;j<i;j++)
		{
			if(j>T)
			{
				break;
			}
			else
			{
				temp2 = lamdax[j];
				temp3 = syndromes[i-j-1];
				if(temp2!=0 && temp3!=0)
				{
					temp2 = norm2exp[temp2];
					temp3 = norm2exp[temp3];
					MODADD(temp2,temp2,temp3);
					temp ^= exp2norm[temp2];
				}
			}
		}
		omegax[i] = temp;
	}
	// transform lamda(x) into lamda'(x)
	// lamda'(x) = lamda(x)(2:end), lamda(2:2:end)=0
	for(i=0;i<T;i+=2)
	{
		lamdax[i] = lamdax[i+1];
		lamdax[i+1] = 0;
	}
	lamdax[T] = 0;
	/****************************************************
	Step 4. Output decoded data and rectify all the errors
	****************************************************/
	for(i=0;i<datalen;i++)
	{
		outputadr[i] = inputadr[i];
	}
	for(i=0;i<rootcnt;i++)
	{
		temp3 = (root[i]==0)?0:(BOUND-root[i]);			//get Xk^-1
		temp = gfpolynomialcalc(omegax,T2+1,temp3);		//omega(x)'s order is 2t
		temp2 = gfpolynomialcalc(lamdax,T,temp3);		//lamda'(x)'s order is t-1
		if(temp!=0 && temp2!=0)
		{
			temp = norm2exp[temp];
			temp2 = norm2exp[temp2];
			MODSUB(temp,temp,temp2);
			MODADD(temp,temp,root[i]);
			temp = exp2norm[temp];
			outputadr[datalen-1-root[i]] ^= temp;
		}
	}
	return 1;
}

/*******************************************************************************
* Function Name  : rsenc()
* Description    : rsenc is used to encoding, the (N-K) encoded redundancy data will be added after the original bytes
					so am at least (N-k+datalen) bytes output buffer must be prepared before calling it
					for example, to perform a (20,10) encoding on 10 bytes data, you have to prepare a 20-bytes buffer for output
					and output adr can be the same to the input adr, so the input data have to be put in the beginning in a long enough buffer
					such as:
					D0 D1 D2 D3 D4 D5 D6 D7 D8 D9 blank blank blank blank blank blank blank blank blank blank
					after calling "rsenc(%D0,%D0,10)"
					the 10 bytes blank will be changed to 10 redundancy data
					WARNING: if you doesn't prepare enough buffer, output will flush beyond the border
					and unexpected result could happen.
					Here I wanna create a highly customizable version
					So the efficiency could not be the highest because of using cycle, pointer or something
					But comparing with the rsdec(), this job is just a piece of cake
* Input          : 
* Output         : None
* Return         : None
*******************************************************************************/
void rsenc(ARRAY_HANDLE inputadr, ARRAY_HANDLE outputadr, u8 datalen)
{
	u8 feedback;
	u8 i,j,temp;
	u8 codereg[T2];
	for(i=0; i<T2; i++)
	{
		codereg[i] = 0;
	}
	for(i=0; i<datalen; i++)
	{
		temp = inputadr[i];
		*outputadr = temp;				// output
		outputadr++;

		feedback = codereg[0]^temp;
		if(feedback==0)
		{
			for(j=0;j<T2-1;j++)
			{
				codereg[j] = codereg[j+1];
			}
			codereg[T2-1] = 0;
		}
		else
		{
			feedback = norm2exp[feedback];
			for(j=0; j<T2-1; j++)
			{
				temp = genepoly[j];			// genepoly is exponent format
				if(temp!=BOUND)
				{
					MODADD(temp,feedback,temp);
					codereg[j] = codereg[j+1]^exp2norm[temp];
				}
				else
				{
					codereg[j] = codereg[j+1];
				}
			}
			//operate the last register
			temp = genepoly[T2-1];
			if(temp!=BOUND)
			{
				MODADD(temp,feedback,temp);
				codereg[T2-1] = exp2norm[temp];
			}
			else
			{
				codereg[T2-1] = 0;
			}
		}
	}
	//output redundant postfix codes
	for(i=0; i<T2; i++)
	{
		*outputadr = codereg[i];
		outputadr++;
	}
}

/*******************************************************************************
* Function Name  : rsencode_vec()
* Description    : 对一组字节数组做RS编码, 编码后格式为10字节原始数据,10字节冗余码元,10字节原始数据...
					原数据末尾不足十字节的数据,不补零直接接10字节冗余码元,因此编码后的字节长度为
					floor(len/10)*20 + (mod(len,10)>0)*(mod(len,10)+10)
					编码后的数据仍存储在原数组空间, 因此要保证内存空间足够, 程序内不检查越界, 由调用程序确保
					由于数据类型为unsigned char, 因此未编码长度字节len < 125;
* Input          : 首字节地址, 数据长度
* Output         : None
* Return         : 编码后字节长度
*******************************************************************************/
u8 rsencode_vec(ARRAY_HANDLE pt, u8 len)
// 
{
	unsigned char pi,pj;
	unsigned char ti,tj,i,tj_bak;
	unsigned char final_len;

	if (len>CFG_RS_ENCODE_LENGTH)
	{
		return 0;
	}
	
	pi = 0;
	pj = 0;
	while((pi+10)<len)
	{
		pi += 10;
		pj += 20;
	}
	
	// encode the last section
	{
		u8 temp[20];					
		
		ti = pi;
		tj = pj ; 
		while(ti<len)
		{
			pt[tj++] = pt[ti++];
		}
		tj_bak = tj;
		while(tj<(pj+10))				//不足的数据后端补零, 按10个字节编码, 为与8051的编码结果一致, 区别于原rsenc的后端数据对齐方式
		{
			pt[tj++] = 0;
		}
		
		rsenc(&pt[pj],temp,10);			// 2013-10-09 原来的做法需要最后至少有20字节容量,容易导致末尾溢出,改成使用额外的内存
//		rsenc(&pt[pj],&pt[pj],10);
		for(i=0;i<=9;i++)
		{
			pt[tj_bak++] = temp[i+10];
		}
		final_len = tj_bak;
	}
	
	// encode prior section
	while(pi!=0)
	{
		ti = pi-10;
		tj = pj-20;
		pi = ti;
		pj = tj;

		for(i=0;i<10;i++)
		{
			pt[tj++] = pt[ti++];
		}
		rsenc(&pt[pj],&pt[pj],10);
	}
	return final_len;
}

/*******************************************************************************
* Function Name  : rsencode_recover()
* Description    : 对RS编码后的向量恢复, 仍存储在原空间, 用于SCAN类型发送时的数据重填
* Input          : 首字节地址, 数据长度
* Output         : None
* Return         : 恢复后数据长度
*******************************************************************************/
unsigned char rsencode_recover(unsigned char xdata * pt, unsigned char len)

{
	unsigned char ti,tj;
	unsigned char i,temp;

	if(len>CFG_RS_DECODE_LENGTH)
	{
		return 0;
	}
		
	ti=0;
	tj=0;
	while(ti<= (len-20))
	{
		for(i=0;i<10;i++)
		{
			pt[tj++] = pt[ti++];
		}
		ti += 10;
	}
	temp = len-ti;		// temp为最后一组数据的长度
	if((temp>0) && (temp<=10))
	{
		return 0;
	}
	else
	{
		if(ti<len)
		{
			//for(i=0;i<=9;i++)				// 2013-10-09 recover无需挪移冗余信元
			//{
			//	pt[len-i-1+20-temp] = pt[len-i-1];
			//}
			//for(i=0;i<=20-temp-1;i++)
			//{
			//	pt[ti+temp-10+i] = 0;
			//}
			for(i=0;i<(temp-10);i++)
			{
				pt[tj++] = pt[ti++];
			}
		}
	}
	return tj;
}

/*******************************************************************************
* Function Name  : rsdecode_vec()
* Description    : 对RS编码后的向量解码, 解码后向量仍存储在原空间
* Input          : 首字节地址, 数据长度
* Output         : None
* Return         : 解码后字节长度
*******************************************************************************/
unsigned char rsdecode_vec(unsigned char * pt, unsigned char len)
{
	unsigned char ti,tj;
	unsigned char i,temp;

	if(len>CFG_RS_DECODE_LENGTH)
	{
		return 0;
	}
	else
	{
		temp = len;				// length check, correct len should be mod(len,20) = 0 or between [11,19]
		while(temp>20)
		{
			temp -= 20;
		}
		if(temp<=10)
		{
			//return len;			// 不符合RS编码长度, 返回原长度(不做修改)
			return 0;			// 2014-09-25 该问题导致无编码的powermesh能被有编码的powermesh接收响应
		}
	}

	/* 解码纠错除最后一组之外的其他RS编码组,并将数据复制到前面 */
	ti=0;
	tj=0;
	if(len>20)
	{
		while(ti<= (len-20))
		{
			rsdec(&pt[ti],&pt[ti],20);
			for(i=0;i<10;i++)
			{
				pt[tj++] = pt[ti++];
			}
			ti += 10;
		}
	}

	if(ti<len)
	{
		u8 last_group_len;
		u8 last_group_buffer[20];
	
		last_group_len = len-ti;									// last_group_len为最后一组数据的长度
		if((last_group_len>0) && (last_group_len<=10))
		{
			return 0;
		}
		else
		{
			mem_cpy(last_group_buffer, &pt[ti], last_group_len);
		
			//if(ti<len)											// 2013-10-09 原做法在处理最后一个分组时需要超出总数据长度的额外空间, 容易导致溢出
			//{
			//	for(i=0;i<=9;i++)
			//	{
			//		pt[len-i-1+20-last_group_len] = pt[len-i-1];
			//	}
			//	for(i=0;i<=20-last_group_len-1;i++)
			//	{
			//		pt[ti+last_group_len-10+i] = 0;
			//	}
			//	rsdec(&pt[ti],&pt[ti],20);
			//	for(i=0;i<(last_group_len-10);i++)
			//	{
			//		pt[tj++] = pt[ti++];
			//	}
			//}

			if(ti<len)
			{
				for(i=0;i<10;i++)						// 复制最后一个group的冗余信元到最后
				{
					last_group_buffer[19-i] = last_group_buffer[last_group_len-1-i];
				}
				for(i=0;i<20-last_group_len;i++)		// 空出的位置清零
				{
					last_group_buffer[last_group_len-10+i] = 0;
				}
				rsdec(last_group_buffer,last_group_buffer,20);	// 解码, 复制到前面
				for(i=0;i<(last_group_len-10);i++)
				{
					pt[tj++] = last_group_buffer[i];
				}
			}			
		}
	}
	return tj;
}

/*******************************************************************************
* Function Name  : rsencoded_len()
* Description    : 对RS编码后的数据长度
* Input          : 编码前长度
* Output         : None
* Return         : 编码后长度
*******************************************************************************/
unsigned char rsencoded_len(unsigned char len)
{
	unsigned char encoded_len = 0;
	while(len>=10)
	{
		len -= 10;
		encoded_len += 20;
	}
	if(len>0)
	{
		encoded_len += (len+10);
	}
	return encoded_len;
}

#endif

///*******************************************************************************
//* Function Name  : calc_crc
//* Description    : Calculate a u8 vector into crc-16 checksum
//* Input          : Head address, length
//* Output         : u16 crc value
//* Return         : u16 crc value
//*******************************************************************************/
#define POLY 			0x1021
u16 calc_crc(ARRAY_HANDLE pt, BASE_LEN_TYPE len) //reentrant
{
	u16 crc;
	u16 i;
	u8 j;
	u8 newbyte,crcbit,databit;

	crc = 0xffff;
	for(i=0;i<len;i++)
	{
		newbyte = *(pt++);
		for (j = 0; j < 8; j++)
		{
			crcbit = (crc & 0x8000) ? 1 : 0;
			databit = (newbyte & 0x80) ? 1 : 0;
			crc = crc << 1;
			if (crcbit != databit)
			{
				crc = crc ^ POLY;
			}
			newbyte = newbyte << 1;
		}
	}
	crc = crc^0xffff;

#ifdef CRC_ISOLATED
	crc ^= CRC_DISTURB;
#endif
	return crc;
}


