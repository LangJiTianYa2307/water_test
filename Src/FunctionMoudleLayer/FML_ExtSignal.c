/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-03-05 16:00:31
 * @LastEditors: PanHan
 * @LastEditTime: 2020-04-16 15:37:19
 * @FilePath: \water_gcc\Src\FunctionMoudleLayer\FML_ExtSignal.c
 */
#include "FML_ExtSignal.h"
#include "key.h"
#include "APP.h"

#define EXT_SING_DECT_MS  10

#define WATER_DECT_TRIG_TIME_MS 100
#define WATER_DECT_UNTRIG_TIME_MS 500
#define VACUUM_TRIG_TIME_MS 200
#define VACUUM_UNTRIG_TIME_MS 50
#define AIR_PRES_TRIG_TIME_MS 100
#define AIR_PRES_UNTRIG_TIME_MS 100
#define H_CLY_TRIG_TIME_MS 100
#define H_CLY_UNTRIG_TIME_MS 50
#define L_CLY_TRIG_TIME_MS 100
#define L_CLY_UNTRIG_TIME_MS 50
#define GRAT_TRIG_TIME_MS 20
#define GRAT_UNTRIG_TIME_MS 50

#define LIQ_TRIG   (u32)(WATER_DECT_TRIG_TIME_MS / EXT_SING_DECT_MS)
#define LIQ_UNTRIG (u32)(WATER_DECT_UNTRIG_TIME_MS / EXT_SING_DECT_MS)
#define VACUUM_TRIG       (u32)(VACUUM_TRIG_TIME_MS / EXT_SING_DECT_MS)
#define VACUUM_UNTRIG     (u32)(VACUUM_UNTRIG_TIME_MS / EXT_SING_DECT_MS)
#define AIR_TRIG     (u32)(AIR_PRES_TRIG_TIME_MS / EXT_SING_DECT_MS)
#define AIR_UNTRIG   (u32)(AIR_PRES_UNTRIG_TIME_MS / EXT_SING_DECT_MS)
#define H_CLY_TRIG        (u32)(H_CLY_TRIG_TIME_MS / EXT_SING_DECT_MS)
#define H_CLY_UNTRIG      (u32)(H_CLY_UNTRIG_TIME_MS / EXT_SING_DECT_MS)
#define L_CLY_TRIG        (u32)(L_CLY_TRIG_TIME_MS / EXT_SING_DECT_MS)
#define L_CLY_UNTRIG      (u32)(L_CLY_UNTRIG_TIME_MS / EXT_SING_DECT_MS)
#define GRAT_TRIG         (u32)(GRAT_TRIG_TIME_MS / EXT_SING_DECT_MS)
#define GRAT_UNTRIG       (u32)(GRAT_UNTRIG_TIME_MS / EXT_SING_DECT_MS)

static void ExitSing_init(ExitSing * const me);  //初始化
static void ExitSing_singDetect(ExitSing * const me); //检测外部信号
static u8 ExitSing_getExitSing(ExitSing * const me, ExitSing_t type);
static void ExitSing_clearFlag(ExitSing * const me, ExitSing_t type);//清除相应的标志位
static void ExitSing_setMask(ExitSing_t type);
static void exitSwitchCb(TimerHandle_t xTimer);
static void ExitSing_registerCallback(ExitSing * me, void * pInstance, const pExtSigCb p, ExtSigCbType type);
/*-----------------------------------------私有成员-----------------------------------------*/
ExitSing exitSing;  //静态分配
static NotifyHandle s_notifyHandle[EXT_MAX]; //通知句柄数组，用于保存回调函数和调用对象
u8 exitSingStateArr[8];
static u8 s_extSigState;  //各传感器状态，0无信号，1有信号(结果已经过去抖动),用于输出给外部
static TimerHandle_t s_timerHandle;
/*-------------------------------------------------------------------------------------------*/

