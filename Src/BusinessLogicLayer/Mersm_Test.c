/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-03-09 10:12:09
 * @LastEditors: PanHan
 * @LastEditTime: 2020-04-29 09:29:43
 * @FilePath: \water_gcc\Src\BusinessLogicLayer\Mersm_Test.c
 */

#include "Mersm_Test.h"
#include "BLL_Alarm.h"
#include "BLL_Modbus.h"
#include "APP.h"
#include "math.h"
/*----------------------------------------#defines--------------------------------------*/
typedef struct
{
  TestStateType_t m_stateID;
  u8 m_isInitFlag;
  u8 m_stopCount;        //停止状态下，stop信号计数
  u8 m_hCylLockState;    //上气缸锁定时的状态
  u8 m_lCylLockState;    //下气缸锁定时的状态
}Mersm_TestData;  //私有成员
/*--------------------------------------------------------------------------------------*/

/*----------------------------------------static function-------------------------------*/
static void Mersm_init(Mersm_Test * const me);                       //初始化
static TestStateType_t getCurrentState(Mersm_Test * const me); //获取当前状态
/*多事件接收器，单例，故不需要指向对象自身的指针*/
static void Mersm_evInit(void); //初始化事件
static void Mersm_evStart(void); //启动事件
static void Mersm_evStop(void);  //停止事件
static void Mersm_evDelayOver(void); //延时结束
static void Mersm_evLiqSwitch(u8 type);  //液位报警
static void Mersm_evCylPosH(u8 type);    //气缸上限位传感器
static void Mersm_evCylPosL(u8 type);    //气缸下限位传感器
static void Mersm_evGrat(u8 type);       //光栅
static void Mersm_evTimeOut(void);       //超时

void enterSTOP(Mersm_Test * const me);
void enterCYKINDER_DOWN(Mersm_Test * const me);
void enter13TOR(Mersm_Test * const me);
void enterPRODUCT_TEST(Mersm_Test * const me);
void enterAIR_PURGE(Mersm_Test * const me);
void enterAIR_PUMP(Mersm_Test * const me);
void enterIS_DRY(Mersm_Test * const me);
void enterWATER_IN(Mersm_Test * const me);
void enterLOW_PRES_DETEC(Mersm_Test * const me);
void enterADD_PRES(Mersm_Test * const me);
void enterHIGH_PRES_DETEC(Mersm_Test * const me);
void enterWATER_EXH(Mersm_Test * const me);
void enterWATER_PUMP(Mersm_Test * const me);
void enterCYKINDER_UP(Mersm_Test * const me);

void exitSTOP(Mersm_Test * const me);
void exitCYKINDER_DOWN(Mersm_Test * const me);
void exit13TOR(Mersm_Test * const me);
void exitPRODUCT_TEST(Mersm_Test * const me);
void exitAIR_PURGE(Mersm_Test * const me);
void exitAIR_PUMP(Mersm_Test * const me);
void exitIS_DRY(Mersm_Test * const me);
void exitWATER_IN(Mersm_Test * const me);
void exitLOW_PRES_DETEC(Mersm_Test * const me);
void exitADD_PRES(Mersm_Test * const me);
void exitHIGH_PRES_DETEC(Mersm_Test * const me);
void exitWATER_EXH(Mersm_Test * const me);
void exitWATER_PUMP(Mersm_Test * const me);
void exitCYKINDER_UP(Mersm_Test * const me);

static void MersmDev_gratCylCtrl(Mersm_Test * const me);
/*--------------------------------------------------------------------------------------*/

/*----------------------------------------static variable-------------------------------*/
static Mersm_Test s_mersm_Test;
static Mersm_TestData s_Mersm_TestData;
/*--------------------------------------------------------------------------------------*/

/**
 * @brief	创建同步状态机对象
 * @param	
 * @retval	
 */
Mersm_Test * Mersm_Test_create(void)
{
  static Mersm_Test * me = 0;
  Mersm_TestData * private_data = 0;
  if (me == 0){
    me = & s_mersm_Test;

    me->init = Mersm_init;
    me->evInit = Mersm_evInit;
    me->evStart = Mersm_evStart;
    me->evStop = Mersm_evStop;
    me->evDelayOver = Mersm_evDelayOver;
    me->evLiqSwitch = Mersm_evLiqSwitch;
    me->evCylPosH = Mersm_evCylPosH;
    me->evCylPosL = Mersm_evCylPosL;
    me->evGrat = Mersm_evGrat;
    me->evTimeOut = Mersm_evTimeOut;

    me->getCurrentState = getCurrentState;

    me->its_pSubLowPresLeak = Mersm_Leak_create(LEAK_LOW_PRES);
    me->its_pSubHighPresLeak = Mersm_Leak_create(LEAK_HIGH_PRES);

    me->private_data = &s_Mersm_TestData;
    private_data = (Mersm_TestData *)me->private_data;
    private_data->m_isInitFlag = 0;
  }
  return me;
}

/**
 * @brief	测试状态机初始化
 * @param	me:指向对象的指针
 * @retval	
 */
static void Mersm_init(Mersm_Test * const me)
{
  Mersm_TestData * private_data = (Mersm_TestData *)me->private_data;
  if (0 == private_data->m_isInitFlag){
    me->its_pSubLowPresLeak->m_pDelay = me->pDelayT;
    me->its_pSubLowPresLeak->m_pPresPara = me->m_pPresPara;
    me->its_pSubLowPresLeak->m_pTimePara = me->m_pTimePara;
    me->its_pSubLowPresLeak->m_pAD = me->m_pAD;
    me->its_pSubLowPresLeak->m_pValve = me->m_pValve;
    me->its_pSubLowPresLeak->init(me->its_pSubLowPresLeak);

    me->its_pSubHighPresLeak->m_pDelay = me->pDelayT;
    me->its_pSubHighPresLeak->m_pPresPara = me->m_pPresPara;
    me->its_pSubHighPresLeak->m_pTimePara = me->m_pTimePara;
    me->its_pSubHighPresLeak->m_pAD = me->m_pAD;
    me->its_pSubHighPresLeak->m_pValve = me->m_pValve;
    me->its_pSubHighPresLeak->init(me->its_pSubHighPresLeak);

    private_data->m_isInitFlag = 1;
    private_data->m_stopCount = 0;
  }
}

static TestStateType_t getCurrentState(Mersm_Test * const me)
{
  Mersm_TestData * private_data = (Mersm_TestData *)me->private_data;
  return private_data->m_stateID;
}

