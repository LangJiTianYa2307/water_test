/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-03-05 17:44:25
 * @LastEditors: PanHan
 * @LastEditTime: 2020-04-11 20:50:32
 * @FilePath: \water_gcc\Src\FunctionMoudleLayer\FML_ExtValve.h
 */
#ifndef _FML_EXT_VALVE_H
#define _FML_EXT_VALVE_H

#include "TypeDef.h"
#include "CommonType.h"
#include "led.h"
#ifdef __cplusplus
       extern "C" {
#endif

//回调函数类型
typedef enum
{
  CB_V_DISP,  //显示阀门状态
  CB_V_MAX
}ValveCbType;

typedef struct _ValveIODev ValveIODev;
struct _ValveIODev
{
	void (* init)(ValveIODev * const me);   //初始化电磁阀IO外设
	void (* setVal)(ValveIODev * const me, valveIO_t valve, u8 state); //设置阀门状态
	u8 (* getVal)(ValveIODev * const me, valveIO_t valve); //获取阀门状态
  void (* registerCallback)(ValveIODev * me, void * pInstance, const pGeneralCb p, ValveCbType type); //注册回调函数

  NotifyHandle * m_notifyHandle[CB_V_MAX]; //指向所有通知句柄的指针数组
  void * private_data;
};
ValveIODev * valveIODevCreate(void);

#ifdef __cplusplus
        }
#endif

#endif