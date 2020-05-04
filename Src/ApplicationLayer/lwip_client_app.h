/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2019-11-27 08:54:56
 * @LastEditors  : PanHan
 * @LastEditTime : 2020-01-14 13:53:50
 */
#ifndef LWIP_CLIENT_APP_H
#define LWIP_CLIENT_APP_H
#include "sys.h"   
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"
 
#define TCP_CLIENT_RX_BUFSIZE	1500	//接收缓冲区长度
#define REMOTE_PORT				8086	//定义远端主机的IP地址
#define LWIP_SEND_DATA			0X80    //定义有数据发送

#ifdef __cplusplus
       extern "C" {
#endif

extern QueueHandle_t tcpClientQ;

extern u8 tcp_client_recvbuf[TCP_CLIENT_RX_BUFSIZE];	//TCP客户端接收数据缓冲区
extern u8 tcp_client_flag;		    //TCP客户端数据发送标志位

uint8_t tcp_client_init(void);  //tcp客户端初始化(创建tcp客户端线程)
  
  
#ifdef __cplusplus
        }
#endif
        
#endif

