/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-01-06 11:08:34
 * @LastEditors: PanHan
 * @LastEditTime: 2020-02-26 14:03:15
 * @FilePath: \water_gcc\Src\HardwareProxy\softIIcProxy.c
 */
#include "SoftIIcProxy.h"
#include "my_malloc.h"
/*----------------------------------------#defines--------------------------------------*/
#define IIC_SDA 0
#define IIC_SCL 1
const GPIO_TypeDef * SoftIICPortArr[SOFT_IIC_MAX][2] = {
  {GPIOH, GPIOH}
};
const u16 SoftIICPinArr[SOFT_IIC_MAX][2] = {
  {GPIO_PIN_4, GPIO_PIN_5}
};
/*--------------------------------------------------------------------------------------*/

/*----------------------------------------static function--------------------------------------*/
static void SoftIIcProxy_init(SoftIIcProxy * me);
static u8 SoftIIcProxy_sendMsg(SoftIIcProxy * me, const I2cMsg_t * msg);
static u8 SoftIIcProxy_getMsg(SoftIIcProxy * me, I2cMsg_t * msg);
static void SDA_IN(SoftIIcProxy * me) ;
static void SDA_OUT(SoftIIcProxy * me);
static void SDA_HIGH(SoftIIcProxy * me);
static void SDA_LOW(SoftIIcProxy * me);
static void SCL_HIGH(SoftIIcProxy * me);
static void SCL_LOW(SoftIIcProxy * me);
static u8 READ_SDA(SoftIIcProxy * me);

static void IIC_START(SoftIIcProxy * me);
static void IIC_STOP(SoftIIcProxy * me);
static u8 IIC_WAIT_ACK(SoftIIcProxy * me);
static void IIC_ACK(SoftIIcProxy * me);
static void IIC_NACK(SoftIIcProxy * me);
static void IIC_Send_Byte(SoftIIcProxy * me, u8 txd);
static u8 IIC_Read_Byte(SoftIIcProxy * me, u8 ack);
/*--------------------------------------------------------------------------------------*/

/*----------------------------------------static variable--------------------------------------*/
static SoftIIcProxy softIIcProxyArr[SOFT_IIC_MAX];
static u32 s_iicEnableCount[SOFT_IIC_MAX]; //IIC使能计数，为0表示未使能
/*--------------------------------------------------------------------------------------*/
/**
  * @brief   create softiic object
  * @param   iicType: iic type
  * @retval  
  */
SoftIIcProxy * SoftIIcProxy_Create(SoftIICType iicType)
{
  SoftIIcProxy * me = &softIIcProxyArr[iicType];
  if (0 == me->m_initFlag){
    me->init = SoftIIcProxy_init;
    me->sendMsg = SoftIIcProxy_sendMsg;
    me->getMsg = SoftIIcProxy_getMsg;
    me->IIC_ACK = IIC_ACK;
    me->IIC_NACK = IIC_NACK;
    me->IIC_Send_Byte = IIC_Send_Byte;
    me->IIC_START = IIC_START;
    me->IIC_STOP = IIC_STOP;
    me->IIC_Read_Byte = IIC_Read_Byte;

    me->m_gpioPort[0] = SoftIICPortArr[iicType][0];
    me->m_gpioPort[1] = SoftIICPortArr[iicType][1];

    me->m_gpioPin[0] = SoftIICPinArr[iicType][0];
    me->m_gpioPin[1] = SoftIICPinArr[iicType][1];

    me->m_iicType = iicType;
    me->m_initFlag = 1;
  }
  return me;
}

/**
  * @brief	transmit message by soft iic
  * @param	me: pointer to the object
  * @param  msg: message need to be transmitted
  * @retval	0:OK,other:error
  */