/*
*函数功能：创建外部信号监控对象
*输入参数：无
*返 回 值：指向对象的指针
*/
ExitSing * exitSingCreate(void)
{
	static ExitSing * me = 0;
  int i = 0;
	if (me == 0){
		me = &exitSing;
		me->init = ExitSing_init;
		me->singDetect = ExitSing_singDetect;
		me->getExitSing = ExitSing_getExitSing;
		me->m_pStateArr = exitSingStateArr;
		me->clearFlag = ExitSing_clearFlag;
		me->registerCallback = ExitSing_registerCallback;

    //将静态分配的通知句柄地址传递给对象成员
    for (i = 0; i < EXT_MAX; i ++){
      me->m_notifyHandle[i] = & s_notifyHandle[i];
    }

    //创建传感器对象
    for (i = 0; i < SING_MAX; i ++){
      me->pSwitchSensorArr[i] = SwitchSensor_create(i);
    }
		me->m_changeFlag = 0;
		me->m_maskBit = 0;
    
		me->init(me);
	}
	return me;
}

/*
*函数功能：初始化外设，读取外部信号状态，并初始化保存其状态的数组
*输入参数：me:指向对象的指针
*返 回 值：无
*/
static void ExitSing_init(ExitSing * const me)
{
  me->pSwitchSensorArr[WATER_DECT_SING]->setTrigType(me->pSwitchSensorArr[WATER_DECT_SING], HIGH_LEVEL_TRIG);
  me->pSwitchSensorArr[WATER_DECT_SING]->setTrigCountLimit(me->pSwitchSensorArr[WATER_DECT_SING], LIQ_TRIG, LIQ_UNTRIG);

  me->pSwitchSensorArr[VACUUM_SING]->setTrigType(me->pSwitchSensorArr[VACUUM_SING], HIGH_LEVEL_TRIG);
  me->pSwitchSensorArr[VACUUM_SING]->setTrigCountLimit(me->pSwitchSensorArr[VACUUM_SING], VACUUM_TRIG, VACUUM_UNTRIG);

  me->pSwitchSensorArr[AIR_PRES_SING]->setTrigType(me->pSwitchSensorArr[AIR_PRES_SING], HIGH_LEVEL_TRIG);
  me->pSwitchSensorArr[AIR_PRES_SING]->setTrigCountLimit(me->pSwitchSensorArr[AIR_PRES_SING], AIR_TRIG, AIR_UNTRIG);

  me->pSwitchSensorArr[H_CLY_POS_SING]->setTrigType(me->pSwitchSensorArr[H_CLY_POS_SING], HIGH_LEVEL_TRIG);
  me->pSwitchSensorArr[H_CLY_POS_SING]->setTrigCountLimit(me->pSwitchSensorArr[H_CLY_POS_SING], H_CLY_TRIG, H_CLY_UNTRIG);

  me->pSwitchSensorArr[L_CLY_POS_SING]->setTrigType(me->pSwitchSensorArr[L_CLY_POS_SING], HIGH_LEVEL_TRIG);
  me->pSwitchSensorArr[L_CLY_POS_SING]->setTrigCountLimit(me->pSwitchSensorArr[L_CLY_POS_SING], L_CLY_TRIG, L_CLY_UNTRIG);

  me->pSwitchSensorArr[GRAT_SING]->setTrigType(me->pSwitchSensorArr[GRAT_SING], LOW_LEVEL_TRIG);
  me->pSwitchSensorArr[GRAT_SING]->setTrigCountLimit(me->pSwitchSensorArr[GRAT_SING], GRAT_TRIG, GRAT_UNTRIG);
  s_timerHandle = xTimerCreate((const char*    )"extSig",
                                (TickType_t     )EXT_SING_DECT_MS,
                                (UBaseType_t    )pdTRUE,
                                (void *         )me,
                                (TimerCallbackFunction_t)exitSwitchCb);
  /*首次检测外部信号。需要对信号状态进行初始化*/
  if (me->getExitSing(me, VACUUM_SING)) s_extSigState |= (1 << VACUUM_SING);
  if (me->getExitSing(me, WATER_DECT_SING)) s_extSigState |= (1 << WATER_DECT_SING);
  if (me->getExitSing(me, H_CLY_POS_SING)) s_extSigState |= (1 << H_CLY_POS_SING);
  if (me->getExitSing(me, L_CLY_POS_SING)) s_extSigState |= (1 << L_CLY_POS_SING);
  if (me->getExitSing(me, GRAT_SING))      s_extSigState |= (1 << GRAT_SING);
  
  
  me->clearFlag(me, VACUUM_SING);     //清除掉相应的标志位
  me->clearFlag(me, WATER_DECT_SING); //清除掉相应的标志位
  me->clearFlag(me, H_CLY_POS_SING);  //清除掉相应的标志位
  me->clearFlag(me, L_CLY_POS_SING);  //清除掉相应的标志位
  me->clearFlag(me, GRAT_SING);       //清除掉相应的标志位

  //发送信号
  if (me->m_notifyHandle[EXT_UPDATE]->callback){
    me->m_notifyHandle[EXT_UPDATE]->callback(me->m_notifyHandle[EXT_UPDATE]->pInstance, &s_extSigState);
  }
  xTimerStart(s_timerHandle, 0);   //启动定时器
}

