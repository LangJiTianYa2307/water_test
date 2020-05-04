/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-01-14 11:23:10
 * @LastEditors  : PanHan
 * @LastEditTime : 2020-01-14 13:58:45
 * @FilePath: \water_keil\Src\HardwareProxy\my_spi.h
 */
#ifndef _MY_SPI_H
#define _MY_SPI_H

#include "sys.h"
/*
*函数功能：初始化SPI1口
*输入参数：无
*输出参数：无
*返 回 值：无
*/
void SPI1_Init(void);
/*
*函数功能：设置SPI1的速率
*输入参数：SpeedSet：分频系数，可选范围SPI_BAUDRATEPRESCALER_2~SPI_BAUDRATEPRESCALER_256
*输出参数：无
*返 回 值：无
*/
void SPI1_SetSpeed(u8 SpeedSet); //设置SPI1速度   
/*
*函数功能：SPI1总线读写一个字节
*输入参数：TxData：向从机写入的数据
*输出参数：无
*返 回 值：从移位寄存器中读取的数据
*/
u8 SPI1_ReadWriteByte(u8 TxData);//SPI1总线读写一个字节
/*
*函数功能：初始化SPI3
*输入参数：无
*输出参数：无
*返 回 值：无
*/
void SPI3_Init(void);
/*
*函数功能：初始化SPI2
*输入参数：无
*输出参数：无
*返 回 值：无
*/
void SPI2_Init(void);
/*
*函数功能：SPI1总线读写一个16位数据
*输入参数：TxData：向从机写入的数据
*输出参数：无
*返 回 值：从移位寄存器中读取的数据
*/
u16 SPI2_ReadWriteByte(u16 TxData);


u8 SPI2_ReadByte(void);

void SPI2_WriteByte(u8 data);
u8 SPI2_readWriteByte(u8 TxData);
#endif
