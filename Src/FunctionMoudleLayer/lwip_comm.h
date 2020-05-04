/*
 * @Descripttion: 
 * @version: 
 * @Author: panhan
 * @Date: 2019-12-24 13:35:27
 * @LastEditors  : PanHan
 * @LastEditTime : 2020-01-14 13:47:46
 * @FilePath: \gas-gcc\Src\FunctionMoudleLayer\lwip_comm.h
 */
#ifndef _LWIP_COMM_H
#define _LWIP_COMM_H 
#include "lan8720.h" 
#include "sys.h"   
 
#define LWIP_MAX_DHCP_TRIES		4   //DHCP????????????????

#ifdef __cplusplus
extern "C" {
#endif
  
//lwip???????
typedef struct  
{
	u8 mac[6];      //MAC???
	u8 remoteip[4];	//???????IP??? 
	u8 ip[4];       //????IP???
	u8 netmask[4]; 	//????????
	u8 gateway[4]; 	//????????IP???
	
	vu8 dhcpstatus;	//dhcp?? 
					//0,δ???DHCP???;
					//1,????DHCP?????
					//2,??????DHCP???
					//0XFF,??????.
}__lwip_dev;
extern __lwip_dev lwipdev;	//lwip???????

void lwip_pkt_handle(void);
void lwip_comm_default_ip_set(__lwip_dev *lwipx);
u8 lwip_comm_mem_malloc(void);
void lwip_comm_mem_free(void);
u8 lwip_comm_init(void);
void lwip_comm_dhcp_creat(void);
void lwip_comm_dhcp_delete(void);
void lwip_comm_destroy(void);
void lwip_comm_delete_next_timeout(void);


#ifdef __cplusplus
}
#endif

#endif