/*
*函数功能：检测各个信号,若信号有变化则对相应的标志位进行置位
*输入参数：me:指向对象的指针
*返 回 值：无
*/
static void ExitSing_singDetect(ExitSing * const me)
{
  u32  i = 0;
  for (i = 0; i < SING_MAX; i ++){
    if (me->pSwitchSensorArr[i]->singDetect(me->pSwitchSensorArr[i], & me->m_pStateArr[i])){
      me->m_changeFlag |= (1 << i);
    }
  }
}

/*
*函数功能：返回信号状态
*输入参数：me:指向对象的指针;type:信号类型
*返 回 值：0关闭，1开启
*/
static u8 ExitSing_getExitSing(ExitSing * const me, ExitSing_t type)
{
	return me->m_pStateArr[type];
}

/*
*函数功能：清除指定的标志位
*输入参数：me:指向对象的指针;type:信号类型
*返 回 值：无
*/
static void ExitSing_clearFlag(ExitSing * const me, ExitSing_t type)
{
	me->m_changeFlag &= ~(1 << type);
}


/*
*函数功能：设置屏蔽位
*输入参数：type:信号类型
*返 回 值：无
*/
static void ExitSing_setMask(ExitSing_t type)
{
	exitSing.m_maskBit |= (1 << type);
}