static u8 SoftIIcProxy_sendMsg(SoftIIcProxy * me, const I2cMsg_t * msg)
{
  u32 i = 0;
  IIC_START(me);
  IIC_Send_Byte(me, msg->address<<1);	  //write
	IIC_WAIT_ACK(me);	    										  		         
  for (i = 0; i < msg->buffLen; i ++){
    IIC_Send_Byte(me, msg->buff[i]);    	 	//send one byte		   
	  IIC_WAIT_ACK(me);
  }
  IIC_STOP(me);
  return 0;	
}

/**
  * @brief	receive message
  * @param	msg: receive message
  * @retval	0:OK,other:error
  */
static u8 SoftIIcProxy_getMsg(SoftIIcProxy * me, I2cMsg_t * msg)
{
  u32 i = 0;
  IIC_START(me);
  IIC_Send_Byte(me, (msg->address<<1) | 0x01);  //read
  IIC_WAIT_ACK(me);
  for (i = 0; i < msg->buffLen; i ++){
    msg->buff[i] = IIC_Read_Byte(me, 1);  //send ack
  }
  IIC_STOP(me);
  return 0;
}

/**
  * @brief   soft iic gpio initialization
  * @param   
  * @retval  
  */
static void SoftIIcProxy_init(SoftIIcProxy * me)
{
  GPIO_InitTypeDef GPIO_Initure;
  __HAL_RCC_GPIOH_CLK_ENABLE();   //enable gpio clock
  
  /*如果IIC未初始化则进行初始化，否则初始化次数加一*/
  if (0 == s_iicEnableCount[me->m_iicType]){
    GPIO_Initure.Mode = GPIO_MODE_OUTPUT_PP;  
    GPIO_Initure.Pull = GPIO_PULLUP;          
    GPIO_Initure.Speed = GPIO_SPEED_FAST;

    GPIO_Initure.Pin = me->m_gpioPin[IIC_SCL];     
    HAL_GPIO_Init(me->m_gpioPort[IIC_SCL], &GPIO_Initure);

    GPIO_Initure.Pin = me->m_gpioPin[IIC_SDA];     
    HAL_GPIO_Init(me->m_gpioPort[IIC_SDA], &GPIO_Initure);
  }else{
    s_iicEnableCount[me->m_iicType] ++;
  }
  
  SDA_HIGH(me);
  SCL_HIGH(me);
}


/**
  * @brief   SDA����Ϊ����
  * @param   
  * @retval  
  */
inline static void SDA_IN(SoftIIcProxy * me)
{
  GPIO_InitTypeDef GPIO_Initure;
  GPIO_Initure.Pin = me->m_gpioPin[IIC_SDA];
  GPIO_Initure.Mode = GPIO_MODE_INPUT;      //input
  GPIO_Initure.Pull = GPIO_NOPULL;          //no pull
  GPIO_Initure.Speed = GPIO_SPEED_HIGH;     //high speed
  HAL_GPIO_Init(me->m_gpioPort[IIC_SDA], &GPIO_Initure);      //initialization
}

/**
  * @brief   ����SDA���
  * @param   
  * @retval  
  */
inline static void SDA_OUT(SoftIIcProxy * me)
{
  GPIO_InitTypeDef GPIO_Initure;
  GPIO_Initure.Pin = me->m_gpioPin[IIC_SDA];
  GPIO_Initure.Mode = GPIO_MODE_OUTPUT_PP;      //output
  GPIO_Initure.Pull = GPIO_PULLUP;          //pull up
  GPIO_Initure.Speed = GPIO_SPEED_HIGH;     //high speed
  HAL_GPIO_Init(me->m_gpioPort[IIC_SDA], &GPIO_Initure);      //initialization
}

/**
  * @brief   ����������
  * @param   
  * @retval  
  */
inline static void SDA_HIGH(SoftIIcProxy * me)
{
  HAL_GPIO_WritePin(me->m_gpioPort[IIC_SDA], me->m_gpioPin[IIC_SDA], SET);
}

/**
  * @brief   ����������
  * @param   
  * @retval  
  */
