/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-03-09 10:07:33
 * @LastEditors: PanHan
 * @LastEditTime: 2020-04-14 14:24:34
 * @FilePath: \water_gcc\Src\BusinessLogicLayer\BLL_MainFsm.c
 */

#include "BLL_MainFsm.h"
#include "BLL_Alarm.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "rtos_timers.h"
#include "event_groups.h"
#include "CommonType.h"
#include "BLL_Modbus.h"
#include "APP.h"
#include "FML_Grat.h"
/*----------------------------------------#defines--------------------------------------*/
#define LEAK_RANDOM_MAX 2.5f  //泄漏量随机数最大值

#define E_L_LEAK_1  						(1 << 7)
#define E_L_LEAK_2  						(1 << 8)
#define E_L_LEAK_3  						(1 << 9)
#define E_L_LEAK_4  						(1 << 10)
#define E_H_LEAK_1  						(1 << 11)
#define E_H_LEAK_2  						(1 << 12)
#define E_H_LEAK_3  						(1 << 13)
#define E_H_LEAK_4  						(1 << 14)
/*--------------------------------------------------------------------------------------*/
//主状态机任务
#define FSM_TASK_PRIO 				28
#define FSM_STK_SIZE				512
/*----------------------------------------static function-------------------------------*/
static void Mersm_init(MainFsmDev * const me);                       //初始化
static void Mersm_sendEvent(const AppMesage * msg);

static void recvExtSigEventCb(void * pInstance, void * pData);
static void delayOverCb(void * pInstance, void * pData);
static void timeoutCb(void * pInstance, void * pData);

static void mainFsm_eventDispatchThread(void *pvParameters);  //事件处理线程
/*--------------------------------------------------------------------------------------*/

/*----------------------------------------static variable-------------------------------*/
static MainFsmDev s_mainFsmDev;
static ParaSetting_t m_paraSetting;   //包含设备运行所需要的所有参数
static TaskHandle_t fsmTask_Handler;
static QueueHandle_t s_fsmQ;                     //用于与网络业务进行通信的消息
/*--------------------------------------------------------------------------------------*/

/**
 * @brief	创建主状态机对象（单例模式）
 * @param	
 * @retval	指向主状态机的指针
 */
MainFsmDev * MainFsmDev_create(void)
{
  static MainFsmDev * me = 0;
  if (me  == 0){
    me = & s_mainFsmDev;
    
    me->init = Mersm_init;
    me->sendEvent = Mersm_sendEvent;

    me->its_pSubTest = Mersm_Test_create();
    me->its_pSubDry = mersm_Dry_create();
    me->its_pSubDebug = mersm_Debug_create();

    me->m_isInitFlag = 0;
  }
  return me;
}

/**
 * @brief	主状态机初始化，主要是给各子状态机传递相应的设备和参数指针
 * @param	
 * @retval	
 */