/**
 * @brief	响应初始化事件，进入默认状态
 * @param	
 * @retval	
 */
static void Mersm_evInit(void)
{
  FlagHand * pErr = errHandCreate();  //错误处理对象
  Mersm_Test * me = & s_mersm_Test;
  Mersm_TestData * private_data = (Mersm_TestData *)me->private_data;
  /*进入默认为状态，清除干燥信息和DEBUG信息*/
  pErr->clearVbStatus(EX_DRY_S);
  pErr->clearVbStatus(EX_DEBUG_S);
  pErr->setVbStatus(EX_END_S);
  private_data->m_stateID = STATE_STOP;
  enterSTOP(me);
}

/**
 * @brief	响应启动信号
 * @param	
 * @retval	
 */
static void Mersm_evStart(void)
{
  Mersm_Test * me = & s_mersm_Test;
  Mersm_TestData * private_data = (Mersm_TestData *)me->private_data;
  FlagHand * pErr = errHandCreate();  //错误处理对象
  AppMesage msg;
  if (STATE_STOP == private_data->m_stateID){
    /*检查光栅是否正常*/
    if (me->m_pExitSig->getExitSing(me->m_pExitSig, GRAT_SING)) pErr->setErrBit(ERR_GRAT);//故障
    else  pErr->clearErrBit(ERR_GRAT);//检测通过，清除故障
    /*检查真空传感器是否正常*/
    if (me->m_pExitSig->getExitSing(me->m_pExitSig, VACUUM_SING)) pErr->setErrBit(ERR_VACUUM);//气压故障
    else  pErr->clearErrBit(ERR_VACUUM);//检测通过，清除气压故障
    /*检查液位传感器是否正常*/
    if (me->m_pExitSig->getExitSing(me->m_pExitSig, WATER_DECT_SING)) pErr->setErrBit(ERR_LIQ);//液位故障
    else  pErr->clearErrBit(ERR_LIQ);//检测通过，清除液位故障
    /*检查气缸位置知否正常*/
    if (0 == me->m_pExitSig->getExitSing(me->m_pExitSig, H_CLY_POS_SING)) pErr->setErrBit(ERR_CYL_H);//气缸未退回
    else  pErr->clearErrBit(ERR_CYL_H);//检测通过，清除气缸未退回故障
    if (1 == me->m_pExitSig->getExitSing(me->m_pExitSig, L_CLY_POS_SING)) pErr->setErrBit(ERR_CYL_L);//气缸错误下压
    else  pErr->clearErrBit(ERR_CYL_L);//检测通过，气缸错误下

    if (pErr->getAllErr())  return; //检测到有错误，则直接返回
    
    /*只有传感器状态检测通过之后才会正式开始测试*/
    exitSTOP(me);
  
    pErr->setVbStatus(EX_RUN_S);  //设备处于运行状态
    pErr->clearAllVbRes();    //清除上次测试结果
    /*更新主界面上历史数据并且清空当前数据*/
    msg.dataType = MB_TEST_OVER;
    xQueueSend(g_modbusQ, &msg, 10);
    
    private_data->m_stateID = STATE_CYKINDER_DOWN;
    enterCYKINDER_DOWN(me);
  }
}

/**
 * @brief	停止事件
 * @param	
 * @retval	
 */
static void Mersm_evStop(void)
{
  Mersm_Test * me = & s_mersm_Test;
  FlagHand * pErr = errHandCreate();  //错误处理对象
  Mersm_TestData * private_data = (Mersm_TestData *)me->private_data;
  switch (private_data->m_stateID){
  case STATE_STOP:
    /*停止停止状态下，连续两次停止信号会强制清除所有错误（光栅除外），否则只清除一般报警*/
    private_data->m_stopCount ++;
    if (2 == private_data->m_stopCount){
      if (ERR_GRAT & pErr->getAllErr()){
        pErr->clearAllErr();  //则清除所有错误
        pErr->setErrBit(ERR_GRAT);
      }
      else pErr->clearAllErr();  //待机状态下收到停止命令，则清除所有错误
      
      private_data->m_stopCount = 0;
      pErr->setVbStatus(EX_READY_S);  //设备准备好，可以启动
    }
    else{
      pErr->clearErrBit(ERR_VACUUM);
      pErr->clearErrBit(ERR_NO_PRODUCT);
      pErr->clearErrBit(ERR_NEED_DRY);
      pErr->clearVbStatus(EX_END_S);
      //没有其他错误
      if (0 == pErr->getAllErr()){
        private_data->m_stopCount = 0;
        pErr->setVbStatus(EX_READY_S);  //设备准备好，可以启动
      } 
    }
    break;
  case STATE_CYKINDER_DOWN:
    exitCYKINDER_DOWN(me);
    private_data->m_stateID = STATE_CYKINDER_UP;
    enterCYKINDER_UP(me);
    break;
  case STATE_13TOR:
    exit13TOR(me);
    private_data->m_stateID  = STATE_CYKINDER_UP;
    enterCYKINDER_UP(me);
    break;
  case STATE_PRODUCT_TEST:
    exitPRODUCT_TEST(me);
    private_data->m_stateID  = STATE_CYKINDER_UP;
    enterCYKINDER_UP(me);
    break;
  case STATE_AIR_PURGE:
    exitAIR_PURGE(me);
    private_data->m_stateID  = STATE_CYKINDER_UP;
    enterCYKINDER_UP(me);
    break;
  case STATE_AIR_PUMP:
    exitAIR_PUMP(me);
    private_data->m_stateID  = STATE_CYKINDER_UP;
    enterCYKINDER_UP(me);
    break;
  case STATE_IS_DRY:
    exitIS_DRY(me);
    private_data->m_stateID  = STATE_CYKINDER_UP;
    enterCYKINDER_UP(me);
    break;
  case STATE_WATER_IN:
    /*注水阶段管路中已经有水了，所以停止时需要进行排水抽水等阶段，后续状态也如此*/
    exitWATER_IN(me);
    private_data->m_stateID  = STATE_WATER_EXH;
    enterWATER_EXH(me);
    break;
  case STATE_LOW_PRES_DETEC:
    /*给子状态机发送停止消息，只有当子状态机停止后才会进入后续状态*/
    me->its_pSubLowPresLeak->evStop(me->its_pSubLowPresLeak);
    if (SUB_NULL == me->its_pSubLowPresLeak->getCurrentState(me->its_pSubLowPresLeak)){
      private_data->m_stateID = STATE_WATER_EXH;
      enterWATER_EXH(me);
    }
    break;
  case STATE_HIGH_PRES_DETEC:
    /*给子状态机发送停止消息，只有当子状态机停止后才会进入后续状态*/
    me->its_pSubHighPresLeak->evStop(me->its_pSubHighPresLeak);
    if (SUB_NULL == me->its_pSubHighPresLeak->getCurrentState(me->its_pSubHighPresLeak)){
      private_data->m_stateID = STATE_WATER_EXH;
      enterWATER_EXH(me);
    }
    break;
  case STATE_ADD_PRES:
    /*此时接收到停止信号时，需要对管路进行排水和抽水*/
    exitADD_PRES(me);
    private_data->m_stateID  = STATE_WATER_EXH;
    enterWATER_EXH(me);
    break;
  case STATE_WATER_EXH:
    exitWATER_EXH(me);
    private_data->m_stateID  = STATE_CYKINDER_UP;
    enterCYKINDER_UP(me);
    break;
  case STATE_WATER_PUMP:
    exitWATER_PUMP(me);
    private_data->m_stateID  = STATE_CYKINDER_UP;
    enterCYKINDER_UP(me);
    break;
  case STATE_CYKINDER_UP:
    /*气缸上升阶段，若是收到停止信号，则进入待机状态*/
    exitCYKINDER_UP(me);
    private_data->m_stateID  = STATE_STOP;
    enterSTOP(me);
    break;
  default:
    break;
  }//end switch (private_data->m_stateID)
}

