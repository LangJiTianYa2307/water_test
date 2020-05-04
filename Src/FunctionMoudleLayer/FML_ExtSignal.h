/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-03-05 16:00:36
 * @LastEditors: PanHan
 * @LastEditTime: 2020-03-25 14:15:03
 * @FilePath: \water_gcc\Src\FunctionMoudleLayer\FML_ExtSignal.h
 */
#ifndef _FML_EXT_SIG_H
#define _FML_EXT_SIG_H

#include "TypeDef.h"
#include "CommonType.h"
#include "key.h"

#ifdef __cplusplus
       extern "C" {
#endif



//回调函数类型
typedef enum
{
  EXT_SIG_CHANGE,   //信号有变化
  EXT_UPDATE,       //信号状态显示
  EXT_MAX
}ExtSigCbType;

typedef  void (* pExtSigCb)(void *, void *); //回调函数

typedef struct _ExitSing ExitSing;

struct _ExitSing
{
	void (* init)(ExitSing * const me);  //初始化
	void (* singDetect)(ExitSing * const me); //检测外部信号
	u8 (* getExitSing)(ExitSing * const me, ExitSing_t type);//获取指定信号量状态
	void (* clearFlag)(ExitSing * const me, ExitSing_t type);//清除标志位
  void (* registerCallback)(ExitSing * me, void * pInstance, const pExtSigCb p, ExtSigCbType type); //注册回调函数
	
	u8 * m_pStateArr;     //指向保存信号状态是的数组
	u32 m_changeFlag;      //状态是否改变的标志位
	u32 m_maskBit;        //屏蔽位

  SwitchSensor * pSwitchSensorArr[SING_MAX];

  NotifyHandle * m_notifyHandle[EXT_MAX]; //指向所有通知句柄的指针数组

  u8 m_isInitFlag;  //初始化标志位
};
ExitSing * exitSingCreate(void);


#ifdef __cplusplus
        }
#endif
#endif
