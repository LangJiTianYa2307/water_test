/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-01-14 11:23:10
 * @LastEditors: PanHan
 * @LastEditTime: 2020-02-27 22:07:11
 * @FilePath: \water_gcc\Src\HardwareProxy\24C02.h
 */
#ifndef _24C02_H
#define _24C02_H
#include "sys.h"
#include "softIIcProxy.h"			  

#define EEPROM_ADDR 0x50

typedef struct _Eeprom_dev  Eeprom_dev;
struct _Eeprom_dev
{
  void (* init)(Eeprom_dev * me);
  u8 (* readData)(Eeprom_dev * me, u32 addr, u8 * buff, u32  len);//从指定地址读取数据
  u8 (* writeData)(Eeprom_dev * me, u32 addr, const u8 * buff, u32  len);//向指定起始地址写入数据
  u8 (* checkDevice)(Eeprom_dev * me);
  SoftIIcProxy * softIIcProxy;  //包含IIC
  u32 m_flashSize;
  u8 m_initFlag;
  const u8 m_address;   //从机地址
  const u8 m_checkByte;
};

Eeprom_dev * Eeprom_dev_create(void);
#endif