/**
 * @brief	延时结束事件，当延时结束后会触发该事件
 * @param	
 * @retval	
 */
static void Mersm_evDelayOver(void)
{
  Mersm_Test * me = & s_mersm_Test;
  FlagHand * pErr = errHandCreate();  //错误处理对象
  Mersm_TestData * private_data = (Mersm_TestData *)me->private_data;
  float tempF;
  u32 u32Temp;
  int i;
  switch (private_data->m_stateID){
  case STATE_13TOR:
    exit13TOR(me);
    /*取13TOR结束时，需要真空能达到设定值，否则会发送停止信号*/				
    if (me->m_pExitSig->getExitSing(me->m_pExitSig, VACUUM_SING)){
      pErr->setErrBit(ERR_PUMP);  //泵异常
      private_data->m_stateID  = STATE_CYKINDER_UP;
      enterCYKINDER_UP(me);
    }
    else{
      private_data->m_stateID = STATE_PRODUCT_TEST;
      enterPRODUCT_TEST(me);
    }
    break;
  case STATE_PRODUCT_TEST:
    /*产品检测倒计时结束后，运行至此，此时根据水压传感器数据来判断产品有无，并决定后续流程*/
    tempF = me->m_pAD->getPresData(me->m_pAD, PRO_DEC_WATER);
    exitPRODUCT_TEST(me);
    /*经过实际测试发现，放上零漏件，产品检测时水压约为-50kPa，撤掉零漏件产品检测时水压约为-19kPa*/
    if ((tempF < 0) && (me->m_pPresPara->existLimit < fabs(tempF))){
      /*产品存在，正常运行*/
      private_data->m_stateID = STATE_AIR_PURGE;
      enterAIR_PURGE(me);
    }else{
      /*没有产品，设备停止运行*/
      private_data->m_stateID  = STATE_CYKINDER_UP;
      enterCYKINDER_UP(me);
      pErr->setErrBit(ERR_NO_PRODUCT);//产品检测错误
    }
    break;
  case STATE_AIR_PURGE:
    exitAIR_PURGE(me);
    private_data->m_stateID = STATE_AIR_PUMP;
    enterAIR_PUMP(me);
    break;
  case STATE_AIR_PUMP:
    exitAIR_PUMP(me);
    private_data->m_stateID = STATE_IS_DRY;
    enterIS_DRY(me);
    break;
  case STATE_IS_DRY:
  /*干燥判断阶段，接收到NEXT信号表示倒计时结束，此时会计算这段时间
    内的平均压力并以此来判断是否达到干燥水平，所以不直接进入下一阶段。如果干燥判断成功
    才会进入下一阶段，否则会停止运行。具体信号由干燥判断公式给出*/
    tempF = me->m_pAD->getPresData(me->m_pAD, TEST_PRES_DATA);
    exitIS_DRY(me);
    if (tempF > me->m_pPresPara->dryLimit){
      private_data->m_stateID  = STATE_CYKINDER_UP;
      enterCYKINDER_UP(me);
      pErr->setErrBit(ERR_NEED_DRY);  //界面提示“需要干燥”
      pErr->setVbStatus(EX_DRY_S);//给PLC发送需要干燥信号
      if (me->m_pExitSig->getExitSing(me->m_pExitSig, WATER_DECT_SING)) pErr->setErrBit(ERR_LIQ); //液位异常
    }	
    else{
      /*干燥良好*/
      if (me->m_pExitSig->getExitSing(me->m_pExitSig, WATER_DECT_SING)){
        /*液位异常则直接停止*/
        pErr->setErrBit(ERR_LIQ);
        private_data->m_stateID  = STATE_CYKINDER_UP;
        enterCYKINDER_UP(me);
      }
      else{
        private_data->m_stateID = STATE_WATER_IN;
        enterWATER_IN(me);
      }
    }					
    break;
  case STATE_WATER_IN:
    /*注水过程正常结束后，进入下一步*/
    exitWATER_IN(me);
    if (me->m_pExitSig->getExitSing(me->m_pExitSig, WATER_DECT_SING)){
      private_data->m_stateID = STATE_LOW_PRES_DETEC;
      enterLOW_PRES_DETEC(me);
      me->its_pSubLowPresLeak->evStart(me->its_pSubLowPresLeak); //测试子流程开始运行
    }
    else{
      pErr->setErrBit(ERR_LIQ);  //液位异常
      private_data->m_stateID = STATE_WATER_EXH;
      enterWATER_EXH(me);
    }
    break;
  case STATE_LOW_PRES_DETEC:
    me->its_pSubLowPresLeak->evDelayOver(me->its_pSubLowPresLeak); //低压测试流程中，给子流程发消息
    u32Temp = me->its_pSubLowPresLeak->getResult(me->its_pSubLowPresLeak);
    //只有子流程处于停止状态时，才会根据真空传感器状态判断后续走向
    if (SUB_NULL == me->its_pSubLowPresLeak->getCurrentState(me->its_pSubLowPresLeak)){
      if ((1 == me->m_pExitSig->getExitSing(me->m_pExitSig, VACUUM_SING)) || \
          (0x0f == u32Temp)){
        // /*提前停止，则根据低压结果来更新NG计数*/
        for (i = 0; i < 4; i ++){
          if (u32Temp & (1 << i)) me->its_pSubLowPresLeak->addNgCount(i+1);
          else me->its_pSubLowPresLeak->clearNgCount(i+1);
        }
        private_data->m_stateID = STATE_WATER_EXH;
        enterWATER_EXH(me);
        pErr->setErrBit(ERR_NEED_DRY);
        pErr->setVbStatus(EX_DRY_S);
      }
      else{
        private_data->m_stateID = STATE_ADD_PRES;
        enterADD_PRES(me);
      }
    }
    break;
  case STATE_ADD_PRES:
    exitADD_PRES(me);
    tempF = me->m_pAD->getPresData(me->m_pAD, PRO_DEC_WATER); //获取水压
    /*加压结束需要检测水压是否在设定范围内*/
    if ((tempF > me->m_pPresPara->highWaterMin) && (tempF < me->m_pPresPara->highWaterMax)){
      private_data->m_stateID = STATE_HIGH_PRES_DETEC;
      enterHIGH_PRES_DETEC(me);
      me->its_pSubHighPresLeak->evStart(me->its_pSubHighPresLeak); //高压测试子流程开始运行
    }
    else{
      private_data->m_stateID = STATE_WATER_EXH;
      enterWATER_EXH(me);
    }
    break;
  case STATE_HIGH_PRES_DETEC:
    me->its_pSubHighPresLeak->evDelayOver(me->its_pSubHighPresLeak); //高压测试流程中，给子流程发消息
    if (SUB_NULL == me->its_pSubHighPresLeak->getCurrentState(me->its_pSubHighPresLeak)){
      private_data->m_stateID = STATE_WATER_EXH;
      enterWATER_EXH(me);
    }
    break;
  case STATE_WATER_EXH:
    exitWATER_EXH(me);
    private_data->m_stateID = STATE_WATER_PUMP;
    enterWATER_PUMP(me);
    break;
  case STATE_WATER_PUMP:
    exitWATER_PUMP(me);
    private_data->m_stateID = STATE_CYKINDER_UP;
    enterCYKINDER_UP(me);
    break;
  default:
    break;
  }
}

