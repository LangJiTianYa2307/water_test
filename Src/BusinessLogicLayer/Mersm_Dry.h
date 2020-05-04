/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-03-08 13:26:30
 * @LastEditors: PanHan
 * @LastEditTime: 2020-04-11 14:47:03
 * @FilePath: \water_gcc\Src\BusinessLogicLayer\Mersm_Dry.h
 */
#ifndef _MERSM_DRY_H
#define _MERSM_DRY_H

#include "FML_Daq.h"
#include "FML_DelayTime.h"
#include "FML_ExtProporV.h"
#include "FML_ExtSignal.h"
#include "FML_ExtValve.h"

/*干燥状态下的子状态*/
typedef enum
{
	DRY_NULL = 0,         //待机
	DRY_CYL_DOWN,         //气缸压下
	DRY_CLY_UP,           //气缸抬起
  DRY_AIR_PUGE,         //吹气
	DRY_AIR_PUMP,         //抽气
  DRY_LEAK              //漏气检测
}DryState_t;

typedef struct _Mersm_Dry Mersm_Dry;
struct _Mersm_Dry
{
  void (* init)(Mersm_Dry * const me);                       //初始化
  /*多事件接收器，单例，故不需要指向对象自身的指针*/
  void (* evInit)(void);  //进入默认伪状态的事件
  void (* evStart)(void); //启动事件
  void (* evStop)(void);  //停止事件
  void (* evDelayOver)(void); //延时结束
  void (* evVacuum)(u8 type);  //真空度报警信号
	void (* evCylPosH)(u8 type);    //气缸上限位传感器
  void (* evCylPosL)(u8 type);    //气缸下限位传感器
  void (* evGrat)(u8 type);       //光栅
  void (* evTimeOut)(void);       //超时

  DryState_t m_stateID;
  ParaSetting_t * m_pParaSetting; //参数

  ValveIODev * m_pValve;                                  //阀门设备类
	DataSever * m_pAD;                                      //数据采集设备
	ExitSing * m_pExitSig;                                  //外部信号
  
  FML_DelayTime * m_delay;        //倒计时定时器
  FML_DelayTime * m_timeOut;      //超时定时器

  SemaphoreHandle_t m_mutex;    //互斥信号量
  u8 m_isInitFlag;  //初始化标志位
};

Mersm_Dry * mersm_Dry_create(void);
#endif