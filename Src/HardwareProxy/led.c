/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-01-14 11:23:10
 * @LastEditors: PanHan
 * @LastEditTime: 2020-04-11 19:43:22
 * @FilePath: \water_gcc\Src\HardwareProxy\led.c
 */
#include "led.h"

/*
*函数功能：LED相关的GPIO初始化
*输入参数：无
*输出参数：无
*返 回 值：无
*/
void LED_Init(void)
{
  GPIO_InitTypeDef GPIO_Initure;
  __HAL_RCC_GPIOG_CLK_ENABLE();			//开启GPIOB时钟

  GPIO_Initure.Pin=GPIO_PIN_6; //PB0,1
  GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;  //推挽输出
  GPIO_Initure.Pull=GPIO_PULLUP;          //上拉
  GPIO_Initure.Speed=GPIO_SPEED_HIGH;     //高速
  HAL_GPIO_Init(GPIOG,&GPIO_Initure);     //初始化GPIOB.0和GPIOB.1

  HAL_GPIO_WritePin(GPIOG,GPIO_PIN_6,GPIO_PIN_SET);	//PB1置0
  

  HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_RESET);	//PB1置0
	
	__HAL_RCC_GPIOG_CLK_ENABLE();			//开启GPIOG时钟
}

/*
*函数功能：电磁阀相关的GPIO初始化
*输入参数：无
*返 回 值：无
*/
void valveInit(void)
{
	GPIO_InitTypeDef GPIO_Initure;
	/*开启上述IO口需要使用的时钟，总共包括ABCD四个端口，共19个IO*/
	__HAL_RCC_GPIOA_CLK_ENABLE();			
  __HAL_RCC_GPIOB_CLK_ENABLE();			
	__HAL_RCC_GPIOC_CLK_ENABLE();			
	__HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOG_CLK_ENABLE();	
	__HAL_RCC_GPIOH_CLK_ENABLE();

  /*所有IO口配置成同样的功能*/
  GPIO_Initure.Mode = GPIO_MODE_OUTPUT_PP;  //推挽输出
  GPIO_Initure.Pull = GPIO_PULLUP;          //上拉
  GPIO_Initure.Speed = GPIO_SPEED_HIGH;     //高速
	
	/*下面分别配置19个IO，并将IO置0*/
	GPIO_Initure.Pin = V_1_PIN;
  HAL_GPIO_Init(V_1_PORT, &GPIO_Initure);
  HAL_GPIO_WritePin(V_1_PORT, V_1_PIN, GPIO_PIN_RESET);	//置0
	
	GPIO_Initure.Pin = V_2_PIN;
  HAL_GPIO_Init(V_2_PORT, &GPIO_Initure);
  HAL_GPIO_WritePin(V_2_PORT, V_2_PIN, GPIO_PIN_RESET);	//置0
	
	GPIO_Initure.Pin = V_3_PIN;
  HAL_GPIO_Init(V_3_PORT, &GPIO_Initure);
  HAL_GPIO_WritePin(V_3_PORT, V_3_PIN, GPIO_PIN_RESET);	//置0
	
	GPIO_Initure.Pin = V_4_PIN;
  HAL_GPIO_Init(V_4_PORT, &GPIO_Initure);
  HAL_GPIO_WritePin(V_4_PORT, V_4_PIN, GPIO_PIN_RESET);	//置0
	
	GPIO_Initure.Pin = V_5_PIN;
  HAL_GPIO_Init(V_5_PORT, &GPIO_Initure);
  HAL_GPIO_WritePin(V_5_PORT, V_5_PIN, GPIO_PIN_RESET);	//置0
	
	GPIO_Initure.Pin = V_6_PIN;
  HAL_GPIO_Init(V_6_PORT, &GPIO_Initure);
  HAL_GPIO_WritePin(V_6_PORT, V_6_PIN, GPIO_PIN_RESET);	//置0
	
	GPIO_Initure.Pin = V_7_PIN;
  HAL_GPIO_Init(V_7_PORT, &GPIO_Initure);
  HAL_GPIO_WritePin(V_7_PORT, V_7_PIN, GPIO_PIN_RESET);	//置0
	
	GPIO_Initure.Pin = V_8_PIN;
  HAL_GPIO_Init(V_8_PORT, &GPIO_Initure);
  HAL_GPIO_WritePin(V_8_PORT, V_8_PIN, GPIO_PIN_RESET);	//置0
	
	GPIO_Initure.Pin = V_9_PIN;
  HAL_GPIO_Init(V_9_PORT, &GPIO_Initure);
  HAL_GPIO_WritePin(V_9_PORT, V_9_PIN, GPIO_PIN_RESET);	//置0
	
	GPIO_Initure.Pin = V_10_PIN;
  HAL_GPIO_Init(V_10_PORT, &GPIO_Initure);
  HAL_GPIO_WritePin(V_10_PORT, V_10_PIN, GPIO_PIN_RESET);	//置0
	
	GPIO_Initure.Pin = V_11_PIN;
  HAL_GPIO_Init(V_11_PORT, &GPIO_Initure);
  HAL_GPIO_WritePin(V_11_PORT, V_11_PIN, GPIO_PIN_RESET);	//置0
	
	GPIO_Initure.Pin = V_12_PIN;
  HAL_GPIO_Init(V_12_PORT, &GPIO_Initure);
  HAL_GPIO_WritePin(V_12_PORT, V_12_PIN, GPIO_PIN_RESET);	//置0
	
	GPIO_Initure.Pin = V_13_PIN;
  HAL_GPIO_Init(V_13_PORT, &GPIO_Initure);
  HAL_GPIO_WritePin(V_13_PORT, V_13_PIN, GPIO_PIN_RESET);	//置0
	
	GPIO_Initure.Pin = V_14_PIN;
  HAL_GPIO_Init(V_14_PORT, &GPIO_Initure);
  HAL_GPIO_WritePin(V_14_PORT, V_14_PIN, GPIO_PIN_RESET);	//置0
	
	GPIO_Initure.Pin = V_15_PIN;
  HAL_GPIO_Init(V_15_PORT, &GPIO_Initure);
  HAL_GPIO_WritePin(V_15_PORT, V_15_PIN, GPIO_PIN_RESET);	//置0
	
	GPIO_Initure.Pin = V_16_PIN;
  HAL_GPIO_Init(V_16_PORT, &GPIO_Initure);
  HAL_GPIO_WritePin(V_16_PORT, V_16_PIN, GPIO_PIN_RESET);	//置0
	
	GPIO_Initure.Pin = V_17_PIN;
  HAL_GPIO_Init(V_17_PORT, &GPIO_Initure);
  HAL_GPIO_WritePin(V_17_PORT, V_17_PIN, GPIO_PIN_RESET);	//置0
	
	GPIO_Initure.Pin = V_18_PIN;
  HAL_GPIO_Init(V_18_PORT, &GPIO_Initure);
  HAL_GPIO_WritePin(V_18_PORT, V_18_PIN, GPIO_PIN_RESET);	//置0
	
	GPIO_Initure.Pin = V_19_PIN;
  HAL_GPIO_Init(V_19_PORT, &GPIO_Initure);
  HAL_GPIO_WritePin(V_19_PORT, V_19_PIN, GPIO_PIN_RESET);	//置0
	
	GPIO_Initure.Pin = V_20_PIN;
  HAL_GPIO_Init(V_20_PORT, &GPIO_Initure);
  HAL_GPIO_WritePin(V_20_PORT, V_20_PIN, GPIO_PIN_RESET);	//置0
	
	GPIO_Initure.Pin = V_21_PIN;
  HAL_GPIO_Init(V_21_PORT, &GPIO_Initure);
  HAL_GPIO_WritePin(V_21_PORT, V_21_PIN, GPIO_PIN_RESET);	//置0
	
	GPIO_Initure.Pin = V_22_PIN;
  HAL_GPIO_Init(V_22_PORT, &GPIO_Initure);
  HAL_GPIO_WritePin(V_22_PORT, V_22_PIN, GPIO_PIN_RESET);	//置0
}


