/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-03-05 22:18:52
 * @LastEditors: PanHan
 * @LastEditTime: 2020-04-03 09:17:10
 * @FilePath: \water_gcc\Inc\CommonType.h
 */
#ifndef _COMMON_TYPE_H
#define _COMMON_TYPE_H

#include "TypeDef.h"

/*不同线程之间传递数据所使用的结构体，
  其中数据类型需要通信双方自行定义
*/
typedef struct _APP_MESSAGE
{
  u32 dataType;       //数据类型（数据类型堆通信双方都应该是透明的）
  void * pVoid;       //数据指针
}AppMesage;

typedef  void (* pGeneralCb)(void *, void *); //通用回调函数原型回调函数

typedef struct{
  pGeneralCb callback;    //回调函数
  void * pInstance;       //回调函数的使用对象
}NotifyHandle;

typedef struct _ParaTime ParaTime_t;
typedef struct _ParaPres ParaPres_t;
typedef struct _ParaSetting ParaSetting_t;
struct _ParaTime
{
	float tor13;          //取13Tor时间
	float productDetc;    //产品有无检测
	float PipeAirPurge;   //管路吹气时间
	float PipeAirPump;    //管路抽气时间
	float pipeIsDry;      //管路干燥时间
	float AirPump;        //抽气
	float balance;        //平衡
	float measure;        //测试
	float measDry;        //干燥
	float addPres;        //加压
	float waterDrain;     //排水
	float waterPump;      //抽水
	float waterInTimeout; //注水超时
	float addPresTimeout; //加压超时
	float reserve_1;        //保留1
	float reserve_2;        //保留2
	float reserve_3;        //保留3
	float reserve_4;        //保留4
	float reserve_5;        //保留5
	float reserve_6;        //保留6
	float reserve_7;        //保留7
	float reserve_8;        //保留8
};
struct _ParaPres
{
	float lowLeakRate;      //低压泄漏量
	float highLeakRate;     //高压泄漏量
	float dryLimit;         //干燥判定限
	float lowWaterMax;      //微漏水压上限
	float lowWaterMin;      //微漏水压下限
	float highWaterMax;     //大漏水压上限
	float highWaterMin;     //大漏水压下限
	float compensitionV_1;  //#1补偿
	float compensitionV_2;  //#2补偿
	float compensitionV_3;  //#3补偿
	float compensitionV_4;  //#4补偿
	float lowWaterOpening;  //低压电子比例阀开度
	float highWaterOpening; //高压电子比例阀开度
	float existLimit;       //产品存在判定限
	float lowerLimitL;        //低压下限
	float lowerLimitH;        //高压下限
	float reserve_6;        //保留6
	float reserve_7;        //保留7
	float reserve_8;        //保留8
	float reserve_9;        //保留9
};

struct _ParaSetting
{
	ParaTime_t paraTime;
	ParaPres_t paraPres;
};

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */

#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})


//双向链表
typedef struct _BsaeList BsaeList;
struct _BsaeList
{
  BsaeList * prev;
  BsaeList * next;
};
#endif