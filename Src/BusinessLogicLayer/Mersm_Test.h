/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-03-09 10:11:53
 * @LastEditors: PanHan
 * @LastEditTime: 2020-04-06 10:52:50
 * @FilePath: \water_gcc\Src\BusinessLogicLayer\Mersm_Test.h
 */
#ifndef _MERSM_TEST_H
#define _MERSM_TEST_H

#include "FML_Daq.h"
#include "FML_DelayTime.h"
#include "FML_ExtProporV.h"
#include "FML_ExtSignal.h"
#include "FML_ExtValve.h"
#include "Mersm_LeakTest.h"


/*整个设备状态*/
typedef enum
{
	STATE_STOP = 0,      //待机状态
	STATE_CYKINDER_DOWN, //气缸下压
	STATE_13TOR,         //取13tor
	STATE_PRODUCT_TEST,  //产品检测
	STATE_AIR_PURGE,     //管路吹气
	STATE_AIR_PUMP,      //管路抽气
	STATE_IS_DRY,        //干燥判断
	STATE_WATER_IN,      //注水
	STATE_LOW_PRES_DETEC,//低压检测
	STATE_ADD_PRES,      //加压
	STATE_HIGH_PRES_DETEC, //高压检测
	STATE_WATER_EXH,      //排水
	STATE_WATER_PUMP,     //抽水
	STATE_CYKINDER_UP     //气缸上抬
}TestStateType_t; 

typedef struct _Mersm_Test Mersm_Test;
struct _Mersm_Test
{
  void (* init)(Mersm_Test * const me);                       //初始化
  TestStateType_t (* getCurrentState)(Mersm_Test * const me); //获取当前状态
  /*多事件接收器，单例，故不需要指向对象自身的指针*/
  void (* evInit)(void);          //初始化事件
  void (* evStart)(void);         //启动事件
  void (* evStop)(void);          //停止事件
  void (* evDelayOver)(void);     //延时结束
	void (* evLiqSwitch)(u8 type);  //液位报警
	void (* evCylPosH)(u8 type);    //气缸上限位传感器
  void (* evCylPosL)(u8 type);    //气缸下限位传感器
  void (* evGrat)(u8 type);       //光栅
  void (* evTimeOut)(void);       //超时

	ValveIODev * m_pValve;            //阀门设备类
	ProporValDev * m_pProporV;        //比例阀
	DataSever * m_pAD;                //数据采集设备
	ExitSing * m_pExitSig;            //外部信号
  FML_DelayTime * pDelayT;          //延时定时器
  FML_DelayTime * pTimeoutT;        //超时定时器
  
  Mersm_Leak * its_pSubLowPresLeak;     //低压测试子状态机
  Mersm_Leak * its_pSubHighPresLeak;    //高压测试子状态机

  ParaPres_t * m_pPresPara;   //压力参数
  ParaTime_t * m_pTimePara;   //时间参数

  void * private_data;  //私有数据
};
Mersm_Test * Mersm_Test_create(void);

#endif