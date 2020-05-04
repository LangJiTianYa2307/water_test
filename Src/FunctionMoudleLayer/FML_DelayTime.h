/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2019-10-28 17:00:42
 * @LastEditors: PanHan
 * @LastEditTime: 2020-03-09 15:22:17
 */
#ifndef _FML_DELAY_TIME_H
#define _FML_DELAY_TIME_H
#include "sys.h"
#include "CommonType.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"


typedef enum
{
  D_TIM_1,    
  D_TIM_2,    
  D_TIM_3,
  D_TIM_MAX
}DelayTimType;

//回调函数类型
typedef enum
{
  TIME_DISP,    //数据显示
  TIME_OVER,    //时间结束
  TIME_REACT,   //状态动作
  TIME_CB_MAX
}CallbackType;


typedef struct _FML_DelayTime FML_DelayTime;
struct _FML_DelayTime
{
  void (* init)(FML_DelayTime * me);
  void (* setDelayTime)(FML_DelayTime * me, float time);
  void (* stopTimer)(FML_DelayTime * me);
  void (* registerCallback)(FML_DelayTime * me, void * pInstance, const pGeneralCb p, CallbackType type);
  void (* unregisterCallback)(FML_DelayTime * me, CallbackType type);//取消回调函数的注册
  float m_delayTime;
  float m_nowTime;
  DelayTimType m_type;

  NotifyHandle * m_cbHandle[TIME_CB_MAX]; //保存指向回调函数句柄的指针
  TimerHandle_t m_timerHandle;
  u8 m_isInitFlag;                  //0未初始化，1已经初始化
};
FML_DelayTime * FML_DelayTime_Create(DelayTimType timType);

#endif