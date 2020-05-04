/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-03-05 22:13:15
 * @LastEditors: PanHan
 * @LastEditTime: 2020-04-20 18:05:57
 * @FilePath: \water_gcc\Src\FunctionMoudleLayer\FML_ExtValve.c
 */
#include "FML_ExtValve.h"
#include "led.h"
#include "FML_Grat.h"

typedef struct
{
  MagbeticValve * pMagValveArr[MAX_NUM_V];  //pointer to the magbetic valve object
  u8 m_vStateArr[MAX_NUM_V];       //静态数组保存每个电磁阀的开关状态，0表示关闭，1表示开启
  NotifyHandle m_notifyHandle[CB_V_MAX];
}MagValveMangData;

static NotifyHandle s_notifyHandle[CB_V_MAX];

static void valve_init(ValveIODev * const me);   //初始化电磁阀IO外设
static void valve_setVal(ValveIODev * const me, valveIO_t valve, u8 state); //设置阀门状态
static u8 valve_getVal(ValveIODev * const me, valveIO_t valve); //获取阀门状态
static void ExitSing_registerCallback(ValveIODev * me, void * pInstance, const pGeneralCb p, ValveCbType type);

static ValveIODev valveIODev;  //静态创建电磁阀管理器对象
static MagValveMangData s_MagValveMangData;
/*
*函数功能：创建电磁阀IO设备对象（单例）
*输入参数：无
*输出参数：无
*返 回 值：me:指向对象的指针
*/
ValveIODev * valveIODevCreate(void)
{
	static ValveIODev * me = 0;
  int i = 0;
	if (me == 0){
		me = & valveIODev;
		me->init = valve_init;
		me->setVal = valve_setVal;
		me->getVal = valve_getVal;
    me->registerCallback = ExitSing_registerCallback;
		
    for (i = 0; i < CB_V_MAX; i ++){
      me->m_notifyHandle[i] = & s_notifyHandle[i];
    }
    me->private_data = &s_MagValveMangData;
    
		me->init(me);  //初始化外设
	}
	return me;
}
/*
*函数功能：初始化电磁阀控制相关的IO口
*输入参数：me:指向对象的指针
*输出参数：无
*返 回 值：无
*/
static void valve_init(ValveIODev * const me)
{
  int i = 0;
  MagValveMangData * privateData = (MagValveMangData *)me->private_data;
  //create magbetic valve object
  for (i = 0; i < MAX_NUM_V; i ++){
    privateData->pMagValveArr[i] = MagbeticValve_create(i);
  }
  /*valve object initialization*/
  privateData->pMagValveArr[WATER_IN_V]->init(privateData->pMagValveArr[WATER_IN_V], V_1_PORT, V_1_PIN, 1);
  privateData->pMagValveArr[PRES_TO_WATER_V]->init(privateData->pMagValveArr[PRES_TO_WATER_V], V_2_PORT, V_2_PIN, 1);
  privateData->pMagValveArr[PUMP_WATER_V]->init(privateData->pMagValveArr[PUMP_WATER_V], V_3_PORT, V_3_PIN, 1);
  privateData->pMagValveArr[AIR_IN_V]->init(privateData->pMagValveArr[AIR_IN_V], V_4_PORT, V_4_PIN, 1);
  privateData->pMagValveArr[WATER_OUT_V]->init(privateData->pMagValveArr[WATER_OUT_V], V_5_PORT, V_5_PIN, 1);
  privateData->pMagValveArr[DRY_V]->init(privateData->pMagValveArr[DRY_V], V_6_PORT, V_6_PIN, 1);
  privateData->pMagValveArr[BIG_VACUUM_V]->init(privateData->pMagValveArr[BIG_VACUUM_V], V_7_PORT, V_7_PIN, 1);
  privateData->pMagValveArr[BIG_LEAK_1_V]->init(privateData->pMagValveArr[BIG_LEAK_1_V], V_10_PORT, V_10_PIN, 1);
  privateData->pMagValveArr[BIG_LEAK_2_V]->init(privateData->pMagValveArr[BIG_LEAK_2_V], V_8_PORT, V_8_PIN, 1);
  privateData->pMagValveArr[BIG_LEAK_3_V]->init(privateData->pMagValveArr[BIG_LEAK_3_V], V_9_PORT, V_9_PIN, 1);
  privateData->pMagValveArr[BIG_LEAK_4_V]->init(privateData->pMagValveArr[BIG_LEAK_4_V], V_11_PORT, V_11_PIN, 1);
  privateData->pMagValveArr[LIT_VACUUM_V]->init(privateData->pMagValveArr[LIT_VACUUM_V], V_12_PORT, V_12_PIN, 1);
  privateData->pMagValveArr[LIT_LEAK_1_V]->init(privateData->pMagValveArr[LIT_LEAK_1_V], V_15_PORT, V_15_PIN, 1);
  privateData->pMagValveArr[LIT_LEAK_2_V]->init(privateData->pMagValveArr[LIT_LEAK_2_V], V_13_PORT, V_13_PIN, 1);
  privateData->pMagValveArr[LIT_LEAK_3_V]->init(privateData->pMagValveArr[LIT_LEAK_3_V], V_14_PORT, V_14_PIN, 1);
  privateData->pMagValveArr[LIT_LEAK_4_V]->init(privateData->pMagValveArr[LIT_LEAK_4_V], V_16_PORT, V_16_PIN, 1);
  privateData->pMagValveArr[AIR_OUT_V]->init(privateData->pMagValveArr[AIR_OUT_V], V_17_PORT, V_17_PIN, 1);
  privateData->pMagValveArr[NEEDLE_V]->init(privateData->pMagValveArr[NEEDLE_V], V_18_PORT, V_18_PIN, 1);
  privateData->pMagValveArr[BALANCE_V]->init(privateData->pMagValveArr[BALANCE_V], V_19_PORT, V_19_PIN, 1);
  privateData->pMagValveArr[WATER_PUMP]->init(privateData->pMagValveArr[WATER_PUMP], V_20_PORT, V_20_PIN, 1);
  privateData->pMagValveArr[CYL_H_V]->init(privateData->pMagValveArr[CYL_H_V], V_21_PORT, V_21_PIN, 1);
  privateData->pMagValveArr[CYL_L_V]->init(privateData->pMagValveArr[CYL_L_V], V_22_PORT, V_22_PIN, 1);
}

