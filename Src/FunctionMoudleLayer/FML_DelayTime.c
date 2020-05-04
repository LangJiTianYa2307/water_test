/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2019-10-28 17:00:50
 * @LastEditors: PanHan
 * @LastEditTime: 2020-03-14 13:42:07
 */

#include "FML_DelayTime.h"

/*----------------------------------------#defines--------------------------------------*/

/*--------------------------------------------------------------------------------------*/

/*----------------------------------------static function--------------------------------------*/
static void FML_DelayTime_init(FML_DelayTime * me);
static void FML_DelayTime_setDelayTime(FML_DelayTime * me, float time);
static void FML_DelayTime_stopTimer(FML_DelayTime * me);
static void FML_DelayTime_registerCallback(FML_DelayTime * me, void * pInstance, const pGeneralCb p, CallbackType type);
static void FML_DelayTime_unregisterCallback(FML_DelayTime * me, CallbackType type);//取消回调函数的注册
static void delayTimerCb(TimerHandle_t xTimer);
/*--------------------------------------------------------------------------------------*/

/*----------------------------------------static variable--------------------------------------*/
static FML_DelayTime fml_DelayTime[D_TIM_MAX];
static TimerHandle_t s_timerHandle[D_TIM_MAX];             //
static NotifyHandle s_notifyHandle[D_TIM_MAX][TIME_CB_MAX]; //通知句柄数组，用于保存回调函数和调用对象
/*--------------------------------------------------------------------------------------*/

/**
  * @brief   create object
  * @param   
  * @retval  
  */
FML_DelayTime * FML_DelayTime_Create(DelayTimType timType)
{
  FML_DelayTime * me = &fml_DelayTime[timType];   //指向对象
  int i = 0;
  if ((me->init) == 0){
    me->init = FML_DelayTime_init;
    me->setDelayTime = FML_DelayTime_setDelayTime;
    me->registerCallback = FML_DelayTime_registerCallback;
    me->unregisterCallback = FML_DelayTime_unregisterCallback;
    me->stopTimer = FML_DelayTime_stopTimer;
    me->m_isInitFlag = 0;
    me->m_type = timType;
    me->m_timerHandle = s_timerHandle[timType];
    //将静态分配的通知句柄地址传递给对象成员
    for (i = 0; i < TIME_CB_MAX; i ++){
      me->m_cbHandle[i] = & s_notifyHandle[timType][i];
    }

    me->init(me);
  }
  return me;
}

/**
  * @brief   initialzation
  * @param   
  * @retval  
  */
static void FML_DelayTime_init(FML_DelayTime * me)
{
  int i = 0;
  if (me->m_isInitFlag == 0){
    me->m_timerHandle = xTimerCreate((const char*    )"extCb",
                                    (TickType_t     )10,
                                    (UBaseType_t    )pdTRUE,
                                    (void *         )me,
                                    (TimerCallbackFunction_t)delayTimerCb);
    me->m_isInitFlag = 1;
  }
}

/**
  * @brief   setting delay time
  * @param   time:time need to delay
  * @retval  
  */
static void FML_DelayTime_setDelayTime(FML_DelayTime * me, float time)
{
  me->m_delayTime = time;
  taskYIELD();  //开启一次任务调度
  if (pdFALSE == xTimerIsTimerActive(me->m_timerHandle)){
    xTimerStart(me->m_timerHandle, 10);
    me->m_nowTime = 0;
  }else{
    MY_ASSERT(0);  
  }
  
}

/**
  * @brief   stop the soft timer
  * @param   
  * @retval  
  */
static void FML_DelayTime_stopTimer(FML_DelayTime * me)
{
  taskYIELD();  //开启一次任务调度
  if (pdFALSE != xTimerIsTimerActive(me->m_timerHandle)){
    xTimerStop(me->m_timerHandle, 10);
  }
}

/**
  * @brief   register callback
  * @param   
  * @retval  
  */
static void FML_DelayTime_registerCallback(FML_DelayTime * me, void * pInstance, const pGeneralCb p, CallbackType type)
{
  me->m_cbHandle[type]->callback = p;
  me->m_cbHandle[type]->pInstance = pInstance;
}

/**
 * @brief	取消回调函数的注册
 * @param	type:回调函数类型
 * @retval	
 */
static void FML_DelayTime_unregisterCallback(FML_DelayTime * me, CallbackType type)
{
  me->m_cbHandle[type]->callback = 0;
  me->m_cbHandle[type]->pInstance = 0;
}

/**
  * @brief   soft timer callback function
  * @param   
  * @retval  
  */
static void delayTimerCb(TimerHandle_t xTimer)
{
  FML_DelayTime * me = (FML_DelayTime *)pvTimerGetTimerID(xTimer);
  me->m_nowTime += 0.01;
  float dispTime = 0;
  if (me->m_nowTime >= me->m_delayTime){
    /*计时事件大于或等于设定时间，则停止该定时器并且执行相关的回调函数*/
    xTimerStop(xTimer, 0);
    if ((me->m_cbHandle[TIME_OVER]->callback != 0)&&(me->m_cbHandle[TIME_OVER]->pInstance != 0)){
      dispTime = 0;
      me->m_cbHandle[TIME_OVER]->callback(me->m_cbHandle[TIME_OVER]->pInstance, 0);
    }
    
    if ((me->m_cbHandle[TIME_DISP]->callback != 0)&&(me->m_cbHandle[TIME_DISP]->pInstance != 0))  \
        me->m_cbHandle[TIME_DISP]->callback(me->m_cbHandle[TIME_DISP]->pInstance, & dispTime);
  }
  else{
    /*倒计时过程中，执行相关的回调函数*/
    if ((me->m_cbHandle[TIME_DISP]->callback != 0)&&(me->m_cbHandle[TIME_DISP]->pInstance != 0)){
      dispTime = (me->m_delayTime - me->m_nowTime);
      me->m_cbHandle[TIME_DISP]->callback(me->m_cbHandle[TIME_DISP]->pInstance, &dispTime);
    }
        
    if ((me->m_cbHandle[TIME_REACT]->callback != 0)&&(me->m_cbHandle[TIME_REACT]->pInstance != 0))  \
        me->m_cbHandle[TIME_REACT]->callback(me->m_cbHandle[TIME_REACT]->pInstance, 0);
  }
}