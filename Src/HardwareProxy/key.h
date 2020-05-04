/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-01-14 11:23:10
 * @LastEditors: PanHan
 * @LastEditTime: 2020-04-14 09:14:07
 * @FilePath: \water_gcc\Src\HardwareProxy\key.h
 */
#ifndef _KEY_H
#define _KEY_H
#include "sys.h"

#ifdef __cplusplus
       extern "C" {
#endif


/*外部开关型信号量定义*/
typedef enum
{
	WATER_DECT_SING = 0,  //液位开关
	VACUUM_SING,          //真空传感器
	AIR_PRES_SING,         //压力传感器
	H_CLY_POS_SING,           //气缸位置
	L_CLY_POS_SING,           //气缸位置
	GRAT_SING,             //光栅信号
  SING_MAX              //传感器信号统计
}ExitSing_t;

//传感器触发方式
typedef enum
{
  LOW_LEVEL_TRIG,    //低电平触发
  HIGH_LEVEL_TRIG  //高电平触发
}VolTrigType;

//传感器状态
typedef enum
{
  TRIG_OFF,   //未触发
  TRIG_ON,    //触发
}SensorState;

//开关传感器
typedef struct _SwitchSensor SwitchSensor;
struct _SwitchSensor
{
  void (* init)(SwitchSensor * me);
  u8 (* singDetect)(SwitchSensor * me, SensorState * state); //信号检测
  void (* setTrigCountLimit)(SwitchSensor * me, u32 trig_on, u32 trig_off);//设置触发计数
  void (* setTrigType)(SwitchSensor * me, VolTrigType type);//设置触发电平
  
  void * private_data;  //私有数据
};
SwitchSensor * SwitchSensor_create(ExitSing_t type);

void extIO_Init(void);         
#ifdef __cplusplus
        }
#endif
        
#endif
