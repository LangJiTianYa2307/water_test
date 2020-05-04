/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-03-08 14:34:08
 * @LastEditors: PanHan
 * @LastEditTime: 2020-04-14 14:35:20
 * @FilePath: \water_gcc\Src\BusinessLogicLayer\Mersm_Debug.c
 */
#include "Mersm_Debug.h"
#include "BLL_Alarm.h"
#include "BLL_Modbus.h"
#include "APP.h"
/*----------------------------------------#defines--------------------------------------*/

/*--------------------------------------------------------------------------------------*/

/*----------------------------------------static function-------------------------------*/
static void Mersm_init(Mersm_Debug * const me);                       //初始化
/*多事件接收器，单例，故不需要指向对象自身的指针*/
static void Mersm_evInit(void); //初始化事件
static void Mersm_evStart(void); //启动事件
static void Mersm_evStop(void);  //停止事件
static void Mersm_evCylPosH(u8 type);    //气缸上限位传感器
static void Mersm_evGrat(u8 type);       //光栅
static void Mersm_evSetSingleV(void * pData);
static void Mersm_evSetProporV(void * pData);
static void Mersm_evTimeOut(void);       //超时

static void enterDEBUG_OFF(Mersm_Debug * const me);
static void enterDEBUG_ON(Mersm_Debug * const me);
static void enterDEBUG_CYL_UP(Mersm_Debug * const me);

static void exitDEBUG_OFF(Mersm_Debug * const me);
static void exitDEBUG_ON(Mersm_Debug * const me);
static void exitDEBUG_CYL_UP(Mersm_Debug * const me);

static void MersmDev_gratCylCtrl(Mersm_Debug * const me);
/*--------------------------------------------------------------------------------------*/

/*----------------------------------------static variable-------------------------------*/
static Mersm_Debug s_mersm_DebugDev;
static u8 s_hCylLockState;    //上气缸锁定时的状态
static u8 s_lCylLockState;    //下气缸锁定时的状态
/*--------------------------------------------------------------------------------------*/
/**
 * @brief	创建DEBUG状态机对象，单例模式
 * @param	
 * @retval	
 */
Mersm_Debug * mersm_Debug_create(void)
{
  static Mersm_Debug * me = 0;
  if (me == 0){
    me = & s_mersm_DebugDev;
    me->init = Mersm_init;
    me->evInit = Mersm_evInit;
    me->evStart = Mersm_evStart;
    me->evStop = Mersm_evStop;
    me->evCylPosH = Mersm_evCylPosH;
    me->evGrat = Mersm_evGrat;
    me->evSetSingleV = Mersm_evSetSingleV;
    me->evSetProporV = Mersm_evSetProporV;

    me->m_isInitFlag = 0;
  }
  return me;
}

/**
 * @brief	DEBUG状态机初始化
 * @param	me:指向DEBUG状态机对象的指针
 * @retval	
 */
