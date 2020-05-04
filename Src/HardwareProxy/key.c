/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-01-14 11:23:10
 * @LastEditors: PanHan
 * @LastEditTime: 2020-04-06 15:52:52
 * @FilePath: \water_gcc\Src\HardwareProxy\key.c
 */
#include "key.h"
#include "delay.h"

/*液位开关信号PI11
	真空传感器  PH8
	干燥压力传感器PC1
	气缸上信号  PB12 //2线
	气缸下信号  PH10 //2线
	光栅信号    PC13
*/
#define LIQ_SWITCH_PIN 			    GPIO_PIN_11
#define LIQ_SWITCH_PORT 		    GPIOI
#define VACUUM_SENS_PIN 		    GPIO_PIN_2
#define VACUUM_SENS_PORT 				GPIOH
#define PRES_SENS_PIN 					GPIO_PIN_8
#define PRES_SENS_PORT 		  		GPIOH
#define CYL_HIGH_POS_PIN        GPIO_PIN_0
#define CYL_HIGH_POS_PORT       GPIOB
#define CYL_LOW_POS_PIN         GPIO_PIN_3
#define CYL_LOW_POS_PORT        GPIOH
#define GRAT_PIN 					      GPIO_PIN_13
#define GRAT_PORT 		  		    GPIOC

typedef struct
{
  GPIO_TypeDef * m_port;  //gpio port
  u16 m_pin;              //gpio pin

  VolTrigType m_trigType; //trig type high or low
  ExitSing_t m_sigType;   //信号类型
  u32 m_trigCountLimit[2];  //触发或者解除触发计数判定限

  u32 m_trigCount[2];     //触发和解除触发计数
  SensorState m_state;    //传感器状态
}KeyData;

static void init(SwitchSensor * me);
static u8 singDetect(SwitchSensor * me, SensorState * state); //信号检测
static void setTrigCountLimit(SwitchSensor * me, u32 trig_on, u32 trig_off);//设置触发计数
static void setTrigType(SwitchSensor * me, VolTrigType type);//设置触发电平


static const GPIO_TypeDef * c_portArr[SING_MAX] = {
  LIQ_SWITCH_PORT, VACUUM_SENS_PORT, PRES_SENS_PORT, CYL_HIGH_POS_PORT, CYL_LOW_POS_PORT, GRAT_PORT
};

static const u16 c_pinArr[SING_MAX] = {
  LIQ_SWITCH_PIN, VACUUM_SENS_PIN, PRES_SENS_PIN, CYL_HIGH_POS_PIN, CYL_LOW_POS_PIN, GRAT_PIN
};

static SwitchSensor switchSensor[SING_MAX];
static KeyData s_privateData[SING_MAX];

/**
 * @brief	创建开关传感器对象
 * @param	type:传感器类型
 * @param port:传感器使用的port
 * @param pin:传感器使用的引脚
 * @retval	指向传感器对象的指针
 */
SwitchSensor * SwitchSensor_create(ExitSing_t type)
{
  SwitchSensor * me = & switchSensor[type];
  KeyData * pKeyData = 0;
  if (0 == me->init){
    me->init = init;
    me->singDetect = singDetect;
    me->setTrigCountLimit = setTrigCountLimit;
    me->setTrigType = setTrigType;

    me->private_data = & s_privateData[type];
    pKeyData = (KeyData *)me->private_data;
    pKeyData->m_sigType = type;
    pKeyData->m_port = c_portArr[type];
    pKeyData->m_pin = c_pinArr[type];

    me->init(me);
  }
  return me;
}

/**
 * @brief	初始化工作，主要是初始化相应的GPIO
 * @param	
 * @retval	
 */
static void init(SwitchSensor * me)
{
  KeyData * private_data = (KeyData *)me->private_data;
  GPIO_InitTypeDef GPIO_Initure;
	
	__HAL_RCC_GPIOC_CLK_ENABLE();           //开启GPIOC时钟
	__HAL_RCC_GPIOH_CLK_ENABLE();           //开启GPIOH时钟
	__HAL_RCC_GPIOG_CLK_ENABLE();           //开启GPIOG时钟
	__HAL_RCC_GPIOI_CLK_ENABLE();           //开启GPIOH时钟
	__HAL_RCC_GPIOA_CLK_ENABLE();           //开启GPIOA时钟
	__HAL_RCC_GPIOB_CLK_ENABLE();           //开启GPIOA时钟
		
	GPIO_Initure.Mode = GPIO_MODE_INPUT;      //输入
	GPIO_Initure.Pull = GPIO_PULLDOWN;        //下拉
	GPIO_Initure.Speed = GPIO_SPEED_HIGH;     //高速

  GPIO_Initure.Pin = private_data->m_pin;            
	HAL_GPIO_Init(private_data->m_port, &GPIO_Initure);

  private_data->m_trigCount[TRIG_ON] = 0;
  private_data->m_trigCount[TRIG_OFF] = 0;
  private_data->m_state = TRIG_OFF;
}