/*
*函数功能：LED0电平翻转
*输入参数：无
*输出参数：无
*返 回 值：无
*/
void led0Revers(void)
{
  HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_6);
}
/*
*函数功能：LED1电平翻转
*输入参数：无
*输出参数：无
*返 回 值：无
*/
void led1Revers(void)
{
  HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_1);
}


typedef struct
{
  u8 m_curState;  //current state
  u8 m_preState;  //pre-state
  GPIO_TypeDef * m_port;  //gpio port
  u16 m_pin;              //gpio pin
  u8 m_onLevel;   //voltage Level when Switch on the valve
  valveIO_t m_type; //
}MagbeticValveData;

static void init(MagbeticValve * me, GPIO_TypeDef * port, u16 pin, u8 onLevel);
static void setState(MagbeticValve * me, u8 state);
static u8 getState(MagbeticValve * me);


static MagbeticValveData s_MagbeticValveData[MAX_NUM_V];
static MagbeticValve s_MagbeticValve[MAX_NUM_V];

MagbeticValve * MagbeticValve_create(valveIO_t type)
{
  MagbeticValve * me = & s_MagbeticValve[type];
  MagbeticValveData * pMagData = 0;
  if (0 == me->init){
    me->init = init;
    me->setState = setState;
    me->getState = getState;
    me->private_data = & s_MagbeticValveData[type];
    pMagData = (MagbeticValveData *)me->private_data;
    pMagData->m_type = type;
  }
  return me;
}

