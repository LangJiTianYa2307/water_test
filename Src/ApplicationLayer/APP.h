/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-01-14 11:22:15
 * @LastEditors: PanHan
 * @LastEditTime: 2020-03-10 20:22:35
 * @FilePath: \water_gcc\Src\ApplicationLayer\APP.h
 */
#ifndef _APP_H
#define _APP_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "rtos_timers.h"
#include "event_groups.h"
#include "CommonType.h"


#ifdef   MY_OS_GLOBALS
#define  MY_OS_EXT
#else
#define  MY_OS_EXT  extern
#endif

//控制是否使用网络端口
#define USE_ETH 0


#ifdef __cplusplus
       extern "C" {
#endif

MY_OS_EXT QueueHandle_t g_modbusQ;                  //用于与modbus业务进行通信的消息
MY_OS_EXT QueueHandle_t g_netQ;                     //用于与网络业务进行通信的消息

//操作系统启动任务
#define START_TASK_PRIO			25	
#define START_STK_SIZE 			256  
MY_OS_EXT TaskHandle_t StartTask_Handler;
void start_task(void *pvParameters);

//通信任务
#define COM_TASK_PRIO 				20
#define COM_STK_SIZE				512
MY_OS_EXT TaskHandle_t comTask_Handler;
void comTask(void *pvParameters);

//统计任务
#define STAT_TASK_PRIO 				2
#define STAT_STK_SIZE				128
MY_OS_EXT TaskHandle_t statTask_Handler;
void statTask(void *pvParameters);

//TCP客户端任务
#define   TCPCLIENT_TASK_PRIO      3
#define   TCPCLIENT_STK_SIZE       256
MY_OS_EXT TaskHandle_t TCPCLIENTTask_Handler;



#ifdef __cplusplus
        }
#endif

#endif
