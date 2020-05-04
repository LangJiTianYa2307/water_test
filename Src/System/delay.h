/*
* Copyright (c) 2018, 瑞声研究院
* All right reserved.
*
* 文件名称：delay.h
* 摘    要：延时函数定义
*
* 当前版本：1.0
* 原 作 者：潘晗
* 完成日期：2018年11月08日
*/
#ifndef _DELAY_H
#define _DELAY_H

#include "sys.h"	

#ifdef __cplusplus
       extern "C" {
#endif

/*
*函数功能：初始化延迟函数
*输入参数：SYSCLK:系统时钟频率,单位MHz
*输出参数：无
*返 回 值：无
*/
void delay_init(u8 SYSCLK);
/*
*函数功能：延时nms,会引起任务调度
*输入参数：nms:延时时间
*输出参数：无
*返 回 值：无
*/
void delay_ms(u32 nms);
/*
*函数功能：延时nus,不会引起任务调度
*输入参数：nus:延时时间
*输出参数：无
*返 回 值：无
*/         
void delay_us(u32 nus);
/*
*函数功能：延时nms,不会引起任务调度
*输入参数：nms:延时时间
*输出参数：无
*返 回 值：无
*/
void delay_xms(u32 nms);
         
#ifdef __cplusplus
        }
#endif
#endif