static void Mersm_init(MainFsmDev * const me)
{
  ExitSing * pExitSing = exitSingCreate();
  ValveIODev * pValveIODev = valveIODevCreate();
  ProporValDev * pProporValDev = proporValCreate();
  DataSever * pDataSever = DataSever_create();
  FML_DelayTime * pDelayT = FML_DelayTime_Create(TIME_DISP_TIM);
  FML_DelayTime * pTimeoutT = FML_DelayTime_Create(TIME_OUT_TIM);
  if (0 == me->m_isInitFlag){
    me->its_pSubTest->m_pTimePara = &m_paraSetting.paraTime;
    me->its_pSubTest->m_pPresPara = &m_paraSetting.paraPres;
    me->its_pSubTest->pDelayT = pDelayT;
    me->its_pSubTest->pTimeoutT = pTimeoutT;
    me->its_pSubTest->m_pAD = pDataSever;
    me->its_pSubTest->m_pExitSig = pExitSing;
    me->its_pSubTest->m_pProporV = pProporValDev;
    me->its_pSubTest->m_pValve = pValveIODev;
    me->its_pSubTest->init(me->its_pSubTest);
    
    me->its_pSubDry->m_pParaSetting = &m_paraSetting;
    me->its_pSubDry->m_delay = pDelayT;
    me->its_pSubDry->m_timeOut = pTimeoutT;
    me->its_pSubDry->m_pValve = pValveIODev;
    me->its_pSubDry->m_pAD = pDataSever;
    me->its_pSubDry->m_pExitSig = pExitSing;
    me->its_pSubDry->init(me->its_pSubDry);

    me->its_pSubDebug->m_pValve = pValveIODev;
    me->its_pSubDebug->m_pExitSig = pExitSing;
    me->its_pSubDebug->m_pProporV = pProporValDev;
    me->its_pSubDebug->pTimeoutT = pTimeoutT;
    me->its_pSubDebug->init(me->its_pSubDebug);

    /*注册回调函数*/
    pExitSing->registerCallback(pExitSing, me, recvExtSigEventCb, EXT_SIG_CHANGE);//信号报警
    pDelayT->registerCallback(pDelayT, me, delayOverCb, TIME_OVER); //延时结束
    pTimeoutT->registerCallback(pTimeoutT, me, timeoutCb, TIME_OVER); //超时时间结束

    taskENTER_CRITICAL();
    //创建任务
    xTaskCreate((TaskFunction_t )mainFsm_eventDispatchThread,             
                (const char*    )"fsmTask",           
                (uint16_t       )FSM_STK_SIZE,        
                (void*          )me,                  
                (UBaseType_t    )FSM_TASK_PRIO,        
                (TaskHandle_t*  )&fsmTask_Handler);
    s_fsmQ = xQueueCreate(20, sizeof(AppMesage)); //创建消息队列

    me->m_MainSteteID = STOP_TEST;
    me->its_pSubTest->evInit(); //进入默认伪状态
    me->m_isInitFlag = 1;
    taskEXIT_CRITICAL();
  }
}

/**
 * @brief	状态机事件处理，其中主状态之间发生转换的必要条件是光栅处于未触发状态
 * @param	
 * @retval	
 */