/**
 * @brief	真空传感器报警事件，暂时不使用
 * @param	
 * @retval	
 */
static void Mersm_evVacuum(u8 type)
{}

/**
 * @brief	水压报警事件，暂时不使用
 * @param	
 * @retval	
 */
static void Mersm_evWaterPres(u8 type)
{}

/**
 * @brief	液位报警信号
 * @param	type:信号类型，0，液位无信号，1，液位有信号
 * @retval	
 */
static void Mersm_evLiqSwitch(u8 type)
{
  Mersm_Test * me = &s_mersm_Test;
  FlagHand * pErr = errHandCreate();  //错误处理对象
  Mersm_TestData * private_data = (Mersm_TestData *)me->private_data;
  /*液位开关警报，可以接受液位变化的信号*/
  switch (private_data->m_stateID){
  case STATE_WATER_IN:
    /*注水状态下接收到液位由低变高，属于正常运行*/
    if (type){
      /*开启倒计时定时器*/
      me->pDelayT->setDelayTime(me->pDelayT, 15); //收到液位信号后继续延时15s,保证液体充满测试腔
      me->pTimeoutT->stopTimer(me->pTimeoutT);    //关闭超时定时器
      pErr->clearErrBit(ERR_LIQ);  //此时是正常运行，清除液位故障
    }
    break;
  case STATE_WATER_EXH:
    /*此时液位为低，说明水已经排出，开始倒计时*/
    if (type == 0){
      me->pDelayT->setDelayTime(me->pDelayT, me->m_pTimePara->waterDrain);//开启定时器
      pErr->clearErrBit(ERR_LIQ);  //此时是正常运行，清除液位故障
    }
    break;
  /*
  **低压检测、加压以及高压检测过程中若出现液位变动（无论液位如何变化，都说明液位由故障）
  */
  case STATE_ADD_PRES:
  case STATE_LOW_PRES_DETEC:
  case STATE_HIGH_PRES_DETEC:
    if (type == 0)  pErr->setErrBit(ERR_LIQ);  //液位故障
    else pErr->clearErrBit(ERR_LIQ);  //清除液位故障
    break;
  case STATE_STOP:
    /*设备待机状态下接收到由低到高的信号表明液位故障，
      收到由高到低的信号则消除故障*/
    if (type == 0)  pErr->clearErrBit(ERR_LIQ);  //清除液位故障
    break;
  default:
    break;
  }
}

/**
 * @brief	气缸上限位信号
 * @param	type:1表示触发信号
 * @retval	
 */
static void Mersm_evCylPosH(u8 type)
{
  Mersm_Test * me = &s_mersm_Test;
  FlagHand * pErr = errHandCreate();  //错误处理对象
  Mersm_TestData * private_data = (Mersm_TestData *)me->private_data;
  if ((STATE_CYKINDER_UP == private_data->m_stateID) && (type == 1)){
    exitCYKINDER_UP(me);
    private_data->m_stateID = STATE_STOP;
    enterSTOP(me);
    pErr->clearErrBit(ERR_CYL_H);  //此时正常流程，清除气缸错误
  }
}

/**
 * @brief	气缸下限位信号
 * @param	type:1表示触发信号
 * @retval	
 */
static void Mersm_evCylPosL(u8 type)
{
  Mersm_Test * me = &s_mersm_Test;
  FlagHand * pErr = errHandCreate();  //错误处理对象
  Mersm_TestData * private_data = (Mersm_TestData *)me->private_data;
  if ((STATE_CYKINDER_DOWN == private_data->m_stateID) && (type == 1)){
      exitCYKINDER_DOWN(me);
      private_data->m_stateID = STATE_13TOR;
      enter13TOR(me);
      pErr->clearErrBit(ERR_CYL_L);  //此时正常流程，清除气缸错误
    }
}

/**
 * @brief	光栅触发
 * @param	type:1光栅触发，0光栅恢复正常
 * @retval	
 */
