/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-01-14 11:23:10
 * @LastEditors: PanHan
 * @LastEditTime: 2020-02-26 14:33:42
 * @FilePath: \water_gcc\Src\HardwareProxy\24C02.c
 */
#include "24C02.h"
#include "delay.h"	

/*----------------------------------------#defines--------------------------------------*/

/*--------------------------------------------------------------------------------------*/

/*----------------------------------------static function--------------------------------------*/
static void Eeprom_dev_init(Eeprom_dev * me);
static u8 Eeprom_dev_readData(Eeprom_dev * me, u32 addr, u8 * buff, u32  len);
static u8 Eeprom_dev_writeData(Eeprom_dev * me, u32 addr, const u8 * buff, u32  len);
static u8 Eeprom_dev_checkDevice(Eeprom_dev * me);
static u8 readOneByte(Eeprom_dev * me, u16 ReadAddr);
static void writeOneByte(Eeprom_dev * me, u16 WriteAddr,u8 DataToWrite);
/*--------------------------------------------------------------------------------------*/

/*----------------------------------------static variable--------------------------------------*/
static Eeprom_dev eeprom_dev = {.m_checkByte = 0x5f, .m_address = EEPROM_ADDR};
/*--------------------------------------------------------------------------------------*/
/**
  * @brief	创建EEPROM设备
  * @param	
  * @retval	指向该设备对象的指针
  */
Eeprom_dev * Eeprom_dev_create(void)
{
  static Eeprom_dev * me = 0;
  if (me == 0){
    me->softIIcProxy = SoftIIcProxy_Create(SOFT_IIC_0); //创建IIC对象
    me->m_flashSize = 256;
  }
  return me;
}

/**
  * @brief	EEPROM对象初始化，主要是初始化所用的IIC设备以及向尾部写入校验数据
  * @param	me:pointer to the object
  * @retval	
  */
static void Eeprom_dev_init(Eeprom_dev * me)
{
  if (0 == me->m_initFlag){
    me->softIIcProxy->init(me->softIIcProxy); //初始化IIC
    writeOneByte(me, 255, me->m_checkByte);
  }
  me->m_initFlag = 1;;
}

/**
  * @brief	read data from eepro
  * @param	me:pointer to the object
  * @param  addr:data address
  * @param  buff:read data buffer
  * @param  len:data length
  * @retval	0:OK,other:error
  */
static u8 Eeprom_dev_readData(Eeprom_dev * me, u32 addr, u8 * buff, u32  len)
{
  u16 count;
  if (0 == buff)  return 1;
  for (count = 0; count < len; count ++){
    buff[count] = readOneByte(me, addr);
    addr ++;
    if (addr >= (me->m_flashSize - 1))  return 1; //尾部地址保存校验值，不允许写入
  }
  return 0;
}
static u8 Eeprom_dev_writeData(Eeprom_dev * me, u32 addr, const u8 * buff, u32  len);
static u8 Eeprom_dev_checkDevice(Eeprom_dev * me);

/**
  * @brief	从EEPROM中读取一个字节数据
  * @param	ReadAddr:内存地址
  * @retval	读取的数据
  */
static u8 readOneByte(Eeprom_dev * me, u16 ReadAddr)
{
  u8 temp=0;		  	    																 
  me->softIIcProxy->IIC_START(me->softIIcProxy);  
	me->softIIcProxy->IIC_Send_Byte(me->softIIcProxy, me->m_address << 1);   //发送器件地址,写数据 	   
	me->softIIcProxy->IIC_WAIT_ACK(me->softIIcProxy); 
  me->softIIcProxy->IIC_Send_Byte(me->softIIcProxy, ReadAddr%256);   //发送地址
	me->softIIcProxy->IIC_WAIT_ACK(me->softIIcProxy);	    
	me->softIIcProxy->IIC_START(me->softIIcProxy);  	 	   
	me->softIIcProxy->IIC_Send_Byte(me->softIIcProxy, (me->m_address << 1) | 0x01);  //发送器件地址，读数据		   
	me->softIIcProxy->IIC_WAIT_ACK(me->softIIcProxy);	 
  temp = me->softIIcProxy->IIC_Read_Byte(me->softIIcProxy, 0);		   
  me->softIIcProxy->IIC_STOP(me->softIIcProxy);//产生一个停止条件	    
	return temp;
}

/**
  * @brief	向EEPROM写入一个字节数据
  * @param	WriteAddr:数据地址
  * @param  DataToWrite:需要写入的数据
  * @retval	
  */
static void writeOneByte(Eeprom_dev * me, u16 WriteAddr,u8 DataToWrite)
{
  me->softIIcProxy->IIC_START(me->softIIcProxy);  //开始信号
	me->softIIcProxy->IIC_Send_Byte(me->softIIcProxy, me->m_address << 1);   //发送器件地址,写数据
	me->softIIcProxy->IIC_WAIT_ACK(me->softIIcProxy); //等待应答
  me->softIIcProxy->IIC_Send_Byte(me->softIIcProxy, WriteAddr % 256);   //发送地址
	me->softIIcProxy->IIC_WAIT_ACK(me->softIIcProxy);
  me->softIIcProxy->IIC_Send_Byte(me->softIIcProxy, DataToWrite);   //发送数据
  me->softIIcProxy->IIC_WAIT_ACK(me->softIIcProxy);		
  me->softIIcProxy->IIC_STOP(me->softIIcProxy);     //产生一个停止条件				    
	delay_ms(10);
}