/**
 * @brief	initialization
 * @param	port:gpio port
 * @param pin:gpio pin
 * @param onLevel:voltage Level when Switch on the valve
 */
static void init(MagbeticValve * me, GPIO_TypeDef * port, u16 pin, u8 onLevel)
{
  MagbeticValveData * privateData = (MagbeticValveData *)me->private_data;
  GPIO_InitTypeDef GPIO_Initure;
  static u8 initFlag = 0;
  if (0 == initFlag){
    /*开启上述IO口需要使用的时钟，总共包括ABCD四个端口，共19个IO*/
    __HAL_RCC_GPIOA_CLK_ENABLE();			
    __HAL_RCC_GPIOB_CLK_ENABLE();			
    __HAL_RCC_GPIOC_CLK_ENABLE();			
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();	
    __HAL_RCC_GPIOH_CLK_ENABLE();
    initFlag = 1;
  }
	
  privateData->m_port = port;
  privateData->m_pin = pin;
  privateData->m_onLevel = onLevel;
  
  /*IO initialization*/
  GPIO_Initure.Mode = GPIO_MODE_OUTPUT_PP;  //推挽输出
  GPIO_Initure.Pull = GPIO_PULLUP;          //上拉
  GPIO_Initure.Speed = GPIO_SPEED_HIGH;     //高速
	GPIO_Initure.Pin = privateData->m_pin;
  HAL_GPIO_Init(privateData->m_port, &GPIO_Initure);
  
  me->setState(me, 0);
}

/**
 * @brief	setting valve state
 * @param	state:valve state, 0 off, 1 on
 */
static void setState(MagbeticValve * me, u8 state)
{
  MagbeticValveData * privateData = (MagbeticValveData *)me->private_data;
  privateData->m_preState = privateData->m_curState;
  privateData->m_curState = state;
  if (0 == state){
    if (1 == privateData->m_onLevel)  HAL_GPIO_WritePin(privateData->m_port, privateData->m_pin, GPIO_PIN_RESET);
    else HAL_GPIO_WritePin(privateData->m_port, privateData->m_pin, GPIO_PIN_SET);
  }
  else{
    if (1 == privateData->m_onLevel)  HAL_GPIO_WritePin(privateData->m_port, privateData->m_pin, GPIO_PIN_SET);
    else HAL_GPIO_WritePin(privateData->m_port, privateData->m_pin, GPIO_PIN_RESET);
  }
}

/**
 * @brief	getting valve state
 * @param	
 * @retval	0 off, 1 on
 */
static u8 getState(MagbeticValve * me)
{
  MagbeticValveData * privateData = (MagbeticValveData *)me->private_data;
  return privateData->m_curState;
}