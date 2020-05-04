/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-03-08 13:26:46
 * @LastEditors: PanHan
 * @LastEditTime: 2020-04-15 10:15:50
 * @FilePath: \water_gcc\Src\BusinessLogicLayer\Mersm_Dry.c
 */
#include "Mersm_Dry.h"
#include "BLL_Alarm.h"
#include "BLL_Modbus.h"
#include "APP.h"
/*----------------------------------------#defines--------------------------------------*/
#define LEAK_TIME_SEC 10    //泄漏检测的时间
#define LEAK_PA_PER_SEC 12  //泄漏量，单位pa/s
#define DRY_COUNT_MAX 50    //最大干燥次数，超出则报错
/*--------------------------------------------------------------------------------------*/

/*----------------------------------------static function-------------------------------*/
static void Mersm_init(Mersm_Dry * const me);                       //初始化
static void Mersm_gratCylCtrl(Mersm_Dry * const me);                //光栅检测控制气缸
/*多事件接收器，单例，故不需要指向对象自身的指针*/
static void Mersm_evInit(void); //初始化事件
static void Mersm_evStart(void); //启动事件
static void Mersm_evStop(void);  //停止事件
static void Mersm_evDelayOver(void); //延时结束
static void Mersm_evVacuum(u8 type);  //真空度报警信号
static void Mersm_evCylPosH(u8 type);    //气缸上限位传感器
static void Mersm_evCylPosL(u8 type);    //气缸下限位传感器
static void Mersm_evGrat(u8 type);       //光栅
static void Mersm_evTimeOut(void);       //超时

static void enterDRY_NULL(Mersm_Dry * const me);
static void enterDRY_CYL_DOWN(Mersm_Dry * const me);
static void enterDRY_CLY_UP(Mersm_Dry * const me);
static void enterDRY_AIR_PUGE(Mersm_Dry * const me);
static void enterDRY_AIR_PUMP(Mersm_Dry * const me);
static void enterDRY_LEAK(Mersm_Dry * const me);


static void exitDRY_NULL(Mersm_Dry * const me);
static void exitDRY_CYL_DOWN(Mersm_Dry * const me);
static void exitDRY_CLY_UP(Mersm_Dry * const me);
static void exitDRY_AIR_PUGE(Mersm_Dry * const me);
static void exitDRY_AIR_PUMP(Mersm_Dry * const me);
static void exitDRY_LEAK(Mersm_Dry * const me);
/*--------------------------------------------------------------------------------------*/

/*----------------------------------------static variable-------------------------------*/
static Mersm_Dry s_mersm_DryDev;
static u8 s_hCylLockState;    //上气缸锁定时的状态
static u8 s_lCylLockState;    //下气缸锁定时的状态
static float s_leakPresRef;   //泄漏参考值
static u32 s_dryCnt;  //干燥计数
/*--------------------------------------------------------------------------------------*/
/**
 * @brief	创建干燥状态机对象
 * @param	
 * @retval	
 */
Mersm_Dry * mersm_Dry_create(void)
{
  static Mersm_Dry * me = 0;
  if (me == 0){
    me = &s_mersm_DryDev;

    me->init = Mersm_init;
    me->evInit = Mersm_evInit;
    me->evStart = Mersm_evStart;
    me->evStop = Mersm_evStop;
    me->evDelayOver = Mersm_evDelayOver;
    me->evVacuum = Mersm_evVacuum;
    me->evCylPosH = Mersm_evCylPosH;
    me->evCylPosL = Mersm_evCylPosL;
    me->evGrat = Mersm_evGrat;
    me->evTimeOut = Mersm_evTimeOut;

    me->m_isInitFlag = 0;
  }
  return me;
}

/**
 * @brief	干燥状态机对象初始化
 * @param	
 * @retval	
 */
static void Mersm_init(Mersm_Dry * const me)
{
  if (me->m_isInitFlag == 0){
    me->m_mutex = xSemaphoreCreateMutex();  //创建互斥信号量
    me->m_isInitFlag = 1;
  }
}

/**
 * @brief	初始化事件，用来进入默认伪状态
 * @param	
 * @retval	
 */
static void Mersm_evInit(void)
{
  Mersm_Dry * me = &s_mersm_DryDev;
  FlagHand * pErr = errHandCreate();  //错误处理对象
  xSemaphoreTake(me->m_mutex, portMAX_DELAY); //锁定
  pErr->clearErrBit(ERR_NEED_DRY);  //进入干燥状态则清除“需要干燥”信息
  me->m_stateID = DRY_NULL;
  enterDRY_NULL(me);
  xSemaphoreGive(me->m_mutex);  //解锁
}