static void Mersm_evGrat(u8 type)
{
  Mersm_Test * me = &s_mersm_Test;
  FlagHand * pErr = errHandCreate();  //错误处理对象
  Mersm_TestData * private_data = (Mersm_TestData *)me->private_data;
  if (1 == type){
    //收到触发信号，记录当前气缸状态，然后锁定气缸位置并报错
    private_data->m_hCylLockState = me->m_pValve->getVal(me->m_pValve, CYL_H_V);
    private_data->m_lCylLockState = me->m_pValve->getVal(me->m_pValve, CYL_L_V);
    me->m_pValve->setVal(me->m_pValve, CYL_H_V, 0);
    me->m_pValve->setVal(me->m_pValve, CYL_L_V, 0);
    pErr->setErrBit(ERR_GRAT);
  }
  else{
    //解锁气缸动作,并清除错误
    me->m_pValve->setVal(me->m_pValve, CYL_H_V, private_data->m_hCylLockState);
    me->m_pValve->setVal(me->m_pValve, CYL_L_V, private_data->m_lCylLockState);
    private_data->m_hCylLockState = 0;
		private_data->m_lCylLockState = 0;
    pErr->clearErrBit(ERR_GRAT);
  }//end if (1 == type)
}

/**
 * @brief	超时信号触发
 * @param	
 * @retval	
 */
static void Mersm_evTimeOut(void)
{
  Mersm_Test * me = &s_mersm_Test;
  FlagHand * pErr = errHandCreate();  //错误处理对象
  Mersm_TestData * private_data = (Mersm_TestData *)me->private_data;
  switch (private_data->m_stateID){
  case STATE_CYKINDER_DOWN:
    /*超时后，如果发现气缸位置正常，则继续往下运行*/
    if ((0 == me->m_pExitSig->getExitSing(me->m_pExitSig, H_CLY_POS_SING)) && \
        (1 == me->m_pExitSig->getExitSing(me->m_pExitSig, L_CLY_POS_SING))){
      exitCYKINDER_DOWN(me);
      private_data->m_stateID = STATE_13TOR;
      enter13TOR(me);
      pErr->clearErrBit(ERR_CYL_L);  //此时正常流程，清除气缸错误
    }
    else{
      exitCYKINDER_DOWN(me);
      private_data->m_stateID = STATE_CYKINDER_UP;
      enterCYKINDER_UP(me);
      pErr->setErrBit(ERR_CYL_L);//气缸未到位异常
    }
    break;
  case STATE_CYKINDER_UP:
    if ((1 == me->m_pExitSig->getExitSing(me->m_pExitSig, H_CLY_POS_SING)) && \
        (0 == me->m_pExitSig->getExitSing(me->m_pExitSig, L_CLY_POS_SING))){
      exitCYKINDER_UP(me);
      private_data->m_stateID = STATE_STOP;
      enterSTOP(me);
      pErr->clearErrBit(ERR_CYL_H);  //此时正常流程，清除气缸错误
    }
    else{
      pErr->setErrBit(ERR_CYL_H);//气缸未退回异常
    }
    break;
  case STATE_WATER_IN:
    /*注水超时，首先确认液位传感器是否高电平，如果高电平则继续正常运行；低电平则报错并停止运行*/
    if (me->m_pExitSig->getExitSing(me->m_pExitSig, WATER_DECT_SING)){
      /*注水过程正常结束后，进入下一步，还需要清除注水错误标志位*/
      exitWATER_IN(me);
      private_data->m_stateID = STATE_LOW_PRES_DETEC;
      enterLOW_PRES_DETEC(me);
      me->its_pSubLowPresLeak->evStart(me->its_pSubLowPresLeak); //测试子流程开始运行
      pErr->clearErrBit(ERR_WATER_IN_TIME_OUT);  //清除错误标识位
    }
    else{
      pErr->setErrBit(ERR_WATER_IN_TIME_OUT);  //更新错误标识位
      exitWATER_IN(me);
      private_data->m_stateID = STATE_WATER_EXH;
      enterWATER_EXH(me);
    }
    break;
  case STATE_WATER_EXH:
    /*排水状态下，若一段时间检测不到液位由高到低，则触发排水超时，自动进入下一阶段*/
    exitWATER_EXH(me);
    private_data->m_stateID = STATE_WATER_PUMP;
    enterWATER_PUMP(me);
    break;
  case STATE_ADD_PRES:
    /*加压超时，在指定时间内水压没有到指定范围*/
    break;
  default:
    break;
  }//end switch (private_data->m_stateID)
}

/**
 * @brief	检测光栅是否触发，涉及气缸动作的流程均需要调用该函数
 * @param	me:指向状态机对象的指针
 * @retval	
 */
static void MersmDev_gratCylCtrl(Mersm_Test * const me)
{
	// FlagHand * pErr = errHandCreate();  //错误处理对象
  // Mersm_TestData * private_data = (Mersm_TestData *)me->private_data;
	// /*气缸动作时，检测到光栅报警信号，需要将当前气缸控制电磁阀的状态进行保存，对其进行断电
	// 	,后续报警结束，会自动根据所保存的状态来恢复气缸状态*/
	// if (me->m_pExitSig->getExitSing(me->m_pExitSig, GRAT_SING)){
	// 	private_data->m_hCylLockState = me->m_pValve->getVal(me->m_pValve, CYL_H_V);
	// 	private_data->m_lCylLockState = me->m_pValve->getVal(me->m_pValve, CYL_L_V);
	// 	me->m_pValve->setVal(me->m_pValve, CYL_H_V, 0);
	// 	me->m_pValve->setVal(me->m_pValve, CYL_L_V, 0);
	// 	pErr->setErrBit(ERR_GRAT);//光栅报警
	// }
  // else{
	// 	pErr->clearErrBit(ERR_GRAT);//清除光栅报警
	// 	private_data->m_hCylLockState = 0;
	// 	private_data->m_lCylLockState = 0;
	// }
}