static void mainFsm_eventDispatchThread(void *pvParameters)
{
  MainFsmDev * me = &s_mainFsmDev;
	BaseType_t res; 
	AppMesage msg;
  float * pFloat = 0;
	u32 u32Temp = 0;
	int i = 0;
  ExitSing * pExitSing = exitSingCreate();
  GratDev * pGratDev = GratDev_create();
  ValveIODev * pValveIODev = valveIODevCreate();
  FlagHand * pErr = errHandCreate();  //错误处理对象
	while (1){
		//通过消息队列接收外部消息，
		res = xQueueReceive(s_fsmQ, &msg, portMAX_DELAY);
		if (res == pdTRUE){
      switch (msg.dataType){
      case EV_START:
        if (STOP_TEST == me->m_MainSteteID){
          me->its_pSubTest->evStart();
        }
        break;
      case EV_STOP:
        if (STOP_TEST == me->m_MainSteteID){
          /*给子状态机发送停止消息*/
          me->its_pSubTest->evStop();
        }
        else if ((STOP_DRY == me->m_MainSteteID) && \
                (0 == pExitSing->getExitSing(pExitSing, GRAT_SING))){
          /*给干燥子状态机发送停止消息，只有子状态正确停止后，才会恢复到STOP_TEST状态*/
          me->its_pSubDry->evStop();
          if (DRY_NULL == me->its_pSubDry->m_stateID){
            me->m_MainSteteID = STOP_TEST;
            me->its_pSubTest->evInit(); //测试子状态进入默认伪状态
          }
        }
        break;
      case EV_DELAY_OVER:
        if (STOP_TEST == me->m_MainSteteID){
          me->its_pSubTest->evDelayOver();
        }
        else if (STOP_DRY == me->m_MainSteteID){
          me->its_pSubDry->evDelayOver();
        }
        break;
      case EV_LIQ_SWITCH:
        if (STOP_TEST == me->m_MainSteteID){
          me->its_pSubTest->evLiqSwitch((u32)msg.pVoid);
        }
        break;
      case EV_CYL_POS_H:
        if (STOP_TEST == me->m_MainSteteID){
          /*给子状态机发送上限位报警消息*/
          me->its_pSubTest->evCylPosH((u32)msg.pVoid);
        }
        else if ((STOP_DRY == me->m_MainSteteID) && \
                (0 == pExitSing->getExitSing(pExitSing, GRAT_SING))){
          /*给干燥子状态机发送上限位报警消息，只有子状态正确停止后，才会恢复到STOP_TEST状态*/
          me->its_pSubDry->evCylPosH((u32)msg.pVoid);
          if (DRY_NULL == me->its_pSubDry->m_stateID){
            me->m_MainSteteID = STOP_TEST;
            me->its_pSubTest->evInit(); //测试子状态进入默认伪状态
          }
        }
        else if ((STOP_DEBUG == me->m_MainSteteID) && \
                (0 == pExitSing->getExitSing(pExitSing, GRAT_SING))){
          me->its_pSubDebug->evCylPosH((u32)msg.pVoid);
          if (DEBUG_OFF == me->its_pSubDebug->m_stateID){
            me->m_MainSteteID = STOP_TEST;
            me->its_pSubTest->evInit(); //测试子状态进入默认伪状态
          }
        }
        break;
      case EV_CYL_POS_L:
        if (STOP_TEST == me->m_MainSteteID){
          me->its_pSubTest->evCylPosL((u32)msg.pVoid);
        }
        else if (STOP_DRY == me->m_MainSteteID){
          me->its_pSubDry->evCylPosL((u32)msg.pVoid);
        }
        break;
      case EV_GRAT:
        if (0 == (u32)msg.pVoid){
          pGratDev->trigOff(pGratDev, pValveIODev);
          pErr->clearErrBit(ERR_GRAT);
        }  
        else{
          pGratDev->trigOn(pGratDev, pValveIODev);
          pErr->setErrBit(ERR_GRAT);
        } 
        // if (STOP_TEST == me->m_MainSteteID){
        //   me->its_pSubTest->evGrat((u32)msg.pVoid);
        // }
        // else if (STOP_DRY == me->m_MainSteteID){
        //   me->its_pSubDry->evGrat((u32)msg.pVoid);
        // }
        // else if (STOP_DEBUG == me->m_MainSteteID){
        //   me->its_pSubDebug->evGrat((u32)msg.pVoid);
        // }
        break;
      case EV_DRY_MODE:
        if ((STOP_TEST == me->m_MainSteteID) && \
            (STATE_STOP == me->its_pSubTest->getCurrentState(me->its_pSubTest)) && \
            (0 == pExitSing->getExitSing(pExitSing, GRAT_SING))){
          me->its_pSubDry->evInit();
          me->its_pSubDry->evStart();
          if (DRY_NULL != me->its_pSubDry->m_stateID){
            me->m_MainSteteID = STOP_DRY;
          }
        }
        break;
      case EV_DEBUG_MODE:
        /*DEBUG模式切换事件*/
        if ((STOP_TEST == me->m_MainSteteID) && \
            (STATE_STOP == me->its_pSubTest->getCurrentState(me->its_pSubTest)) && \
            (0 == pExitSing->getExitSing(pExitSing, GRAT_SING))){
          me->its_pSubDebug->evInit();
          me->its_pSubDebug->evStart();
          if (DEBUG_OFF != me->its_pSubDebug->m_stateID){
            me->m_MainSteteID = STOP_DEBUG;
          }
        }
        else if ((STOP_DEBUG == me->m_MainSteteID) && \
                (0 == pExitSing->getExitSing(pExitSing, GRAT_SING))){
          me->its_pSubDebug->evStop();
          if (DEBUG_OFF == me->its_pSubDebug->m_stateID){
            me->m_MainSteteID = STOP_TEST;
            me->its_pSubTest->evInit(); //测试子状态进入默认伪状态
          }
        }
        break;
      case EV_TIME_OUT:
        if (STOP_TEST == me->m_MainSteteID){
          me->its_pSubTest->evTimeOut();
        }
        else if (STOP_DRY == me->m_MainSteteID){
          me->its_pSubDry->evTimeOut();
        }
        else if (STOP_DEBUG == me->m_MainSteteID){
          me->its_pSubDebug->evTimeout();
          if (DEBUG_OFF == me->its_pSubDebug->m_stateID){
            me->m_MainSteteID = STOP_TEST;
            me->its_pSubTest->evInit(); //测试子状态进入默认伪状态
          }
        }
        break;
      case EV_SET_PARA:
        /*根据参数设置界面中的数据来更新各种判定限*/
        pFloat = (float *)&m_paraSetting;
        for (i = 0; i < 42; i ++){
          u32Temp = (((u16 *)msg.pVoid)[2 * i] << 16) | (((u16 *)msg.pVoid)[2 * i + 1]); //两个16位数组合成32位
          *pFloat ++ = *(float *)&u32Temp;
        }
        m_paraSetting.paraPres.lowWaterOpening = m_paraSetting.paraPres.lowWaterOpening / 100.0;
        m_paraSetting.paraPres.highWaterOpening = m_paraSetting.paraPres.highWaterOpening / 100.0;
        break;
      case EV_SET_PROPOR:
        if (STOP_DEBUG == me->m_MainSteteID){
          me->its_pSubDebug->evSetProporV(msg.pVoid);
        }
        break;
      case EV_SET_SIG_V:
        if (STOP_DEBUG == me->m_MainSteteID){
          me->its_pSubDebug->evSetSingleV(msg.pVoid);
        }
        break;
      default:
        break;
      }
    }
  }
}