static void Mersm_init(Mersm_Debug * const me)
{
  if (0 == me->m_isInitFlag){
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
  Mersm_Debug * me = & s_mersm_DebugDev;
  me->m_stateID = DEBUG_OFF;
  enterDEBUG_OFF(me);
}

/**
 * @brief	启动信号，即进入DEBUG模式
 * @param	
 * @retval	
 */
static void Mersm_evStart(void)
{
  Mersm_Debug * me = & s_mersm_DebugDev;
  FlagHand * pErr = errHandCreate();  //错误处理对象
  if (DEBUG_OFF == me->m_stateID){
    exitDEBUG_OFF(me);
    pErr->setVbStatus(EX_DEBUG_S);
    me->m_stateID = DEBUG_ON;
    enterDEBUG_ON(me);
  }
}

/**
 * @brief	停止信号，即退出DEBUG模式
 * @param	
 * @retval	
 */
static void Mersm_evStop(void)
{
  Mersm_Debug * me = & s_mersm_DebugDev;
  if (DEBUG_ON == me->m_stateID){
    exitDEBUG_ON(me);
    if (me->m_pExitSig->getExitSing(me->m_pExitSig, H_CLY_POS_SING)){
      me->m_stateID = DEBUG_OFF;
      enterDEBUG_OFF(me);
    }
    else{
      me->m_stateID = DEBUG_CYL_UP;
      enterDEBUG_CYL_UP(me);
    }
  }
  else if (DEBUG_CYL_UP == me->m_stateID){
    exitDEBUG_CYL_UP(me);
    me->m_stateID = DEBUG_OFF;
    enterDEBUG_OFF(me);
  }
}

/**
 * @brief	气缸上限位传感器信号
 * @param	
 * @retval	
 */
static void Mersm_evCylPosH(u8 type)
{
  Mersm_Debug * me = & s_mersm_DebugDev;
  if ((DEBUG_CYL_UP == me->m_stateID) && (type == 1)){
    exitDEBUG_CYL_UP(me);
    me->m_stateID = DEBUG_OFF;
    enterDEBUG_OFF(me);
  }
}

/**
 * @brief	光栅事件响应,主要是有光栅动作都会关闭上下气缸
 * @param	
 * @retval	
 */
static void Mersm_evGrat(u8 type)
{
  Mersm_Debug * me = & s_mersm_DebugDev;
  FlagHand * pErr = errHandCreate();  //错误处理对象
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
}

/**
 * @brief	设置单个阀门状态
 * @param	pData:指向MODBUS寄存器的指针
 * @retval	
 */
static void Mersm_evSetSingleV(void * pData)
{
  int i = 0;
  Mersm_Debug * me = & s_mersm_DebugDev;
  if (DEBUG_ON == me->m_stateID){
    for (i = 0; i < 22; i ++){
      if ((u32)pData & (1 << i)){
        if ((i == 20) || (i == 21) ){
          /*当接收到气缸电磁阀动作信号时，需要检查光栅是否触发，只有光栅不触发的情况下才会动作*/
          if (!(me->m_pExitSig->getExitSing(me->m_pExitSig, GRAT_SING))){
            if ((u32)pData & 0x80000000)	me->m_pValve->setVal(me->m_pValve, i, 1);
            else me->m_pValve->setVal(me->m_pValve, i, 0);
          }
        }else{
          if ((u32)pData & 0x80000000)	me->m_pValve->setVal(me->m_pValve, i, 1);
          else me->m_pValve->setVal(me->m_pValve, i, 0);
        }	
      }
    }
  }
}

/**
 * @brief	设置比例阀开度
 * @param	pData:指向MODBUS寄存器的指针
 * @retval	
 */
static void Mersm_evSetProporV(void * pData)
{
  Mersm_Debug * me = & s_mersm_DebugDev;
  u32 u32Temp;
  if (DEBUG_ON == me->m_stateID){
    u32Temp = (((u16 *)pData)[0] << 16) | (((u16 *)pData)[1]); //两个16位数组合成32位
    me->m_pProporV->setOutputPres(me->m_pProporV, (*(float *)&u32Temp) / 100.0);
    printf("valve opening is %f\r\n", (*(float *)&u32Temp) / 100.0);
  }
}

/*action when timeout*/
static void Mersm_evTimeOut(void)
{
  Mersm_Debug * me = & s_mersm_DebugDev;
  FlagHand * pErr = errHandCreate();  //错误处理对象
  
  if (DEBUG_CYL_UP == me->m_stateID){
    exitDEBUG_CYL_UP(me);
    pErr->setErrBit(ERR_CYL_H); //超时则发送气缸未退回警告
    me->m_stateID = DEBUG_OFF;
    enterDEBUG_OFF(me);
  }
}

static void enterDEBUG_OFF(Mersm_Debug * const me)
{
  FlagHand * pErr = errHandCreate();  //错误处理对象
  me->m_pProporV->setOutputPres(me->m_pProporV, 0); //关闭比例阀
}


static void enterDEBUG_ON(Mersm_Debug * const me)
{
  AppMesage msg;
	FlagHand * pErr = errHandCreate();  //错误处理对象
	/*给MODBUS发消息更新当前状态,直接发送指针*/
	msg.dataType = MB_UPDATE_STATE;
	msg.pVoid = (void *)MB_STATE_DEBUG;
	xQueueSend(g_modbusQ, &msg, 10);
}
static void enterDEBUG_CYL_UP(Mersm_Debug * const me)
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
  //Mersm_gratCylCtrl(me);  //光栅检测
  me->pTimeoutT->setDelayTime(me->pTimeoutT, 15); //定时15s
}

static void exitDEBUG_OFF(Mersm_Debug * const me)
{}
static void exitDEBUG_ON(Mersm_Debug * const me)
{}
static void exitDEBUG_CYL_UP(Mersm_Debug * const me)
{}

/*光栅检测*/
static void MersmDev_gratCylCtrl(Mersm_Debug * const me)
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