/*------------------------------以下为状态机动作函数------------------------------*/
/*进入待机状态*/
void enterSTOP(Mersm_Test * const me)
{
  int i = 0;
	static float iniTime = 0;  //停止状态下的延时时间
	AppMesage msg;
  Mersm_TestData * private_data = (Mersm_TestData *)me->private_data;
	FlagHand * pErr = errHandCreate();  //错误处理对象
	for (i = WATER_IN_V; i < MAX_NUM_V; i ++){
		/*待机时，大真空、微真空阀、平衡阀和气缸下进气控制阀打开，其他阀门关闭*/
		if ((i == BIG_VACUUM_V) || \
			  (i == LIT_VACUUM_V) || \
				(i == BALANCE_V)    || (i == CYL_L_V))
		{
			me->m_pValve->setVal(me->m_pValve, i, 1);
		}else	me->m_pValve->setVal(me->m_pValve, i, 0);
	}
  

	me->pDelayT->stopTimer(me->pDelayT);  //关闭延时时间更新定时
	me->m_pProporV->setOutputPres(me->m_pProporV, 0); //设置PWM电压0V
	/*打开加压阀和p排水阀，1s后关闭，为了排出里面多余的气*/
	me->m_pValve->setVal(me->m_pValve, PRES_TO_WATER_V, 1);
	me->m_pValve->setVal(me->m_pValve, WATER_OUT_V, 1);
	vTaskDelay(1000);
	me->m_pValve->setVal(me->m_pValve, PRES_TO_WATER_V, 0);
  vTaskDelay(1000);
	me->m_pValve->setVal(me->m_pValve, WATER_OUT_V, 0);
	/*给MODBUS发消息更新当前状态,直接发送指针*/
	msg.dataType = MB_UPDATE_STATE;
	msg.pVoid = (void *)MB_STATE_READY;
	xQueueSend(g_modbusQ, &msg, 10);
	/*给modbus发消息，更新倒计时寄存器*/
	msg.dataType = MB_UPDATE_DELAY_TIME;
	msg.pVoid = &iniTime;
	xQueueSend(g_modbusQ, &msg, 10);

  private_data->m_stopCount = 0;  //进入停止状态时，清零计数
  pErr->setVbStatus(EX_END_S);  //机械手END信号
	printf("enterSTOP\r\n");
}

/*进入气缸下压状态*/
void enterCYKINDER_DOWN(Mersm_Test * const me)
{
  int i = 0;
	AppMesage msg;
	for (i = WATER_IN_V; i < MAX_NUM_V; i ++){
		/*气缸下压状态，大真空、微真空阀和平衡阀和气缸上进气控制阀打开，其他阀门关闭*/
		if ((i == BIG_VACUUM_V) || \
			  (i == LIT_VACUUM_V) || \
				(i == BALANCE_V)    || (i == CYL_H_V))
		{
			me->m_pValve->setVal(me->m_pValve, i, 1);
		}else	me->m_pValve->setVal(me->m_pValve, i, 0);
	}

  MersmDev_gratCylCtrl(me);//光栅检测，根据检测结果决定气缸是否继续动作
	/*给MODBUS发消息更新当前状态,直接发送指针*/
	msg.dataType = MB_UPDATE_STATE;
	msg.pVoid = (void *)MB_STATE_CYL_DOWN;
	xQueueSend(g_modbusQ, &msg, 10);
  
	me->pTimeoutT->setDelayTime(me->pTimeoutT, 30); //启动超时定时器，定时时间30s

	printf("enterCYKINDER_DOWN\r\n");
}

/*进入取13TOR状态*/
void enter13TOR(Mersm_Test * const me)
{
  int i = 0;
	AppMesage msg;
	for (i = WATER_IN_V; i < MAX_NUM_V; i ++){
		/*大真空、微真空阀和针阀、平衡阀打开，其他阀门关闭*/
		if ((i == BIG_VACUUM_V) || \
			  (i == LIT_VACUUM_V) || \
				(i == NEEDLE_V)     || \
				(i == BALANCE_V)    || (i == CYL_H_V))
		{
			me->m_pValve->setVal(me->m_pValve, i, 1);
		}else	me->m_pValve->setVal(me->m_pValve, i, 0);
	}

	/*给MODBUS发消息更新当前状态,直接发送指针*/
	msg.dataType = MB_UPDATE_STATE;
	msg.pVoid = (void *)MB_STATE_13_TOR;
	xQueueSend(g_modbusQ, &msg, 10);
	
	me->pDelayT->setDelayTime(me->pDelayT, me->m_pTimePara->tor13);//设置延时时间

	printf("enter13TOR\r\n");
}

/*进入产品检测状态*/
void enterPRODUCT_TEST(Mersm_Test * const me)
{
  int i = 0;
	AppMesage msg;
	for (i = WATER_IN_V; i < MAX_NUM_V; i ++){
		/*微漏1234，抽气阀，抽水阀打开，其他阀门关闭*/
		if ((i == LIT_LEAK_1_V) || \
			  (i == LIT_LEAK_2_V) || \
				(i == LIT_LEAK_3_V) || \
				(i == LIT_LEAK_4_V) || \
				(i == AIR_IN_V)     || \
				(i == PUMP_WATER_V) || (i == CYL_H_V))
		{
			me->m_pValve->setVal(me->m_pValve, i, 1);
		}else	me->m_pValve->setVal(me->m_pValve, i, 0);
	}

  
	/*给MODBUS发消息更新当前状态,直接发送指针*/
	msg.dataType = MB_UPDATE_STATE;
	msg.pVoid = (void *)MB_STATE_EXIST;
	xQueueSend(g_modbusQ, &msg, 10);
	
  me->pDelayT->setDelayTime(me->pDelayT, me->m_pTimePara->productDetc);//设置延时时间
  
	printf("enterPRODUCT_TEST\r\n");
}

/*进入管路吹气状态*/
void enterAIR_PURGE(Mersm_Test * const me)
{
  int i = 0;
	AppMesage msg;
	for (i = WATER_IN_V; i < MAX_NUM_V; i ++){
		/*大漏微漏1234，干燥，排气打开，其他阀门关闭*/
		if ((i == BIG_LEAK_1_V) || (i == BIG_LEAK_2_V) || \
			  (i == BIG_LEAK_3_V) || (i == BIG_LEAK_4_V) || \
				(i == LIT_LEAK_1_V) || (i == LIT_LEAK_2_V) || \
			  (i == LIT_LEAK_3_V) || (i == LIT_LEAK_4_V) || \
				(i == DRY_V)        || (i == AIR_OUT_V)    || (i == CYL_H_V))
		{
			me->m_pValve->setVal(me->m_pValve, i, 1);
		}else	me->m_pValve->setVal(me->m_pValve, i, 0);
	}
  
	/*给MODBUS发消息更新当前状态,直接发送指针*/
	msg.dataType = MB_UPDATE_STATE;
	msg.pVoid = (void *)MB_STATE_PURGE;
	xQueueSend(g_modbusQ, &msg, 10);
	
  me->pDelayT->setDelayTime(me->pDelayT, me->m_pTimePara->PipeAirPurge);//设置延时时间

	printf("enterAIR_PURGE\r\n");
}