/*
*函数功能：设置电磁阀控制相关的IO口
*输入参数：me:指向对象的指针;	valve:阀门类型; state:阀门状态，0关闭，1开启
*输出参数：无
*返 回 值：无
*/
static void valve_setVal(ValveIODev * const me, valveIO_t valve, u8 state)
{
  MagValveMangData * privateData = (MagValveMangData *)me->private_data;
  GratDev * pGratDev = GratDev_create();

  privateData->pMagValveArr[valve]->setState(privateData->pMagValveArr[valve], state);
  privateData->m_vStateArr[valve] = state;

  if (pGratDev->isTrig(pGratDev)){
    if ((CYL_L_V == valve) || (CYL_H_V == valve)){
      pGratDev->updateCylPos(pGratDev, privateData->m_vStateArr[CYL_H_V], privateData->m_vStateArr[CYL_L_V]);
      privateData->m_vStateArr[valve] = 0;
      privateData->pMagValveArr[valve]->setState(privateData->pMagValveArr[valve], 0);
    } 
  } 
  
  //阀门状态由变化，调用相应的回调函数
  if ((me->m_notifyHandle[CB_V_DISP]->pInstance != 0)&&(me->m_notifyHandle[CB_V_DISP]->callback != 0)){
    me->m_notifyHandle[CB_V_DISP]->callback(me->m_notifyHandle[CB_V_DISP]->pInstance, privateData->m_vStateArr);
  }
}    

/*
*函数功能：获取电磁阀控制相关的IO口状态
*输入参数：me:指向对象的指针;	valve:阀门类型
*输出参数：无
*返 回 值：阀门状态，0关闭，1开启
*/
static u8 valve_getVal(ValveIODev * const me, valveIO_t valve)
{
  MagValveMangData * privateData = (MagValveMangData *)me->private_data;
	return privateData->m_vStateArr[valve];
}

/**
  * @brief	注册回调函数
  * @param	pInstance:一般指向注册回调函数的对象
  * @param  p:指向回调函数的指针
  * @param  type:回调函数类型
  * @retval	
  */
static void ExitSing_registerCallback(ValveIODev * me, void * pInstance, const pGeneralCb p, ValveCbType type)
{
  me->m_notifyHandle[type]->pInstance = pInstance;
  me->m_notifyHandle[type]->callback = p;
}


