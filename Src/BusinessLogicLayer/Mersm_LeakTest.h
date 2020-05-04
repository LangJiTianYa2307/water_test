/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-03-08 16:34:52
 * @LastEditors: PanHan
 * @LastEditTime: 2020-04-06 13:24:59
 * @FilePath: \water_gcc\Src\BusinessLogicLayer\Mersm_LeakTest.h
 */
#ifndef _MERSM_LOW_LEAK_H
#define _MERSM_LOW_LEAK_H

#include "FML_DelayTime.h"
#include "FML_ExtProporV.h"
#include "FML_ExtSignal.h"
#include "FML_ExtValve.h"
#include "FML_Daq.h"

//泄漏测试子流程中回调函数类型
typedef enum
{
  CB_LEAK_OVER, //泄漏状态结束
  CB_LEAK_MAX   //回调函数类型计数
}LeakCbType;

typedef enum
{
  LEAK_LOW_PRES,    //低压泄泄漏测试
  LEAK_HIGH_PRES    //高压泄漏测试
}LeakTestType;

/*测试过程状态*/
typedef enum
{
	SUB_NULL = 0,        //待机状态
	SUB_AIR_PUMP,        //抽气
	SUB_BALANCE,         //平衡
	SUB_MEAS_1,          //测试1
	SUB_DRY_1,           //干燥
	SUB_MEAS_2,          //测试2
	SUB_DRY_2,           //干燥
	SUB_MEAS_3,          //测试3
	SUB_DRY_3,           //干燥
	SUB_MEAS_4,          //测试4
  SUB_MAX              //状态计数  
}LeakStateType_t;

typedef struct _Mersm_Leak Mersm_Leak;
struct _Mersm_Leak
{
  void (* init)(Mersm_Leak * const me);                       //初始化
  /*多事件接收器，单例，故不需要指向对象自身的指针*/
  void (* evInit)(Mersm_Leak * const me);          //初始化事件
  void (* evStart)(Mersm_Leak * const me); //启动事件
  void (* evStop)(Mersm_Leak * const me);  //停止事件
  void (* evDelayOver)(Mersm_Leak * const me); //延时结束

  LeakStateType_t (* getCurrentState)(Mersm_Leak * const me); //获取当前状态
  u32 (* getResult)(Mersm_Leak * const me);                   //获取测试结果
  void (* clearPublicResult)(void); //清除所有对象共有的测试结果
  void (* addNgCount)(u8 port);
  void (* clearNgCount)(u8 port);
  void (* clearAllNgCount)(void);
  
  ValveIODev * m_pValve;      //阀门设备类
	DataSever * m_pAD;          //数据采集设备
  FML_DelayTime * m_pDelay;   //延时定时器对象,由父状态传递指针
  ParaPres_t * m_pPresPara;   //压力参数
  ParaTime_t * m_pTimePara;   //时间参数

  void * private_data;  //私有数据
};
Mersm_Leak * Mersm_Leak_create(LeakTestType leakType);

#endif
