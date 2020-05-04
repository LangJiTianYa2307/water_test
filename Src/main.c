/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2019-12-17 08:52:58
 * @LastEditors: PanHan
 * @LastEditTime: 2020-03-13 10:49:42
 * @FilePath: \water_gcc\Src\main.c
 */
#include "APP.h"
#include "delay.h"
#include "usart.h"
static void hardwareInit(void);

int main(void)
{
  hardwareInit(); //硬件初始化
  xTaskCreate((TaskFunction_t )start_task,            
              (const char*    )"start_task",          
              (uint16_t       )START_STK_SIZE,        
              (void*          )NULL,                  
              (UBaseType_t    )START_TASK_PRIO,       
              (TaskHandle_t*  )&StartTask_Handler);          
  vTaskStartScheduler();      
}

static void hardwareInit(void)
{
    Write_Through();                //Cahce强制透写
    MPU_Memory_Protection();        //保护相关存储区域  
    Cache_Enable();                 //打开L1-Cache
    Stm32_Clock_Init(432,25,2,9);   //设置时钟,216Mhz 
    HAL_Init();				              //初始化HAL库
    delay_init(216);                //延时初始化
	  uart_init(115200);
    __HAL_RCC_CRC_CLK_ENABLE();		  //使能CRC时钟,EMWIN需要
}