inline static void SDA_LOW(SoftIIcProxy * me)
{
  HAL_GPIO_WritePin(me->m_gpioPort[IIC_SDA], me->m_gpioPin[IIC_SDA], RESET);
}
/**
  * @brief   ����ʱ����
  * @param   
  * @retval  
  */
inline static void SCL_HIGH(SoftIIcProxy * me)
{
  HAL_GPIO_WritePin(me->m_gpioPort[IIC_SCL], me->m_gpioPin[IIC_SCL], SET);
}
/**
  * @brief   ����ʱ����
  * @param   
  * @retval  
  */
inline static void SCL_LOW(SoftIIcProxy * me)
{
  HAL_GPIO_WritePin(me->m_gpioPort[IIC_SCL], me->m_gpioPin[IIC_SCL], RESET);
}

inline static u8 READ_SDA(SoftIIcProxy * me)
{
  return HAL_GPIO_ReadPin(me->m_gpioPort[IIC_SDA], me->m_gpioPin[IIC_SDA]);
}

static void IIC_START(SoftIIcProxy * me)
{
  SDA_OUT(me);     //SDA output
	SDA_HIGH(me);	  	  
	SCL_HIGH(me);
	delay_us(4);
 	SDA_LOW(me);    //A high-to-low transition on the SDA line while the SCL is hige defines a START condition
	delay_us(4);
	SCL_LOW(me);    //Ready to transmit or resive data
}
static void IIC_STOP(SoftIIcProxy * me)
{
  SDA_OUT(me);  //SDA output
	SCL_LOW(me);
	SDA_LOW(me); //
 	delay_us(4);
	SCL_HIGH(me); 
	delay_us(4);			
	SDA_HIGH(me); //A low-to-high transition on the SDA line while the SCL is high defines a STOP condition
}
static u8 IIC_WAIT_ACK(SoftIIcProxy * me)
{
  u8 ucErrTime = 0;
	SDA_IN(me);      //release the SDA line
  delay_us(1);	   
	SCL_HIGH(me);
  delay_us(1);	 
	while(READ_SDA(me)){
		ucErrTime ++;
		if(ucErrTime>250){
			IIC_Stop();
			return 1;
		}
	}
  SCL_LOW(me);
	return 0; 
}
static void IIC_ACK(SoftIIcProxy * me)
{
  SCL_LOW(me);
	SDA_OUT(me);
	SDA_LOW(me);   //pull down the SDA line
	delay_us(2);
	SCL_HIGH(me);
	delay_us(2);
	SCL_LOW(me);
}
static void IIC_NACK(SoftIIcProxy * me)
{
  SCL_LOW(me);
	SDA_OUT(me);
	SDA_HIGH(me);  //SDA lines remains high
	delay_us(2);
	SCL_HIGH(me);
	delay_us(2);
	SCL_LOW(me);
}
static void IIC_Send_Byte(SoftIIcProxy * me, u8 txd)
{
  u8 t;   
	SDA_OUT(me); 	    
  SCL_LOW(me);   //pull down the SCK line
  //One data bit is transferred during each clock pulse of the SCL
  for(t = 0; t < 8; t ++) {
    if ((txd & 0x80) >> 7)   SDA_HIGH(me);
    else  SDA_LOW(me);        
    txd <<= 1; 	  
    delay_us(2);   
    SCL_HIGH(me);    //pull up the SCK line, and the slave will read the data
    delay_us(2); 
    SCL_LOW(me);	   //pull down the SCK line
    delay_us(2);
  }
}
static u8 IIC_Read_Byte(SoftIIcProxy * me, u8 ack)
{
  unsigned char i,receive=0;
	SDA_IN(me);   
  for(i = 0; i < 8; i ++ )
	{
    SCL_LOW(me);    //pull down the SCK line 
    delay_us(2);
		SCL_HIGH(me);    //pull up the SCK line
    receive <<= 1;
    if(READ_SDA(me)) receive ++; //read data bit  
		delay_us(1); 
  }					 
  if (!ack) IIC_NACK(me);//send nACK
  else  IIC_Ack(me); //send ACK   
  return receive;
}