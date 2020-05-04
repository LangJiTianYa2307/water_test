/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-03-08 14:34:02
 * @LastEditors: PanHan
 * @LastEditTime: 2020-04-13 19:52:06
 * @FilePath: \water_gcc\Src\BusinessLogicLayer\Mersm_Debug.h
 */
#ifndef _MERSM_DEBUG_H
#define _MERSM_DEBUG_H

#include "FML_DelayTime.h"
#include "FML_ExtProporV.h"
#include "FML_ExtSignal.h"
#include "FML_ExtValve.h"

/*DEBUG状态下的子状态*/
typedef enum
{
	DEBUG_OFF = 0,         //待机
  DEBUG_ON,              //调试中
  DEBUG_CYL_UP           //气缸上台
}DebugState_t;

typedef struct _Mersm_Debug Mersm_Debug;
struct _Mersm_Debug
{
  void (* init)(Mersm_Debug * const me);                       //初始化
  
  /*多事件接收器，单例，故不需要指向对象自身的指针*/
  void (* evInit)(void);          //初始化事件
  void (* evStart)(void); //启动事件
  void (* evStop)(void);  //停止事件
	void (* evCylPosH)(u8 type);    //气缸上限位传感器
  void (* evGrat)(u8 type);       //光栅
  void (* evSetProporV)(void * pData);//设置比例阀开度
  void (* evSetSingleV)(void * pData);//设置单个阀门状态
  void (* evTimeout)(void); //超时

  DebugState_t m_stateID;

  ValveIODev * m_pValve;                                  //阀门设备类
  ProporValDev * m_pProporV;                                //比例阀
	ExitSing * m_pExitSig;                                  //外部信号
  FML_DelayTime * pTimeoutT;        //超时定时器

  u8 m_isInitFlag;  //初始化标志位
};

Mersm_Debug * mersm_Debug_create(void);
#endif