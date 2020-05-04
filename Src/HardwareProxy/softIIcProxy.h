/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-01-06 11:10:32
 * @LastEditors: PanHan
 * @LastEditTime: 2020-02-26 13:56:01
 * @FilePath: \water_gcc\Src\HardwareProxy\softIIcProxy.h
 */
#ifndef _SOFT_IIC_H
#define _SOFT_IIC_H

#include "sys.h"

#ifdef __cplusplus
       extern "C" {
#endif
//IIC类型
typedef enum
{
  SOFT_IIC_0,
  SOFT_IIC_MAX
}SoftIICType;

//IIC消息类型
typedef struct _I2cMsg_t
{
  u8 * buff;  //消息
  u8 buffLen; //消息长度
  u8 address; //地址
}I2cMsg_t;


typedef struct _SoftIIcProxy SoftIIcProxy;
struct _SoftIIcProxy
{
  void (* init)(SoftIIcProxy * me);
  u8 (* sendMsg)(SoftIIcProxy * me, const I2cMsg_t * msg);  //给从机发送消息
  u8 (* getMsg)(SoftIIcProxy * me, I2cMsg_t * msg);         //接收从机消息
  
  void (* IIC_START)(SoftIIcProxy * me);
  void (* IIC_STOP)(SoftIIcProxy * me);
  u8 (* IIC_WAIT_ACK)(SoftIIcProxy * me);
  void (* IIC_ACK)(SoftIIcProxy * me);
  void (* IIC_NACK)(SoftIIcProxy * me);
  void (* IIC_Send_Byte)(SoftIIcProxy * me, u8 txd);
  u8 (* IIC_Read_Byte)(SoftIIcProxy * me, u8 ack);
  SoftIICType m_iicType;
  GPIO_TypeDef * m_gpioPort[2];
  u16 m_gpioPin[2];
  u8 m_initFlag;
};
SoftIIcProxy * SoftIIcProxy_Create(SoftIICType iicType);

#ifdef __cplusplus
        }
#endif

#endif