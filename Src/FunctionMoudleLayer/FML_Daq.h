/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-02-28 15:37:39
 * @LastEditors: PanHan
 * @LastEditTime: 2020-03-03 09:40:11
 * @FilePath: \water_gcc\Src\FunctionMoudleLayer\FML_Daq.h
 */
#ifndef _FML_DAQ_H
#define _FML_DAQ_H

#include "TypeDef.h"


//压力数据的类型
typedef enum
{ 
  PUMP_PRES_DATA = 0, //抽气阶段的平均压力（10s）
  TEST_PRES_DATA,     //检测阶段的平均压力（1.2s）
  CUR_VACUM_DATA,     //实时气压
  CUR_WATER_DATA,     //实时水压
  PRO_DEC_WATER       //产品检测阶段的水压
}PresDataType;

typedef struct _DataSever DataSever;
struct _DataSever
{
	void (* init)(DataSever * const me);   //初始化
  float (* getPresData)(DataSever * const me, PresDataType dataType); //读取压力数据
	
	float m_measurePres;   //测试腔压力
	float m_waterPres;     //注水腔压力
	
};


DataSever * DataSever_create(void);
#endif