/**
 * @brief	接收到“启动”事件信号
 * @param	
 * @retval	
 */
static void Mersm_evStart(void)
{
  Mersm_Dry * me = &s_mersm_DryDev;
  FlagHand * pErr = errHandCreate();  //错误处理对象
  xSemaphoreTake(me->m_mutex, portMAX_DELAY);
  //只有处于待机状态，才会相应启动信号
  if (DRY_NULL == me->m_stateID){
    exitDRY_NULL(me);
    pErr->setVbStatus(EX_RUN_S);  //已经进入干燥阶段，设备运行
    /*进入气缸下压*/
    me->m_stateID = DRY_CYL_DOWN;
    enterDRY_CYL_DOWN(me);
  }
  xSemaphoreGive(me->m_mutex);
}

/**
 * @brief	接收到“停止”事件信号，退出干燥模式
 * @param	
 * @retval	
 */
static void Mersm_evStop(void)
{
  Mersm_Dry * me = &s_mersm_DryDev;
  FlagHand * pErr = errHandCreate();  //错误处理对象
  xSemaphoreTake(me->m_mutex, portMAX_DELAY); //上锁
  /*否则根据所处的状态来执行不同的操作*/
  switch (me->m_stateID){
  case DRY_CYL_DOWN:
    exitDRY_CYL_DOWN(me);
    me->m_stateID = DRY_CLY_UP;
    enterDRY_CLY_UP(me);
    break;
  case DRY_AIR_PUGE:
    exitDRY_AIR_PUGE(me);
    me->m_stateID = DRY_CLY_UP;
    enterDRY_CLY_UP(me);
    break;
  case DRY_AIR_PUMP:
    exitDRY_AIR_PUMP(me);
    me->m_stateID = DRY_CLY_UP;
    enterDRY_CLY_UP(me);
    break;
  case DRY_LEAK:
    exitDRY_LEAK(me);
    me->m_stateID = DRY_CLY_UP;
    enterDRY_CLY_UP(me);
    break;
  case DRY_CLY_UP:
    //只有在光栅未触发的条件下，才会进入待机状态
    exitDRY_CLY_UP(me);
    me->m_stateID = DRY_NULL;
    enterDRY_NULL(me);
    break;
  default:
    break;
  }//end switch
  xSemaphoreGive(me->m_mutex);  //解锁
}

/**
 * @brief	延时信号处理
 * @param	
 * @retval	
 */
static void Mersm_evDelayOver(void)
{
  Mersm_Dry * me = &s_mersm_DryDev;
  float tempPres;
  FlagHand * pErr = errHandCreate();  //错误处理对象
  xSemaphoreTake(me->m_mutex, portMAX_DELAY); //上锁
  if (DRY_AIR_PUGE == me->m_stateID){
    /*吹气时间结束，就进入抽气*/
    exitDRY_AIR_PUGE(me);
    me->m_stateID = DRY_AIR_PUMP;
    enterDRY_AIR_PUMP(me);
  }
  else if (DRY_AIR_PUMP == me->m_stateID){
    tempPres = me->m_pAD->getPresData(me->m_pAD, TEST_PRES_DATA);
    exitDRY_AIR_PUMP(me);
    if (tempPres < me->m_pParaSetting->paraPres.dryLimit){
      /*初步干燥检测通过，则进入泄漏检测阶段*/
      s_leakPresRef = me->m_pAD->getPresData(me->m_pAD, TEST_PRES_DATA);  //setting pres reference
      me->m_stateID = DRY_LEAK;
      enterDRY_LEAK(me);
    }
    else{
      if (s_dryCnt >= 50){
        /*drrCnt>50,then stop*/
        pErr->setErrBit(ERR_DRY); //dry error
        me->m_stateID = DRY_CLY_UP;
        enterDRY_CLY_UP(me);
      }
      else{
        /*初步检测不通过则继续吹气*/
        me->m_stateID = DRY_AIR_PUGE;
        enterDRY_AIR_PUGE(me);
      }
      
    }	
  }
  else if (DRY_LEAK == me->m_stateID){
    exitDRY_LEAK(me);
    tempPres = me->m_pAD->getPresData(me->m_pAD, TEST_PRES_DATA) - s_leakPresRef;
    /*计时结束，通过计算压降来判断是否干燥成功*/
    if (LEAK_PA_PER_SEC > (tempPres / LEAK_TIME_SEC)){
      me->m_stateID = DRY_CLY_UP;
      enterDRY_CLY_UP(me);
    }
    else{
      me->m_stateID = DRY_AIR_PUGE;
      enterDRY_AIR_PUGE(me);
    }
  }
  xSemaphoreGive(me->m_mutex);  //解锁
}