/**
 * @brief	用于注册信号改变事件的回调函数
 * @param	pInstance:注册回调函数的对象，此处为同步状态机对象
 * @param pData:回调函数需要传递的数据指针，此处为AppMesage
 * @retval	
 */
static void recvExtSigEventCb(void * pInstance, void * pData)
{
  AppMesage * pMsg = (AppMesage *)pData;
  MainFsmDev * me = (MainFsmDev *)pInstance;
  switch (pMsg->dataType){
  case WATER_DECT_SING:
    pMsg->dataType = EV_LIQ_SWITCH;
    me->sendEvent(pMsg);
    break;
  case VACUUM_SING:
    pMsg->dataType = EV_VACUUM;
    me->sendEvent(pMsg);
    break;
  case H_CLY_POS_SING:
    pMsg->dataType = EV_CYL_POS_H;
    me->sendEvent(pMsg);
    break;
  case L_CLY_POS_SING:
    pMsg->dataType = EV_CYL_POS_L;
    me->sendEvent(pMsg);
    break;
  case GRAT_SING:
    pMsg->dataType = EV_GRAT;
    me->sendEvent(pMsg);
    break;
  default:
    break;
  }
}

/**
 * @brief	注册延时结束时的回调函数
 * @param	pInstance:指向注册者对象的指针
 * @param pData:回调函数所传递的数据
 * @retval	
 */
static void delayOverCb(void * pInstance, void * pData)
{
  /*延时结束，给主状态机发送延时结束事件*/
  MainFsmDev * me = (MainFsmDev *)pInstance;
  AppMesage msg;
  msg.dataType = EV_DELAY_OVER;
  me->sendEvent(& msg);
}

/**
 * @brief	注册超时事件的回调函数
 * @param	pInstance:指向注册者对象的指针
 * @param pData:回调函数所传递的数据
 * @retval	
 */
static void timeoutCb(void * pInstance, void * pData)
{
  /*延时结束，给主状态机发送超时事件*/
  MainFsmDev * me = (MainFsmDev *)pInstance;
  AppMesage msg;
  msg.dataType = EV_TIME_OUT;
  me->sendEvent(& msg);
}

/**
 * @brief	给状态机管理线程发送事件消息
 * @param	pMsg:指向消息的指针
 * @retval	
 */
static void Mersm_sendEvent(const AppMesage * pMsg)
{
  AppMesage msg;
  msg.dataType = pMsg->dataType;
  msg.pVoid = pMsg->pVoid;
  xQueueSend(s_fsmQ, &msg, 0);
}