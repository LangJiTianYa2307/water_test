/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-01-14 11:23:10
 * @LastEditors: PanHan
 * @LastEditTime: 2020-01-14 13:57:00
 * @FilePath: \water_keil\Src\HardwareProxy\usart.h
 */
#ifndef _USART_H
#define _USART_H
#include "sys.h"
#include "stdio.h"	

#ifdef __cplusplus
       extern "C" {
#endif

#define USART_REC_LEN  			200  	//定义最大接收字节数 200
#define EN_USART1_RX 			  0		//使能（1）/禁止（0）串口1接收
	  	
extern u8  USART_RX_BUF[USART_REC_LEN];  //接收缓冲,最大USART_REC_LEN个字节.末字节为换行符 
extern u16 USART_RX_STA;         		     //接收状态标记	
extern UART_HandleTypeDef UART1_Handler; //UART句柄

#define RXBUFFERSIZE   1 //缓存大小
extern u8 aRxBuffer[RXBUFFERSIZE];//HAL库USART接收Buffer
/*
*函数功能：串口初始化
*输入参数：bound:串口波特率
*输出参数：无
*返 回 值：无
*/
void uart_init(u32 bound);
/*
*函数功能：通过串口发送数据
*输入参数：buff:需要发送的数据，len：发送的数据长度
*输出参数：无
*返 回 值：无
*/
void uartSendMes(u8 * buff, u8 len);

#ifdef __cplusplus
        }
#endif
        
#endif