/**
 * @brief	响应气缸上限位信号
 * @param	
 * @retval	
 */
static void Mersm_evCylPosH(u8 type)
{
  Mersm_Dry * me = &s_mersm_DryDev;
  FlagHand * pErr = errHandCreate();  //错误处理对象
  xSemaphoreTake(me->m_mutex, portMAX_DELAY); //上锁
  if ((DRY_CLY_UP == me->m_stateID) && (type == 1)){
    exitDRY_CLY_UP(me);
    me->m_stateID = DRY_NULL;
    enterDRY_NULL(me);
  }
  xSemaphoreGive(me->m_mutex);  //解锁
}

/**
 * @brief	响应气缸下限位信号
 * @param	
 * @retval	
 */
static void Mersm_evCylPosL(u8 type)
{
  Mersm_Dry * me = &s_mersm_DryDev;
  FlagHand * pErr = errHandCreate();  //错误处理对象
  xSemaphoreTake(me->m_mutex, portMAX_DELAY); //上锁
  if ((DRY_CYL_DOWN == me->m_stateID) && (type == 1)){
    exitDRY_CYL_DOWN(me);
    me->m_stateID = DRY_AIR_PUGE;
    enterDRY_AIR_PUGE(me);
    pErr->clearErrBit(ERR_CYL_L);  //此时正常流程，清除气缸错误
  }
  xSemaphoreGive(me->m_mutex);  //解锁
}

/**
 * @brief	暂时不用
 * @param	
 * @retval	
 */
static void Mersm_evVacuum(u8 type)
{
}
/**
 * @brief	光栅触发
 * @param	
 * @retval	
 */
static void Mersm_evGrat(u8 type)
{
  Mersm_Dry * me = &s_mersm_DryDev;
  FlagHand * pErr = errHandCreate();  //错误处理对象
  xSemaphoreTake(me->m_mutex, portMAX_DELAY); //上锁
  if (type){
    s_hCylLockState = me->m_pValve->getVal(me->m_pValve, CYL_H_V);
    s_lCylLockState = me->m_pValve->getVal(me->m_pValve, CYL_L_V);
    me->m_pValve->setVal(me->m_pValve, CYL_H_V, 0);
		me->m_pValve->setVal(me->m_pValve, CYL_L_V, 0);
		pErr->setErrBit(ERR_GRAT);//光栅报警
  }
  else{
    me->m_pValve->setVal(me->m_pValve, CYL_H_V, s_hCylLockState);
		me->m_pValve->setVal(me->m_pValve, CYL_L_V, s_lCylLockState);
    pErr->clearErrBit(ERR_GRAT);//清除光栅报警
  }
  xSemaphoreGive(me->m_mutex);  //解锁
}

/**
 * @brief	检测光栅是否触发，涉及气缸动作的流程均需要调用该函数
 * @param	me:指向状态机对象的指针
 * @retval	
 */
static void Mersm_gratCylCtrl(Mersm_Dry * const me)
{
  // FlagHand * pErr = errHandCreate();  //错误处理对象
	// /*气缸动作时，检测到光栅报警信号，需要将当前气缸控制电磁阀的状态进行保存，对其进行断电
	// 	,后续报警结束，会自动根据所保存的状态来恢复气缸状态*/
	// if (me->m_pExitSig->getExitSing(me->m_pExitSig, GRAT_SING)){
	// 	s_hCylLockState = me->m_pValve->getVal(me->m_pValve, CYL_H_V);
	// 	s_lCylLockState = me->m_pValve->getVal(me->m_pValve, CYL_L_V);
	// 	me->m_pValve->setVal(me->m_pValve, CYL_H_V, 0);
	// 	me->m_pValve->setVal(me->m_pValve, CYL_L_V, 0);
	// 	pErr->setErrBit(ERR_GRAT);//光栅报警
	// }
  // else{
	// 	pErr->clearErrBit(ERR_GRAT);//清除光栅报警
	// 	s_hCylLockState = 0;
	// 	s_lCylLockState = 0;
	// }
}
/**
 * @brief	超时事件处理
 * @param	
 * @retval	
 */