/*进入管路抽气状态状态*/
void enterAIR_PUMP(Mersm_Test * const me)
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
	
	/*给MODBUS发消息更新当前状态,直接发送指针*/
	msg.dataType = MB_UPDATE_STATE;
	msg.pVoid = (void *)MB_STATE_PUMP;
	xQueueSend(g_modbusQ, &msg, 10);
	
  me->pDelayT->setDelayTime(me->pDelayT, me->m_pTimePara->PipeAirPump);//设置延时时间
	printf("enterAIR_PUMP\r\n");
}

/*进入干燥判断状态*/
void enterIS_DRY(Mersm_Test * const me)
{
  int i = 0;
	AppMesage msg;
	for (i = WATER_IN_V; i < MAX_NUM_V; i ++){
		/*大真空微真空，平衡打开，其他阀门关闭*/
		if ((i == BIG_VACUUM_V) || (i == LIT_VACUUM_V) || \
				(i == BALANCE_V)    || (i == CYL_H_V))
		{
			me->m_pValve->setVal(me->m_pValve, i, 1);
		}else	me->m_pValve->setVal(me->m_pValve, i, 0);
	}
	
	/*给MODBUS发消息更新当前状态,直接发送指针*/
	msg.dataType = MB_UPDATE_STATE;
	msg.pVoid = (void *)MB_STATE_IS_DRY;
	xQueueSend(g_modbusQ, &msg, 10);

  me->pDelayT->setDelayTime(me->pDelayT, me->m_pTimePara->pipeIsDry);//设置延时时间

	printf("enterIS_DRY\r\n");
}

/*进入注水状态*/
void enterWATER_IN(Mersm_Test * const me)
{
  int i = 0;
	AppMesage msg;
	for (i = WATER_IN_V; i < MAX_NUM_V; i ++)
	{
		/*大真空微真空，微漏1234，针阀，平衡，注水，排水，水泵，其他阀门关闭*/
		if ((i == BIG_VACUUM_V) || (i == LIT_VACUUM_V) || \
				(i == LIT_LEAK_1_V) || (i == LIT_LEAK_2_V) || \
			  (i == LIT_LEAK_3_V) || (i == LIT_LEAK_4_V) || \
				(i == NEEDLE_V)     || (i == BALANCE_V)    || \
			  (i == WATER_IN_V)   || (i == WATER_OUT_V)  || \
				(i == WATER_PUMP)   || (i == CYL_H_V))
		{
			me->m_pValve->setVal(me->m_pValve, i, 1);
		}else	me->m_pValve->setVal(me->m_pValve, i, 0);
	}
	
	/*给MODBUS发消息更新当前状态,直接发送指针*/
	msg.dataType = MB_UPDATE_STATE;
	msg.pVoid = (void *)MB_STATE_WATER_IN;
	xQueueSend(g_modbusQ, &msg, 10);

  me->pTimeoutT->setDelayTime(me->pTimeoutT, 25); //启动超时定时器，超时时间设定为25s

	printf("enterWATER_IN\r\n");
}

/*进入低压检测状态*/
void enterLOW_PRES_DETEC(Mersm_Test * const me)
{
  /*此处由子状态机进行处理*/
  me->its_pSubLowPresLeak->evInit(me->its_pSubLowPresLeak); //首次进入子状态需要初始化
  me->its_pSubLowPresLeak->clearPublicResult(); //清除测试阶段的结果
}

/*进入加压阶段*/
void enterADD_PRES(Mersm_Test * const me)
{
  int i = 0;
	AppMesage msg;
	for (i = WATER_IN_V; i < MAX_NUM_V; i ++){
		/*微真空，微漏1234，针阀，平衡，加压，其他阀门关闭*/
		if ((i == LIT_VACUUM_V) || \
				(i == LIT_LEAK_1_V) || (i == LIT_LEAK_2_V) || \
			  (i == LIT_LEAK_3_V) || (i == LIT_LEAK_4_V) || \
				(i == NEEDLE_V)     || (i == BALANCE_V)    || \
				(i == PRES_TO_WATER_V) || (i == CYL_H_V))
		{
			me->m_pValve->setVal(me->m_pValve, i, 1);
		}else	me->m_pValve->setVal(me->m_pValve, i, 0);
	}
	
	/*给MODBUS发消息更新当前状态,直接发送指针*/
	msg.dataType = MB_UPDATE_STATE;
	msg.pVoid = (void *)MB_STATE_ADD_PRES;
	xQueueSend(g_modbusQ, &msg, 10);

	me->m_pProporV->setOutputPres(me->m_pProporV, me->m_pPresPara->highWaterOpening);//设置比例阀开度

  me->pDelayT->setDelayTime(me->pDelayT, me->m_pTimePara->addPres);

	printf("enterADD_PRES\r\n");
}

/*进入高压测试状态*/
void enterHIGH_PRES_DETEC(Mersm_Test * const me)
{
  /*此处由子状态进行处理*/
  me->its_pSubHighPresLeak->evInit(me->its_pSubHighPresLeak);//首次进入，发送初始化事件
}

/*进入排水状态*/
void enterWATER_EXH(Mersm_Test * const me)
{
  int i = 0;
	AppMesage msg;
	for (i = WATER_IN_V; i < MAX_NUM_V; i ++){
		/*微真空，大漏微漏1234，干燥，加压，排气，排水，其他阀门关闭*/
		if ((i == LIT_VACUUM_V) || \
				(i == LIT_LEAK_1_V) || (i == LIT_LEAK_2_V) || \
			  (i == LIT_LEAK_3_V) || (i == LIT_LEAK_4_V) || \
				(i == BIG_LEAK_1_V) || (i == BIG_LEAK_2_V) || \
			  (i == BIG_LEAK_3_V) || (i == BIG_LEAK_4_V) || \
				(i == DRY_V)        || (i == PRES_TO_WATER_V) || \
				(i == AIR_OUT_V)    || (i == WATER_OUT_V)   || (i == CYL_H_V))
		{
			me->m_pValve->setVal(me->m_pValve, i, 1);
		}else	me->m_pValve->setVal(me->m_pValve, i, 0);
	}
	me->m_pProporV->setOutputPres(me->m_pProporV, 0.6);
  
	/*给MODBUS发消息更新当前状态,直接发送指针*/
	msg.dataType = MB_UPDATE_STATE;
	msg.pVoid = (void *)MB_STATE_WATER_OUT;
	xQueueSend(g_modbusQ, &msg, 10);

  me->pTimeoutT->setDelayTime(me->pTimeoutT, 20); //启动超时定时器，超时时间设定为20s

	printf("enterWATER_EXH\r\n");
}

