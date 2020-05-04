/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-01-14 11:23:10
 * @LastEditors  : PanHan
 * @LastEditTime : 2020-01-14 13:58:31
 * @FilePath: \water_keil\Src\HardwareProxy\my_timer.h
 */
#ifndef _MY_TIMER_H
#define _MY_TIMER_H

#include "stm32f7xx.h"
#include "sys.h"
extern TIM_HandleTypeDef TIM6_Handler;      //定时器句柄 
/*
*函数功能：定时器初始化
*输入参数：arr:自动重装值;psc:时钟预分频数
*输出参数：无
*返 回 值：0,初始化成功;1,LSE开启失败;2,进入初始化模式失败;
*/
void TIM3_Init(u16 arr,u16 psc);
void TIM3_PWM_Init(u16 arr,u16 psc);
void TIM_SetTIM3Compare4(u32 compare);

void TIM6_Init(u16 arr,u16 psc);
#endif