static void Mersm_evTimeOut(void)
{
  Mersm_Dry * me = &s_mersm_DryDev;
  FlagHand * pErr = errHandCreate();  //错误处理对象
  xSemaphoreTake(me->m_mutex, portMAX_DELAY); //上锁
  if (DRY_CYL_DOWN == me->m_stateID){
    pErr->setErrBit(ERR_CYL_L);//气缸未到位异常
    exitDRY_CYL_DOWN(me);
    me->m_stateID = DRY_CLY_UP;
    enterDRY_CLY_UP(me);
  }
  else if (DRY_CLY_UP == me->m_stateID){
    pErr->setErrBit(ERR_CYL_H);//气缸未退回异常
    exitDRY_CLY_UP(me);
    me->m_stateID = DRY_NULL;
    enterDRY_NULL(me);
  }
  xSemaphoreGive(me->m_mutex);  //解锁
}

/*
*函数功能：有限状态机进入干燥状态下的待机子状态
*/	
static void enterDRY_NULL(Mersm_Dry * const me)
{
  s_dryCnt = 0;
	printf("enterDRY_NULL\r\n");
}
	
/*
*函数功能：有限状态机进入干燥状态下的气缸下压子状态
*/
static void enterDRY_CYL_DOWN(Mersm_Dry * const me)
{
	int i = 0;
	AppMesage msg;
	for (i = WATER_IN_V; i < MAX_NUM_V; i ++){
		/*气缸下压状态，大真空、微真空阀和平衡阀打开，上气缸进气打开，其他阀门关闭*/
		if ((i == BIG_VACUUM_V) || \
			  (i == LIT_VACUUM_V) || \
				(i == BALANCE_V)    || (i == CYL_H_V))
		{
			me->m_pValve->setVal(me->m_pValve, i, 1);
		}else	me->m_pValve->setVal(me->m_pValve, i, 0);
	}
  Mersm_gratCylCtrl(me);  //光栅检测
	/*给MODBUS发消息更新当前状态,直接发送指针*/
	msg.dataType = MB_UPDATE_STATE;
	msg.pVoid = (void *)MB_STATE_DRY_CYL_DOWN;
	xQueueSend(g_modbusQ, &msg, 10);

  me->m_timeOut->setDelayTime(me->m_timeOut, 25); //定时30s

	printf("enterDRY_CYL_DOWN\r\n");
}
	
/*
*函数功能：有限状态机进入干燥状态下的气缸上抬子状态
*/
static void enterDRY_CLY_UP(Mersm_Dry * const me)
{
	int i = 0;
	AppMesage msg;
	for (i = WATER_IN_V; i < MAX_NUM_V; i ++){
		/*微真空，大漏1234，干燥，平衡，抽气，抽水,气缸下进气控制阀打开，其他阀门关闭*/
		if ((i == LIT_VACUUM_V) || \
				(i == BIG_LEAK_1_V) || (i == BIG_LEAK_2_V) || \
			  (i == BIG_LEAK_3_V) || (i == BIG_LEAK_4_V) || \
				(i == DRY_V)        || (i == BALANCE_V)    || \
				(i == AIR_IN_V)     || (i == PUMP_WATER_V) || \
		    (i == CYL_L_V))
		{
			me->m_pValve->setVal(me->m_pValve, i, 1);
		}else	me->m_pValve->setVal(me->m_pValve, i, 0);
	}
  Mersm_gratCylCtrl(me);  //光栅检测
	/*给MODBUS发消息更新当前状态,直接发送指针*/
	msg.dataType = MB_UPDATE_STATE;
	msg.pVoid = (void *)MB_STATE_DRY_CYL_UP;
	xQueueSend(g_modbusQ, &msg, 10);

  me->m_timeOut->setDelayTime(me->m_timeOut, 25); //定时25s

	printf("enterDRY_CLY_UP\r\n");
}
	
/*
*函数功能：有限状态机进入干燥状态下的吹气子状态
*/
static void enterDRY_AIR_PUGE(Mersm_Dry * const me)
{
	int i = 0;
	AppMesage msg;
	for (i = WATER_IN_V; i < MAX_NUM_V; i ++){
		/*大漏微漏1234，干燥，排气打开，其他阀门关闭*/
		if ((i == BIG_LEAK_1_V) || (i == BIG_LEAK_2_V) || \
			  (i == BIG_LEAK_3_V) || (i == BIG_LEAK_4_V) || \
				(i == LIT_LEAK_1_V) || (i == LIT_LEAK_2_V) || \
			  (i == LIT_LEAK_3_V) || (i == LIT_LEAK_4_V) || \
				(i == DRY_V)        || (i == AIR_OUT_V) || (i == CYL_H_V))
		{
			me->m_pValve->setVal(me->m_pValve, i, 1);
		}else	me->m_pValve->setVal(me->m_pValve, i, 0);
	}
  Mersm_gratCylCtrl(me);  //光栅检测

	/*给MODBUS发消息更新当前状态,直接发送指针*/
	msg.dataType = MB_UPDATE_STATE;
	msg.pVoid = (void *)MB_STATE_DRY_CYL_PURGE;
	xQueueSend(g_modbusQ, &msg, 10);

  me->m_delay->setDelayTime(me->m_delay, 15);/*开启定时器，吹气时间暂定为15s*/
	
	printf("enterDRY_AIR_PUGE\r\n");
}
	
