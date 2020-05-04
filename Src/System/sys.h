/*
* Copyright (c) 2018, 瑞声研究院
* All right reserved.
*
* 文件名称：sys.h
* 摘    要：系统所需的类型定义及中断定义
*
* 当前版本：1.0
* 原 作 者：潘晗
* 完成日期：2018年11月08日
*/
#ifndef _SYS_H
#define _SYS_H



#include "stm32f7xx.h"
#include "core_cm7.h"
#include "stm32f7xx_hal.h"
#include "TypeDef.h"
         
#define SYSTEM_SUPPORT_OS		1		//定义系统文件夹是否支持OS	
#define ASSERT_ENABLE 1         //定义系统是否支持断言

#if ASSERT_ENABLE
  //断言
  #define MY_AssertCalled(char,int) printf("Error:%s,%d\r\n",char,int)
  #define MY_ASSERT(x) if((x)==0) MY_AssertCalled(__FILE__,__LINE__)
#else
  #define MY_AssertCalled(char,int)
  #define MY_ASSERT(x)
#endif

/***********************************************************************
*                               类型定义                               *
************************************************************************/
//typedef int32_t  s32;
//typedef int16_t s16;
//typedef int8_t  s8;

typedef const int32_t sc32;  
typedef const int16_t sc16;  
typedef const int8_t sc8;  

typedef __IO int32_t  vs32;
typedef __IO int16_t  vs16;
typedef __IO int8_t   vs8;

typedef __I int32_t vsc32;  
typedef __I int16_t vsc16; 
typedef __I int8_t vsc8;   

//typedef uint32_t  u32;
//typedef uint16_t u16;
//typedef uint8_t  u8;

typedef const uint32_t uc32;  
typedef const uint16_t uc16;  
typedef const uint8_t uc8; 

typedef __IO uint32_t  vu32;
typedef __IO uint16_t vu16;
typedef __IO uint8_t  vu8;

typedef __I uint32_t vuc32;  
typedef __I uint16_t vuc16; 
typedef __I uint8_t vuc8;  

#define ON	1
#define OFF	0
#define Write_Through() (*(__IO uint32_t*)0XE000EF9C=1UL<<2) //Cache透写模式 和SCB->CACR|=1<<2;作用一致

/*
*函数功能：使能CPU的L1-Cache
*输入参数：无
*输出参数：无
*返 回 值：无
*/
void Cache_Enable(void);
/*
*函数功能：配置系统时钟
*输入参数：plln:主PLL倍频系数(PLL倍频),取值范围:64~432.
           pllm:主PLL和音频PLL分频系数(PLL之前的分频),取值范围:2~63
           pllp:系统时钟的主PLL分频系数(PLL之后的分频),取值范围:2,4,6,8.(仅限这4个值!)
           pllq:USB/SDIO/随机数产生器等的主PLL分频系数(PLL之后的分频),取值范围:2~15.
*输出参数：无
*返 回 值：无
*/
void Stm32_Clock_Init(u32 plln,u32 pllm,u32 pllp,u32 pllq);
/*
*函数功能：判断I_Cache是否打开
*输入参数：无
*输出参数：无
*返 回 值：0 关闭，1 打开
*/
u8 Get_ICahceSta(void);
/*
*函数功能：判断D_Cache是否打开
*输入参数：无
*输出参数：无
*返 回 值：0 关闭，1 打开
*/
u8 Get_DCahceSta(void);

/*********************************下面为内嵌汇编函数**************************************/
/*
*函数功能：关闭所有中断(不包括fault和NMI)
*输入参数：无
*输出参数：无
*返 回 值：无
*/
void INTX_DISABLE(void);
/*
*函数功能：开启所有中断
*输入参数：无
*输出参数：无
*返 回 值：无
*/
void INTX_ENABLE(void);	




#endif