/*
*函数功能：注册外部开关信号箱定时采集数据
*输入参数：xTimer:定时器句柄
*返 回 值：无
*/
static void exitSwitchCb(TimerHandle_t xTimer)
{
	ExitSing * me = exitSingCreate();
	u8 sigState = 0;
  AppMesage msg;
	me->singDetect(me);   //遍历所有传感器
  //检测到有变化
  if (me->m_changeFlag & 0x01) {
    sigState = me->getExitSing(me, WATER_DECT_SING);
    if (sigState) s_extSigState |= (1 << WATER_DECT_SING);
    else s_extSigState &= ~(1 << WATER_DECT_SING);
    
    msg.dataType = WATER_DECT_SING;
    msg.pVoid = (void *)sigState;

    //信号有变化，发送信号
    if (me->m_notifyHandle[EXT_SIG_CHANGE]->callback){
      me->m_notifyHandle[EXT_SIG_CHANGE]->callback(me->m_notifyHandle[EXT_SIG_CHANGE]->pInstance, &msg);
    }

    me->clearFlag(me, WATER_DECT_SING); //清除掉相应的标志位
  }
  //真空传感器有变化
  if (me->m_changeFlag & 0x02) {
    sigState = me->getExitSing(me, VACUUM_SING);
    if (sigState) s_extSigState |= (1 << VACUUM_SING);
    else s_extSigState &= ~(1 << VACUUM_SING);

    msg.dataType = VACUUM_SING;
    msg.pVoid = (void *)sigState;
    //信号有变化，发送信号
    if (me->m_notifyHandle[EXT_SIG_CHANGE]->callback){
      me->m_notifyHandle[EXT_SIG_CHANGE]->callback(me->m_notifyHandle[EXT_SIG_CHANGE]->pInstance, &msg);
    }
    me->clearFlag(me, VACUUM_SING); //清除掉相应的标志位
	}
	/*气缸上限外置信号有变化或者其没有被屏蔽,就会检测位置传感器*/
  if ((me->m_changeFlag & 0x08)) {
    sigState = me->getExitSing(me, H_CLY_POS_SING);
    if (sigState) s_extSigState |= (1 << H_CLY_POS_SING);
    else s_extSigState &= ~(1 << H_CLY_POS_SING);

    msg.dataType = H_CLY_POS_SING;
    msg.pVoid = (void *)sigState;
    //信号有变化，发送信号
    if (me->m_notifyHandle[EXT_SIG_CHANGE]->callback){
      me->m_notifyHandle[EXT_SIG_CHANGE]->callback(me->m_notifyHandle[EXT_SIG_CHANGE]->pInstance, &msg);
    }
    me->clearFlag(me, H_CLY_POS_SING);
  }
  /*气缸下限外置信号有变化或者其没有被屏蔽,就会检测位置传感器*/
  if ((me->m_changeFlag & 0x10)) {
    sigState = me->getExitSing(me, L_CLY_POS_SING);
    if (sigState) s_extSigState |= (1 << L_CLY_POS_SING);
    else s_extSigState &= ~(1 << L_CLY_POS_SING);

    msg.dataType = L_CLY_POS_SING;
    msg.pVoid = (void *)sigState;
    //信号有变化，发送信号
    if (me->m_notifyHandle[EXT_SIG_CHANGE]->callback){
      me->m_notifyHandle[EXT_SIG_CHANGE]->callback(me->m_notifyHandle[EXT_SIG_CHANGE]->pInstance, &msg);
    }
    me->clearFlag(me, L_CLY_POS_SING);
  }
  /*光栅信号有变化*/
  if (me->m_changeFlag & 0x20) 
  {
    sigState = me->getExitSing(me, GRAT_SING);
    if (sigState) s_extSigState |= (1 << GRAT_SING);
    else s_extSigState &= ~(1 << GRAT_SING);

    msg.dataType = GRAT_SING;
    msg.pVoid = (void *)sigState;
    //信号有变化，发送信号
    if (me->m_notifyHandle[EXT_SIG_CHANGE]->callback){
      me->m_notifyHandle[EXT_SIG_CHANGE]->callback(me->m_notifyHandle[EXT_SIG_CHANGE]->pInstance, &msg);
    }
    me->clearFlag(me, GRAT_SING);
  }
	/*给MODBUS发消息更新传感器状态*/
  if (me->m_notifyHandle[EXT_UPDATE]->callback){
      me->m_notifyHandle[EXT_UPDATE]->callback(me->m_notifyHandle[EXT_UPDATE]->pInstance, &s_extSigState);
    }
}

/**
  * @brief	注册回调函数
  * @param	pInstance:一般指向注册回调函数的对象
  * @param  p:指向回调函数的指针
  * @param  type:回调函数类型
  * @retval	
  */
static void ExitSing_registerCallback(ExitSing * me, void * pInstance, const pExtSigCb p, ExtSigCbType type)
{
  me->m_notifyHandle[type]->callback = p;
  me->m_notifyHandle[type]->pInstance = pInstance;
}