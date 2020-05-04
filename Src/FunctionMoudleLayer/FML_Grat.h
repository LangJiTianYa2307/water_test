/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-04-14 10:33:48
 * @LastEditors: PanHan
 * @LastEditTime: 2020-04-14 14:11:03
 * @FilePath: \water_gcc\Src\FunctionMoudleLayer\FML_Grat.h
 */
#ifndef _FML_GRAT_H
#define _FML_GRAT_H

#include "TypeDef.h"
#include "CommonType.h"
#include "FML_ExtValve.h"

typedef struct _GratDev GratDev;
struct _GratDev
{
  void (* init)(GratDev * me);
  void (* trigOn)(GratDev * me, ValveIODev * const pValveIODev);  //光栅触发
  void (* trigOff)(GratDev * me, ValveIODev * const pValveIODev); //光栅解除触发
  void (* updateCylPos)(GratDev * me, u8 hCyl, u8 lCyl);  //更新气缸状态
  u8 (* isTrig)(GratDev * me);  //光栅是否触发

  void * private_data;
};
GratDev * GratDev_create(void);

#endif