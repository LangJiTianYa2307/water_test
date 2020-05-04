/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-01-14 11:23:10
 * @LastEditors: PanHan
 * @LastEditTime: 2020-03-11 11:37:12
 * @FilePath: \water_gcc\Src\HardwareProxy\usart.c
 */
#include "usart.h"
#include "delay.h"
#include "my_malloc.h"
#include "string.h"
#include "FreeRTOS.h"     
#include "task.h"
#include "semphr.h"
#include "stdio.h"

#include "APP.h"

//串口1中断服务程序
//注意,读取USARTx->SR能避免莫名其妙的错误   	
u8 USART_RX_BUF[USART_REC_LEN];     //接收缓冲,最大USART_REC_LEN个字节.
//接收状态
//bit15，	接收完成标志
//bit14，	接收到0x0d
//bit13~0，	接收到的有效字节数目
u16 USART_RX_STA=0;       //接收状态标记	

u8 aRxBuffer[1];//HAL库使用的串口接收缓冲
UART_HandleTypeDef UART1_Handler; //UART句柄

/**
  * @brief	初始化usart
  * @param	bound:波特率
  * @retval	
  */
void uart_init(u32 bound)
{	
	//UART 初始化设置
	UART1_Handler.Instance = USART1;					    //USART1
	UART1_Handler.Init.BaudRate = bound;				    //波特率
	UART1_Handler.Init.WordLength = UART_WORDLENGTH_8B;   //字长为8位数据格式
	UART1_Handler.Init.StopBits = UART_STOPBITS_1;	    //一个停止位
	UART1_Handler.Init.Parity = UART_PARITY_NONE;		    //无奇偶校验位
	UART1_Handler.Init.HwFlowCtl = UART_HWCONTROL_NONE;   //无硬件流控
	UART1_Handler.Init.Mode = UART_MODE_TX_RX;		    //收发模式
	HAL_UART_Init(&UART1_Handler);					    //HAL_UART_Init()会使能UART1
	
	HAL_UART_Receive_IT(&UART1_Handler, (u8 *)aRxBuffer, 1);//该函数会开启接收中断：标志位UART_IT_RXNE，并且设置接收缓冲以及接收缓冲接收最大数据量
  
}

//UART底层初始化，时钟使能，引脚配置，中断配置
//此函数会被HAL_UART_Init()调用
//huart:串口句柄

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    //GPIO端口设置
	GPIO_InitTypeDef GPIO_Initure;
	
	if(huart->Instance==USART1)//如果是串口1，进行串口1 MSP初始化
	{
		__HAL_RCC_GPIOA_CLK_ENABLE();			//使能GPIOA时钟
		__HAL_RCC_USART1_CLK_ENABLE();			//使能USART1时钟
	
		GPIO_Initure.Pin=GPIO_PIN_9;			//PA9
		GPIO_Initure.Mode=GPIO_MODE_AF_PP;		//复用推挽输出
		GPIO_Initure.Pull=GPIO_PULLUP;			//上拉
		GPIO_Initure.Speed=GPIO_SPEED_FAST;		//高速
		GPIO_Initure.Alternate=GPIO_AF7_USART1;	//复用为USART1
		HAL_GPIO_Init(GPIOA,&GPIO_Initure);	   	//初始化PA9

		GPIO_Initure.Pin=GPIO_PIN_10;			//PA10
		HAL_GPIO_Init(GPIOA,&GPIO_Initure);	   	//初始化PA10
		
#if EN_USART1_RX
		HAL_NVIC_EnableIRQ(USART1_IRQn);				//使能USART1中断通道
		HAL_NVIC_SetPriority(USART1_IRQn, 6, 0);			//抢占优先级7，子优先级0
#endif	
	}
	if(huart->Instance==USART2)//如果是串口2，进行串口2 MSP初始化
	{
		__HAL_RCC_GPIOA_CLK_ENABLE();			//使能GPIOA时钟
		__HAL_RCC_USART2_CLK_ENABLE();			//使能USART1时钟
	
		GPIO_Initure.Pin = GPIO_PIN_2 | GPIO_PIN_3;			//PA2
		GPIO_Initure.Mode = GPIO_MODE_AF_PP;		//复用推挽输出
		GPIO_Initure.Pull = GPIO_PULLUP;			//上拉
		GPIO_Initure.Speed = GPIO_SPEED_FAST;		//高速
		GPIO_Initure.Alternate = GPIO_AF7_USART2;	//复用为USART1
		HAL_GPIO_Init(GPIOA, &GPIO_Initure);	   	//初始化PA2,3

	}
}

/*串口接收回调函数，函数以0s0d结束
*/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	AppMesage appMsg;
	BaseType_t xHighPri = pdFALSE;
	if(huart->Instance==USART1)//如果是串口1
	{
		if((USART_RX_STA&0x8000) == 0)//接收未完成
		{
			if(USART_RX_STA & 0x4000)//接收到了0x0d
			{
				if(aRxBuffer[0]!=0x0a) USART_RX_STA = 0;//接收错误,重新开始
				else
				{
					USART_RX_STA |= 0x8000;	//接收完成了 
					USART_RX_BUF[USART_RX_STA & 0X3FFF] = '\0';
					appMsg.dataType = 5;
					appMsg.pVoid = USART_RX_BUF;
					//xQueueSendFromISR(g_comQ, &appMsg, &xHighPri);
					printf("over\r\n");
				}					
			}else //还没收到0X0D
			{	
				if(aRxBuffer[0]==0x0d) USART_RX_STA |= 0x4000;
				else
				{
					USART_RX_BUF[USART_RX_STA&0X3FFF] = aRxBuffer[0] ;
					USART_RX_STA ++;
					if(USART_RX_STA>(USART_REC_LEN-1)) USART_RX_STA = 0;//接收数据错误,重新开始接收	  
				}
			}
		}

	}
}
 
//串口1中断服务程序
void USART1_IRQHandler(void)                	
{ 
	u32 timeout=0;
  u32 maxDelay=0x1FFFF;
	HAL_UART_IRQHandler(&UART1_Handler);	//调用HAL库中断处理公用函数
	
	timeout=0;
  while (HAL_UART_GetState(&UART1_Handler)!=HAL_UART_STATE_READY)//等待就绪
	{
		timeout++;////超时处理
		if(timeout>maxDelay) break;		
	}
     
	timeout=0;
	while(HAL_UART_Receive_IT(&UART1_Handler,(u8 *)aRxBuffer, 1)!=HAL_OK)//一次处理完成之后，重新开启中断并设置RxXferCount为1
	{
		timeout++; //超时处理
		if(timeout>maxDelay) break;	
	}
} 

/**
  * @brief	seng message by usart
  * @param	buff:data buffer
  * @param  len:buffer length
  * @retval	
  */
void uartSendMes(u8 * buff, u8 len)
{
  HAL_UART_Transmit(&UART1_Handler,buff,len,1000);	//发送接收到的数据
  while(__HAL_UART_GET_FLAG(&UART1_Handler,UART_FLAG_TC)!=SET);		//等待发送结束
}
 

 




