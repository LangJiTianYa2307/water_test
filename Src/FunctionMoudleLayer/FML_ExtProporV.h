/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-03-05 22:42:10
 * @LastEditors: PanHan
 * @LastEditTime: 2020-04-06 16:04:30
 * @FilePath: \water_gcc\Src\FunctionMoudleLayer\FML_ExtProporV.h
 */
#ifndef _FML_EXT_PROPOR_V_H
#define _FML_EXT_PROPOR_V_H

#include "CommonType.h"

typedef enum{
  CB_PRO_DISP,    //阀门开度显示
  CB_PRO_MAX      //回调函数最大值
}ProporValCbType;

typedef struct _ProporValDev ProporValDev;
struct _ProporValDev
{
	void (* init)(ProporValDev * const me);  //初始化
	void (* setOutputPres)(ProporValDev * const me, float pres);//设置输出压力
	void (* setMaxPres)(float pres);
	float (* getMaxPres)(void);
	void (* registerCallback)(ProporValDev * me, void * pInstance, const pGeneralCb p, ProporValCbType type); //注册回调函数
	float m_voltage; //输入电压
	float m_outPres; //输出压力
	float m_openPercent; //阀门开度百分比

  NotifyHandle * m_cbHandle[CB_PRO_MAX];
};
ProporValDev * proporValCreate(void);


#endif