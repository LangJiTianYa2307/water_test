/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-01-14 11:22:15
 * @LastEditors: PanHan
 * @LastEditTime: 2020-03-14 20:27:50
 * @FilePath: \water_gcc\Src\ApplicationLayer\RTOS_Task.c
 */
#define MY_OS_GLOBALS

#include "APP.h"
#include "BLL_Modbus.h"
#include "BLL_MainFsm.h"

static void adConvestCb(TimerHandle_t xTimer);

static TimerHandle_t g_adConvT;
//开始任务任务函数
void start_task(void *pvParameters)
{  
  taskENTER_CRITICAL();
  //通讯任务
  xTaskCreate((TaskFunction_t )comTask,             
              (const char*    )"comTask",           
              (uint16_t       )COM_STK_SIZE,        
              (void*          )NULL,                  
              (UBaseType_t    )COM_TASK_PRIO,        
              (TaskHandle_t*  )&comTask_Handler);
	/*
	 *下面的代码是创建系统所需要的定时器
	 */
	//用于实时数据更新的定时器，更新周期500ms，周期更新
	g_adConvT = xTimerCreate((const char*    )"adConvestCb",
														(TickType_t     )500,
														(UBaseType_t    )pdTRUE,
													  (void *         )3,
														(TimerCallbackFunction_t)adConvestCb);
	/*创建消息队列*/													
	g_modbusQ = xQueueCreate(20, sizeof(AppMesage));
	g_netQ = xQueueCreate(10, sizeof(AppMesage));

  //挂起时间显示任务
  vTaskDelete(StartTask_Handler); //删除开始任务
  taskEXIT_CRITICAL();            //退出临界区

}

//通讯任务
void comTask(void *pvParameters)
{
	ModbusDev * me = modbusCreate();
   //创建主状态机并初始化
  MainFsmDev * pMainFsmDev = MainFsmDev_create();
  pMainFsmDev->init(pMainFsmDev);
  
  xTimerStart(g_adConvT, 10);
	me->devRun_thread();
}

//统计任务
//static char timeBuffer[1000];
void statTask(void *pvParameters)
{
	// UBaseType_t free;
	// UBaseType_t used;
  // float pres = 0;
	while (1)
	{
//		//设备控制堆栈使用情况
//    free = uxTaskGetStackHighWaterMark(DeciveTask_Handler);
//		used = DEVICE_STK_SIZE - free;
//		printf("DeciveTask is          :%d\r\n", (int)(used * 100 / (used + free)));
//    //有限状态机堆栈使用情况
//		free = uxTaskGetStackHighWaterMark(FsmTask_Handler);
//		used = FSM_STK_SIZE - free;
//		printf("FsmTask is              :%d\r\n", (int)(used * 100 / (used + free)));
//    //通讯任务堆栈使用情况
//    free = uxTaskGetStackHighWaterMark(comTask_Handler);
//		used = COM_STK_SIZE - free;
//		printf("comTask is             :%d\r\n", (int)(used * 100 / (used + free)));
//    
//    //统计任务堆栈使用情况
//    free = uxTaskGetStackHighWaterMark(statTask_Handler);
//		used = STAT_STK_SIZE - free;
//		printf("statTask is               :%d\r\n", (int)(used * 100 / (used + free)));
//		vTaskGetRunTimeStats(timeBuffer);
//		printf("task name\truning time\tpercentage\r\n");
//		printf("%s\r\n", timeBuffer);
//		printf("\r\n"); 
    vTaskDelay(1000);
	}
}

static void adConvestCb(TimerHandle_t xTimer)
{
	DataSever * me = DataSever_create();
  float presTemp = 0;
	AppMesage msg;
	static struct
	{
		float waterPres;
		float absPres;
	}pres;
  presTemp = me->getPresData(me, CUR_VACUM_DATA);   //获取真空压力,单位pa
  pres.absPres = presTemp / 1000.0; //转换为kPa

  presTemp = me->getPresData(me, CUR_WATER_DATA);     //获取水压，单位为kpa
	pres.waterPres = presTemp / 100.0; //转换为Bar

  /*给MODBUS发消息更新水压和气压数据*/
  msg.dataType = MB_UPDATE_WATER_PRES;
  msg.pVoid = &pres;
  xQueueSend(g_modbusQ, &msg, 0);
}







