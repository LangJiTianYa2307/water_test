/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-01-14 13:53:33
 * @LastEditors  : PanHan
 * @LastEditTime : 2020-01-14 14:05:24
 * @FilePath: \water_keil\Src\ApplicationLayer\lwip_client_app.c
 */
#include "lwip_client_app.h"
#include "lwip/opt.h"
#include "lwip_comm.h"
#include <string.h>
#include "delay.h"
#include "lwip/lwip_sys.h"
#include "lwip/api.h" 
#include "APP.h"

#include "BLL_Modbus.h"
#include "mb.h"
#define TCP_SERVER_RX_BUFSIZE	1000		//定义tcp server最大接收数据长度
#define TCP_SERVER_PORT			8088	      //定义tcp server的端口，用于MODBUS通信的端口默认为502
#define LWIP_SEND_DATA			0X80	//定义有数据发送

u8 tcp_server_recvbuf[1000];	//TCP客户端接收数据缓冲区

u8 modbusTcpRecvBuff[1000];  //modbus接收到的数据

struct netconn *tcp_clientconn = NULL;					//TCP CLIENT网络连接结构体
u8 tcp_client_recvbuf[TCP_CLIENT_RX_BUFSIZE];	//TCP客户端接收数据缓冲区
u8 tcp_client_flag;		//TCP客户端数据发送标志位

//tcp客户端任务函数
void tcp_client_thread(void *arg)
{
  BaseType_t res;         //freeRTOS基本类型
	AppMesage appMsg;       //自定义消息类型
	ModbusDev * pModbus = modbusCreate();//创建MODBUS对象
	u32 data_len = 0;       //数据长度
	struct pbuf *q;
	err_t err,recv_err;
	u8 remot_addr[4];
	struct netconn *conn, *newconn;
	static ip_addr_t ipaddr;
	static u16_t 			port;
	conn = netconn_new(NETCONN_TCP);  //创建一个TCP链接
	netconn_bind(conn, IP_ADDR_ANY, TCP_SERVER_PORT);  //绑定端口 8088号端口
	netconn_listen(conn);  		//进入监听模式
	conn->recv_timeout = 10;  	//禁止阻塞线程 等待10ms
	
	while (1) 
	{
		err = netconn_accept(conn,&newconn);  //接收连接请求
		if(err==ERR_OK)newconn->recv_timeout = 10;

		if (err == ERR_OK)    //处理新连接的数据
		{ 
			struct netbuf *recvbuf;
			netconn_getaddr(newconn,&ipaddr,&port,0); //获取远端IP地址和端口号
			remot_addr[3] = (uint8_t)(ipaddr.addr >> 24); 
			remot_addr[2] = (uint8_t)(ipaddr.addr>> 16);
			remot_addr[1] = (uint8_t)(ipaddr.addr >> 8);
			remot_addr[0] = (uint8_t)(ipaddr.addr);
			printf("client%d.%d.%d.%d connect server,client port is :%d\r\n",remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3],port);
			
			while(1)
			{			
				/*通过网络发送数据*/
				res = xQueueReceive(g_netQ, &appMsg, 0);//获取消息队列中的数据
        if (res == pdTRUE)
        {
					err = netconn_write(newconn, (u8 *)appMsg.pVoid, appMsg.dataType, NETCONN_COPY); //发送tcp_server_sendbuf中的数据
          if(err != ERR_OK)
					{
						printf("send failed\r\n");
					}
        }
				/*接收到数据*/
				if((recv_err = netconn_recv(newconn, &recvbuf)) == ERR_OK) 
				{		
					taskENTER_CRITICAL(); //关中断
					memset(tcp_server_recvbuf, 0, TCP_SERVER_RX_BUFSIZE);  //数据接收缓冲区清零
					for(q = recvbuf->p; q != NULL; q = q->next)  //遍历完整个pbuf链表
					{
						//判断要拷贝到TCP_SERVER_RX_BUFSIZE中的数据是否大于TCP_SERVER_RX_BUFSIZE的剩余空间，如果大于
						//的话就只拷贝TCP_SERVER_RX_BUFSIZE中剩余长度的数据，否则的话就拷贝所有的数据
						if(q->len > (TCP_SERVER_RX_BUFSIZE-data_len)) memcpy(tcp_server_recvbuf+data_len,q->payload,(TCP_SERVER_RX_BUFSIZE-data_len));//拷贝数据
						else memcpy(tcp_server_recvbuf+data_len,q->payload,q->len);
						data_len += q->len;  	
						if(data_len > TCP_SERVER_RX_BUFSIZE) break; //超出TCP客户端接收数组,跳出	
					}
					taskEXIT_CRITICAL();  //开中断
					data_len = 0;  //复制完成后data_len要清零。	

					pModbus->m_pRecvBuff = tcp_server_recvbuf;
					eMBTcpPoll();
					
					netbuf_delete(recvbuf);
				}else if(recv_err == ERR_CLSD)  //关闭连接
				{
					netconn_close(newconn);
					netconn_delete(newconn);
					printf("client :%d.%d.%d.%d disconnect server\r\n",remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
					break;
				}
			}
		}
	}	
}

//创建TCP客户端线程
//返回值:0 TCP客户端创建成功
//		其他 TCP客户端创建失败
uint8_t tcp_client_init(void)
{
	BaseType_t res;
	
	taskENTER_CRITICAL();              
	res = xTaskCreate((TaskFunction_t)tcp_client_thread,
					(const char*  )"tcp_client_task",
					(uint16_t     )TCPCLIENT_STK_SIZE,
					(void*        )NULL,
					(UBaseType_t  )TCPCLIENT_TASK_PRIO,
					(TaskHandle_t*)&TCPCLIENTTask_Handler);
	taskEXIT_CRITICAL();
	
	if(res == pdPASS) return 0;
	return 1;
}

