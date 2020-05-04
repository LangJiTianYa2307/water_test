/*
* Copyright (c) 2018, 瑞声研究院
* All right reserved.
*
* 文件名称：delay.c
* 摘    要：延时函数定义，使用FreeRTOS操作系统
*
* 当前版本：1.0
* 原 作 者：潘晗
* 完成日期：2018年11月08日
*/
#include "delay.h"
#include "sys.h"

#if SYSTEM_SUPPORT_OS
#include "FreeRTOS.h"					//FreeRTOS使用	 
#include "task.h"
#endif

static u32 fac_us=0;							//us延时倍乘数

#if SYSTEM_SUPPORT_OS		
    static u16 fac_ms=0;				  //ms延时倍乘数,在os下,代表每个节拍的ms数
#endif

extern void xPortSysTickHandler(void);

/*函数功能：systick中断服务函数，用于freeRTOS计时*/
void SysTick_Handler(void)
{	
    if(xTaskGetSchedulerState()!=taskSCHEDULER_NOT_STARTED)//系统已经运行
    {
        xPortSysTickHandler();	
    }
    HAL_IncTick();
}

/*函数功能：初始化延迟函数，即初始化systick寄存器*/
void delay_init(u8 SYSCLK)
{
	u32 reload;
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);  //SysTick频率为HCLK
	fac_us = SYSCLK;						                          //不论是否使用OS,fac_us都需要使用
	reload = SYSCLK;					                            //每秒钟的计数次数 单位为K	   
	reload *= 1000000 / configTICK_RATE_HZ;		            //根据delay_ostickspersec设定溢出时间
                                                        //reload为24位寄存器,最大值:16777216,在216M下,约合77.7ms左右	
	fac_ms = 1000 / configTICK_RATE_HZ;			              //代表OS可以延时的最少单位	   
	SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;            //开启SYSTICK中断
	SysTick->LOAD = reload; 					                    //每1/OS_TICKS_PER_SEC秒中断一次	
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;             //开启SYSTICK
}								    

/*nus:0~204522252(最大值即2^32/fac_us@fac_us=21)*/
void delay_us(u32 nus)
{		
	u32 ticks;
	u32 told,tnow,tcnt=0;
	u32 reload = SysTick->LOAD;				//LOAD的值	    	 
	ticks = nus * fac_us; 						//需要的节拍数 
	told = SysTick->VAL;        				//刚进入时的计数器值
	while(1)
	{
		tnow = SysTick->VAL;	
		if(tnow != told)
		{	    
			if(tnow<told) tcnt += told - tnow;	//这里注意一下SYSTICK是一个递减的计数器就可以了.
			else tcnt += reload - tnow + told;	    
			told = tnow;
			if(tcnt >= ticks) break;			//时间超过/等于要延迟的时间,则退出.
		}  
	};									    
}  
 
/*nms:0~65535*/
void delay_ms(u32 nms)
{	
	if(xTaskGetSchedulerState()!=taskSCHEDULER_NOT_STARTED)//系统已经运行
	{		
		if(nms>=fac_ms)						//延时的时间大于OS的最少时间周期 
		{ 
   			vTaskDelay(nms/fac_ms);	 		//FreeRTOS延时
		}
		nms%=fac_ms;						//OS已经无法提供这么小的延时了,采用普通方式延时    
	}
	delay_us((u32)(nms*1000));				//普通方式延时
}

void delay_xms(u32 nms)
{
	u32 i;
	for(i=0;i<nms;i++) delay_us(1000);
}


































