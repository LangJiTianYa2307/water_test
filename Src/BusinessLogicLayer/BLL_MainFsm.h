/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-03-09 10:07:26
 * @LastEditors: PanHan
 * @LastEditTime: 2020-04-06 10:58:36
 * @FilePath: \water_gcc\Src\BusinessLogicLayer\BLL_MainFsm.h
 */
#ifndef _BLL_MAIN_FSM_H
#define _BLL_MAIN_FSM_H
#include "FML_Daq.h"
#include "FML_DelayTime.h"
#include "FML_ExtProporV.h"
#include "FML_ExtSignal.h"
#include "FML_ExtValve.h"

#include "Mersm_Debug.h"
#include "Mersm_Dry.h"
#include "Mersm_Test.h"

#define TIME_DISP_TIM D_TIM_1   //用于显示倒计时的定时器
#define TIME_OUT_TIM  D_TIM_2   //用于进行超时计时的定时器

/*待机状态下的设备状态*/
typedef enum
{
	STOP_TEST = 0,        //测试
	STOP_DRY,             //干燥
	STOP_WATER_OUT,       //排水
  STOP_DEBUG,           //调试
}MainState_t;

/*定义可以使状态机状态改变的事件类型*/
typedef enum
{
	EV_START = 0,      //开始信号
	EV_STOP,           //停止信号
	EV_DELAY_OVER,     //下一步
  EV_LIQ_SWITCH,     //液位报警
	EV_VACUUM,         //真空度不足报警信号
	EV_CYL_POS_H,      //气缸位置
	EV_CYL_POS_L,
	EV_GRAT,           //光栅
	EV_DRY_MODE,       //干燥模式
  EV_DEBUG_MODE,     //调试模式
	EV_TIME_OUT,       //超时
	EV_SET_PARA,       //设置参数
  EV_SET_PROPOR,     //设置比例阀开度
  EV_SET_SIG_V       //设置单个阀门
}EventType_t;

typedef struct _MainFsm_DEV MainFsmDev;
struct _MainFsm_DEV
{
	void (* init)(MainFsmDev * const me);                       //初始化
  void (* sendEvent)(const AppMesage * msg);  //发送事件
	
	MainState_t m_MainSteteID;     //主状态
  Mersm_Dry * its_pSubDry;          //干燥子状态
  Mersm_Debug * its_pSubDebug;      //调试子状态
  Mersm_Test * its_pSubTest;        //测试子状态
  
  u8 m_isInitFlag;  //初始化标志位
};
MainFsmDev * MainFsmDev_create(void);

#endif