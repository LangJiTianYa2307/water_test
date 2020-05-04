/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2019-10-23 09:11:16
 * @LastEditors  : PanHan
 * @LastEditTime : 2020-01-14 13:44:06
 */
#ifndef _MY_MALLOC_H
#define _MY_MALLOC_H
#include "sys.h"

#ifdef __cplusplus
       extern "C" {
#endif

#ifndef NULL
  #define NULL 0
#endif
#define SRAMBANK 	3	    //定义支持的SRAM块数.	

/*mem1内存参数设定.mem1完全处于内部SRAM里面*/
#define MEM1_MAX_SIZE			  200*1024  						            //最大管理内存 100K
/*mem2内存参数设定.mem2的内存池处于外部SDRAM里面*/
#define MEM2_MAX_SIZE			  1  					//最大管理内存2M		 
/*mem3内存参数设定.mem3处于CCM,用于管理DTCM(特别注意,这部分SRAM,仅CPU可以访问!!)*/
#define MEM3_MAX_SIZE			  50*1024  						//最大管理内存50K

typedef enum
{
  MEM_SRAM,
  MEM_DRAM,
  MEM_DTCM
}MemType_t;

void *myMalloc(MemType_t memx,u32 size);
void myFree(MemType_t memx,void *ptr);
u32 getFreeHeapSize(MemType_t memx);
u32 getMinimumEverFreeHeapSize(MemType_t memx);
#ifdef __cplusplus
        }
#endif


#endif
