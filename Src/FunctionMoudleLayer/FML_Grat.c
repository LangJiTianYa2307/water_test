/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-04-14 10:33:58
 * @LastEditors: PanHan
 * @LastEditTime: 2020-04-14 14:10:07
 * @FilePath: \water_gcc\Src\FunctionMoudleLayer\FML_Grat.c
 */
#include "FML_Grat.h"

typedef struct
{
  u8 m_hCylPos;   //上气缸状态
  u8 m_lCylPos;   //下气缸状态
  u8 m_isTrig;    //光栅触发状态，0未触发，1已触发
}GratDevData;

static void init(GratDev * me);
static void trigOn(GratDev * me, ValveIODev * const pValveIODev);  //光栅触发
static void trigOff(GratDev * me, ValveIODev * const pValveIODev); //光栅解除触发
static void updateCylPos(GratDev * me, u8 hCyl, u8 lCyl);  //更新气缸状态
static u8 isTrig(GratDev * me);

static GratDev s_GratDev;
static GratDevData s_GratDevData;

GratDev * GratDev_create(void)
{
  static GratDev * me = 0;
  if (me == 0){
    me = & s_GratDev;
    me->init = init;
    me->trigOn = trigOn;
    me->trigOff = trigOff;
    me->updateCylPos = updateCylPos;
    me->isTrig = isTrig;

    me->private_data = & s_GratDevData;
    me->init(me);
  }
  return me;
}

static void init(GratDev * me)
{
  GratDevData * privateData = (GratDevData *)me->private_data;
  privateData->m_hCylPos = 0;
  privateData->m_lCylPos = 0;
  privateData->m_isTrig = 0;
}

/**
 * @brief	光栅触发，会关闭所有气缸并且保存当前状态
 * @param	pValveIODev:指向阀门管理器对象的指针
 * @retval	
 */
static void trigOn(GratDev * me, ValveIODev * const pValveIODev)
{
  GratDevData * privateData = (GratDevData *)me->private_data;
  privateData->m_hCylPos = pValveIODev->getVal(pValveIODev, CYL_H_V);
  privateData->m_lCylPos = pValveIODev->getVal(pValveIODev, CYL_L_V);
  pValveIODev->setVal(pValveIODev, CYL_H_V, 0);
  pValveIODev->setVal(pValveIODev, CYL_L_V, 0);

  privateData->m_isTrig = 1;
}

/**
 * @brief	光栅解除触发，会将气缸位置设置为所保存的状态
 * @param	pValveIODev:指向阀门管理器对象的指针
 * @retval	
 */
static void trigOff(GratDev * me, ValveIODev * const pValveIODev)
{
  GratDevData * privateData = (GratDevData *)me->private_data;

  privateData->m_isTrig = 0;
  
  pValveIODev->setVal(pValveIODev, CYL_H_V, privateData->m_hCylPos);
  pValveIODev->setVal(pValveIODev, CYL_L_V, privateData->m_lCylPos);

  privateData->m_hCylPos = 0;
  privateData->m_lCylPos = 0;
}

/**
 * @brief	更新气缸位置，只有在光栅触发的时候才会更新
 * @param	pValveIODev:指向阀门管理器对象的指针
 * @retval	
 */
static void updateCylPos(GratDev * me, u8 hCyl, u8 lCyl)
{
  GratDevData * privateData = (GratDevData *)me->private_data;

  privateData->m_hCylPos = hCyl;
  privateData->m_lCylPos = lCyl;
}

static u8 isTrig(GratDev * me)
{
  GratDevData * privateData = (GratDevData *)me->private_data;
  return privateData->m_isTrig;
}