/*
*函数功能：有限状态机进入干燥状态下的抽气子状态
*/	
static void enterDRY_AIR_PUMP(Mersm_Dry * const me)
{
	int i = 0;
	AppMesage msg;
	for (i = WATER_IN_V; i < MAX_NUM_V; i ++){
		/*大真空微真空，大漏微漏1234，平衡打开，其他阀门关闭*/
		if ((i == BIG_LEAK_1_V) || (i == BIG_LEAK_2_V) || \
			  (i == BIG_LEAK_3_V) || (i == BIG_LEAK_4_V) || \
				(i == LIT_LEAK_1_V) || (i == LIT_LEAK_2_V) || \
			  (i == LIT_LEAK_3_V) || (i == LIT_LEAK_4_V) || \
				(i == BIG_VACUUM_V) || (i == LIT_VACUUM_V) || \
				(i == BALANCE_V)    || (i == CYL_H_V))
		{
			me->m_pValve->setVal(me->m_pValve, i, 1);
		}else	me->m_pValve->setVal(me->m_pValve, i, 0);
	}
  Mersm_gratCylCtrl(me);  //光栅检测
	/*给MODBUS发消息更新当前状态,直接发送指针*/
	msg.dataType = MB_UPDATE_STATE;
	msg.pVoid = (void *)MB_STATE_DRY_CYL_PUMP;
	xQueueSend(g_modbusQ, &msg, 10);

  s_dryCnt ++;
  me->m_delay->setDelayTime(me->m_delay, 30); //抽气时间暂定为30s
	printf("enterDRY_AIR_PUMP\r\n");
}

/*进入泄漏检测阶段*/
static void enterDRY_LEAK(Mersm_Dry * const me)
{
  int i = 0;
	for (i = WATER_IN_V; i < MAX_NUM_V; i ++){
		/*微漏1234打开，其他阀门关闭*/
		if ((i == LIT_LEAK_1_V) || (i == LIT_LEAK_2_V) || \
			  (i == LIT_LEAK_3_V) || (i == LIT_LEAK_4_V) || \
        (i == CYL_H_V))
		{
			me->m_pValve->setVal(me->m_pValve, i, 1);
		}else	me->m_pValve->setVal(me->m_pValve, i, 0);
	}
  Mersm_gratCylCtrl(me);  //光栅检测

  me->m_delay->setDelayTime(me->m_delay, LEAK_TIME_SEC); //开始泄漏检测
	printf("enterDRY_LEAK\r\n");
}

/*
*函数功能：有限状态机离开干燥状态下的待机子状态
*/
static void exitDRY_NULL(Mersm_Dry * const me)
{
	printf("exitDRY_CYL_DOWN\r\n");
}
	
/*
*函数功能：有限状态机离开干燥状态下的气缸下压子状态
*/
static void exitDRY_CYL_DOWN(Mersm_Dry * const me)
{
  me->m_timeOut->stopTimer(me->m_timeOut);//关闭超时定时器
	printf("exitDRY_CYL_DOWN\r\n");
}
	
/*
*函数功能：有限状态机离开干燥状态下的气缸上抬子状态
*/
static void exitDRY_CLY_UP(Mersm_Dry * const me)
{
  me->m_timeOut->stopTimer(me->m_timeOut);//关闭超时定时器
	printf("exitDRY_CLY_UP\r\n");
}
	
/*
*函数功能：有限状态机离开干燥状态下的吹气子状态
*/
static void exitDRY_AIR_PUGE(Mersm_Dry * const me)
{
  me->m_delay->stopTimer(me->m_delay);  //停止定时器
	printf("exitDRY_AIR_PUGE\r\n");
}
	
/*
*函数功能：有限状态机离开干燥状态下的抽气子状态
*/	
static void exitDRY_AIR_PUMP(Mersm_Dry * const me)
{
  me->m_delay->stopTimer(me->m_delay);  //停止定时器
	printf("exitDRY_AIR_PUMP\r\n");
}

/*离开泄漏检测阶段*/
static void exitDRY_LEAK(Mersm_Dry * const me)
{
  me->m_delay->stopTimer(me->m_delay);  //停止定时器
}