/*进入抽水状态*/
void enterWATER_PUMP(Mersm_Test * const me)
{
  int i = 0;
	AppMesage msg;
	for (i = WATER_IN_V; i < MAX_NUM_V; i ++){
		/*微真空，大漏微漏1234，干燥，排气，抽气，抽水，其他阀门关闭*/
		if ((i == LIT_VACUUM_V) || \
				(i == LIT_LEAK_1_V) || (i == LIT_LEAK_2_V) || \
			  (i == LIT_LEAK_3_V) || (i == LIT_LEAK_4_V) || \
				(i == BIG_LEAK_1_V) || (i == BIG_LEAK_2_V) || \
			  (i == BIG_LEAK_3_V) || (i == BIG_LEAK_4_V) || \
				(i == DRY_V)        || (i == AIR_IN_V)     || \
				(i == AIR_OUT_V)    || (i == PUMP_WATER_V)  || (i == CYL_H_V))
		{
			me->m_pValve->setVal(me->m_pValve, i, 1);
		}else	me->m_pValve->setVal(me->m_pValve, i, 0);
	}
	
	/*给MODBUS发消息更新当前状态,直接发送指针*/
	msg.dataType = MB_UPDATE_STATE;
	msg.pVoid = (void *)MB_STATE_WATER_PUMP;
	xQueueSend(g_modbusQ, &msg, 10);
	
  me->pDelayT->setDelayTime(me->pDelayT, me->m_pTimePara->waterPump);
	
	printf("enterWATER_PUMP\r\n");
}

/*进入气缸上抬状态*/
void enterCYKINDER_UP(Mersm_Test * const me)
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
	
	/*给MODBUS发消息更新当前状态,直接发送指针*/
	msg.dataType = MB_UPDATE_STATE;
	msg.pVoid = (void *)MB_STATE_CYL_UP;
	xQueueSend(g_modbusQ, &msg, 10);
	
  me->pTimeoutT->setDelayTime(me->pTimeoutT, 30); //设置超时时间30s

	printf("enterCYKINDER_UP\r\n");
}

/*离开待机状态*/
void exitSTOP(Mersm_Test * const me)
{

}

/*离开气缸下压状态*/
void exitCYKINDER_DOWN(Mersm_Test * const me)
{
  me->pTimeoutT->stopTimer(me->pTimeoutT);  //关闭超时定时器
}

/*离开取13TOR状态*/
void exit13TOR(Mersm_Test * const me)
{
  me->pDelayT->stopTimer(me->pDelayT);  //关闭延时定时器
}

/*离开产品检测状态*/
void exitPRODUCT_TEST(Mersm_Test * const me)
{
  me->pDelayT->stopTimer(me->pDelayT);  //关闭延时定时器
}

/*离开管路吹气状态*/
void exitAIR_PURGE(Mersm_Test * const me)
{
  me->pDelayT->stopTimer(me->pDelayT);  //关闭延时定时器
}

/*离开管路抽气状态*/
void exitAIR_PUMP(Mersm_Test * const me)
{
  me->pDelayT->stopTimer(me->pDelayT);  //关闭延时定时器
}

/*离开干燥判断状态*/
void exitIS_DRY(Mersm_Test * const me)
{
  me->pDelayT->stopTimer(me->pDelayT);  //关闭延时定时器
}

/*离开注水状态*/
void exitWATER_IN(Mersm_Test * const me)
{
	/*由于阀门开关顺序可能会导致腔内气压升高，因此做一定的延时处理*/
	me->m_pValve->setVal(me->m_pValve, WATER_IN_V, 0);//关注水阀
	me->m_pValve->setVal(me->m_pValve, WATER_PUMP, 0);//关水泵
	vTaskDelay(500);//延时200ms
	me->m_pValve->setVal(me->m_pValve, WATER_OUT_V, 0);//关排水阀
	vTaskDelay(200);//延时200ms
	me->m_pValve->setVal(me->m_pValve, PRES_TO_WATER_V, 1);//打开加压阀

	/*设定比例阀为20kPa，给MODBUS发消息清零数据更新比例阀开度*/
	me->m_pProporV->setOutputPres(me->m_pProporV, me->m_pPresPara->lowWaterOpening);

	me->pDelayT->stopTimer(me->pDelayT);  //关闭延时定时器
  me->pTimeoutT->stopTimer(me->pTimeoutT);  //关闭超时定时器

	printf("exitWATER_IN\r\n");
}

/*离开低压测试状态*/
void exitLOW_PRES_DETEC(Mersm_Test * const me)
{
  /*此部分由子状态机处理*/
}

/*离开加压状态*/
void exitADD_PRES(Mersm_Test * const me)
{
  me->pDelayT->stopTimer(me->pDelayT);  //关闭延时定时器
}

/*离开高压测试状态*/
void exitHIGH_PRES_DETEC(Mersm_Test * const me)
{
  /*此部分由子状态机处理*/
}

/*离开排水状态*/
void exitWATER_EXH(Mersm_Test * const me)
{
  me->pDelayT->stopTimer(me->pDelayT);  //关闭延时定时器
  me->pTimeoutT->stopTimer(me->pTimeoutT);  //关闭超时定时器
	
	/*排水结束后，要关掉电子比例阀，并延时一段时间，以便腔内剩余气体排出*/
	/*给MODBUS发消息更新比例阀开度*/
	me->m_pProporV->setOutputPres(me->m_pProporV, 0);
	vTaskDelay(500);
	me->m_pValve->setVal(me->m_pValve, PRES_TO_WATER_V, 0);//关加压阀
	vTaskDelay(200);

	printf("exitWATER_EXH\r\n");
}

/*离开抽水状态*/
void exitWATER_PUMP(Mersm_Test * const me)
{
  me->pDelayT->stopTimer(me->pDelayT);  //关闭延时定时器
}

/*离开气缸上抬状态*/
void exitCYKINDER_UP(Mersm_Test * const me)
{
  me->pTimeoutT->stopTimer(me->pTimeoutT);  //关闭超时定时器
}