/**
 * @brief	信号检测
 * @param	state:保存信号检测结果
 * @retval	0信号无变化，1信号有变化
 */
static u8 singDetect(SwitchSensor * me, SensorState * state)
{
  KeyData * private_data = (KeyData *)me->private_data;
  u8 changeFlag = 0;  //状态改变标志位
	if (private_data->m_trigType == HAL_GPIO_ReadPin(private_data->m_port, private_data->m_pin)){
    /*触发*/
		private_data->m_trigCount[TRIG_ON] ++;
    private_data->m_trigCount[TRIG_OFF] = 0;
		if ((private_data->m_state == TRIG_OFF) && \
        (private_data->m_trigCount[TRIG_ON] >= private_data->m_trigCountLimit[TRIG_ON])){
			private_data->m_state = TRIG_ON;
			changeFlag = 1;
		}
	}else{
		private_data->m_trigCount[TRIG_OFF] ++;
    private_data->m_trigCount[TRIG_ON] = 0;
		if ((private_data->m_state == TRIG_ON) && \
        (private_data->m_trigCount[TRIG_OFF] >= private_data->m_trigCountLimit[TRIG_OFF])){
			private_data->m_state = TRIG_OFF;
			changeFlag = 1;
		} 
	}

  * state = private_data->m_state;
  return changeFlag;
}

/**
 * @brief	设置信号检测计数限制，用于滤波
 * @param	trig_on:触发信号的判定
 * @param trig_off:解除触发的判定
 * @retval	
 */
static void setTrigCountLimit(SwitchSensor * me, u32 trig_on, u32 trig_off)
{
  KeyData * private_data = (KeyData *)me->private_data;
  private_data->m_trigCountLimit[TRIG_ON] = trig_on;
  private_data->m_trigCountLimit[TRIG_OFF] = trig_off;
}

/**
 * @brief	设置触发类型（高电平触发或者低电平触发）
 * @param	
 * @retval	
 */
static void setTrigType(SwitchSensor * me, VolTrigType type)
{
  KeyData * private_data = (KeyData *)me->private_data;
  private_data->m_trigType = type;
}

/*
*函数功能：外部信号输入相关的GPIO初始化
*输入参数：无
*返 回 值：无
*/
void extIO_Init(void)
{
	GPIO_InitTypeDef GPIO_Initure;
	
	__HAL_RCC_GPIOC_CLK_ENABLE();           //开启GPIOC时钟
	__HAL_RCC_GPIOH_CLK_ENABLE();           //开启GPIOH时钟
	__HAL_RCC_GPIOG_CLK_ENABLE();           //开启GPIOG时钟
	__HAL_RCC_GPIOI_CLK_ENABLE();           //开启GPIOH时钟
	__HAL_RCC_GPIOA_CLK_ENABLE();           //开启GPIOA时钟
	__HAL_RCC_GPIOB_CLK_ENABLE();           //开启GPIOA时钟
		
	GPIO_Initure.Mode = GPIO_MODE_INPUT;      //输入
	GPIO_Initure.Pull = GPIO_PULLDOWN;        //下拉
	GPIO_Initure.Speed = GPIO_SPEED_HIGH;     //高速

	GPIO_Initure.Pin = LIQ_SWITCH_PIN;            
	HAL_GPIO_Init(LIQ_SWITCH_PORT, &GPIO_Initure);

	GPIO_Initure.Pin = VACUUM_SENS_PIN;           
	HAL_GPIO_Init(VACUUM_SENS_PORT, &GPIO_Initure);

	GPIO_Initure.Pin = PRES_SENS_PIN;            
	HAL_GPIO_Init(PRES_SENS_PORT, &GPIO_Initure);
	
	GPIO_Initure.Pin = CYL_HIGH_POS_PIN;            
	HAL_GPIO_Init(CYL_HIGH_POS_PORT, &GPIO_Initure);

	GPIO_Initure.Pin = CYL_LOW_POS_PIN;           
	HAL_GPIO_Init(CYL_LOW_POS_PORT, &GPIO_Initure);

	GPIO_Initure.Pin = GRAT_PIN;            
	HAL_GPIO_Init(GRAT_PORT, &GPIO_Initure);
}