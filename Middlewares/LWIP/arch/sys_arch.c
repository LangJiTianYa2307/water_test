/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
 
#define SYS_ARCH_GLOBALS
 
/* lwIP includes. */
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/lwip_sys.h"
#include "lwip/mem.h"
#include "delay.h"
#include "arch/sys_arch.h"
#include "my_malloc.h"

const uint32_t NullMessage;
 
/**
  * @brief	创建消息邮箱
  * @param	mbox:指向消息邮箱的指针
  * @param	size:大小
  * @retval	ERR_OK,创建成功
  */
err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
	if(size>MAX_QUEUE_ENTRIES)  size = MAX_QUEUE_ENTRIES;		//消息队列最多容纳MAX_QUEUE_ENTRIES消息数目
 	mbox->xQueue = xQueueCreate(size, sizeof(void *));  		//创建消息队列，该消息队列存放指针
	LWIP_ASSERT("OSQCreate", mbox->xQueue!=NULL); 
	if(mbox->xQueue != NULL)  return ERR_OK;  
	else return ERR_MEM; 
} 

/**
  * @brief	释放并删除消息邮箱
  * @param	mbox:需要删除的消息邮箱
  * @retval	
  */
void sys_mbox_free(sys_mbox_t *mbox)
{
	vQueueDelete(mbox->xQueue);
//	LWIP_ASSERT( "OSQDel ",mbox->xQueue == NULL ); 
	mbox->xQueue = NULL;
}

/**
  * @brief	向消息邮箱中发送消息(必须发送成功)
  * @param	msg:要发送的消息
  * @retval	
  */
void sys_mbox_post(sys_mbox_t *mbox, void* msg)
{	 
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  //当msg为空时 msg等于pvNullPointer指向的值 
	if(msg == NULL) msg = (void*)&NullMessage;

  //线程执行时，直接阻塞发送消息；终端状态下，调用相应中断函数发送消息
	if((SCB_ICSR_REG & 0xFF) == 0) {
		while(xQueueSendToBack(mbox->xQueue, &msg, portMAX_DELAY) != pdPASS);
	}else{
		while(xQueueSendToBackFromISR(mbox->xQueue, &msg, &xHigherPriorityTaskWoken) != pdPASS);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}


/**
  * @brief	向消息邮箱中发送消息(不阻塞，只发送一次，无论是否成功)
  * @param	msg:要发送的消息
  * @retval	ERR_OK,OK;
  */
err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{ 
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  //当msg为空时 msg等于pvNullPointer指向的值 
	if(msg == NULL) msg = (void*)&NullMessage;

	if((SCB_ICSR_REG & 0xFF) == 0){
		if(xQueueSendToBack(mbox->xQueue, &msg, 0) != pdPASS) return ERR_MEM;
	}else{
		if(xQueueSendToBackFromISR(mbox->xQueue, &msg, &xHigherPriorityTaskWoken) != pdPASS)  return ERR_MEM;
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}

	return ERR_OK;
}

/**
  * @brief	等待消息邮箱中的消息
  * @param	mbox:消息指针
  * @param	*msg:消息
  * @param	timeout:超时时间
  * @retval	当timeout不为0时如果成功的话就返回等待的时间，
  * 失败的话就返回超时SYS_ARCH_TIMEOUT
  */
u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{ 
	u32_t rtos_timeout,timeout_new;
	BaseType_t temp;
	
	temp = xQueueReceive(mbox->xQueue, msg, 0);
	if((temp == pdPASS) && (*msg != NULL)){	
		if(*msg == (void*)&NullMessage) *msg = NULL;
		return 0;
	}
	
	if(timeout != 0){
		rtos_timeout = (timeout*configTICK_RATE_HZ)/1000; //延时转换为RTOS节拍
		if(rtos_timeout < 1)  rtos_timeout=1;
		else if(rtos_timeout >= portMAX_DELAY)  rtos_timeout = portMAX_DELAY - 1;
	}else rtos_timeout = 0;

	timeout = HAL_GetTick(); //获取当前时间
  //等待时间为0则阻塞等待，否则等待延时时间
	if(rtos_timeout != 0) temp = xQueueReceive(mbox->xQueue, msg, rtos_timeout); 
	else  temp = xQueueReceive(mbox->xQueue, msg, portMAX_DELAY);

	if(temp == errQUEUE_EMPTY){
    /*没有正常读取到数据，返回超时*/
		timeout = SYS_ARCH_TIMEOUT; 
		*msg = NULL;
	}else{
		if(*msg!=NULL){	
		  if(*msg == (void*)&NullMessage)   *msg = NULL;   	
	  }    
		timeout_new= HAL_GetTick();
		if (timeout_new > timeout) timeout_new = timeout_new - timeout;//计算消息等待时间
		else timeout_new = 0xffffffff - timeout + timeout_new; 
		timeout = timeout_new*1000/configTICK_RATE_HZ + 1; //将节拍转换为ms
	}
	return timeout; 
}

//���Ի�ȡ��Ϣ
//*mbox:��Ϣ����
//*msg:��Ϣ
//����ֵ:�ȴ���Ϣ���õ�ʱ��/SYS_ARCH_TIMEOUT
/**
  * @brief	尝试读取邮箱(无等待时间)
  * @param	
  * @retval	0，OK；其他，无消息
  */
u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
	BaseType_t temp;
	
	temp=xQueueReceive(mbox->xQueue, msg, 0);
	if((temp == pdPASS) && (*msg != NULL)){	
		if(*msg == (void*)&NullMessage) *msg = NULL;
		return 0;
	}else return SYS_MBOX_EMPTY;
}

/**
  * @brief	检查邮箱是否有效
  * @param	mbox:邮箱
  * @retval	1有效；0无效
  */
int sys_mbox_valid(sys_mbox_t *mbox)
{  
	if(mbox->xQueue != NULL)  return 1;
	return 0;
} 

/**
  * @brief	设置一个邮箱为无效状态
  * @param	mbox:邮箱
  * @retval	
  */
void sys_mbox_set_invalid(sys_mbox_t *mbox)
{
	mbox->xQueue=NULL;
} 

//����һ���ź���
//*sem:�������ź���
//count:�ź���ֵ
//����ֵ:ERR_OK,����OK
// 	     ERR_MEM,����ʧ��
/**
  * @brief	创建信号量
  * @param	sem:信号箱句柄
  * count:信号量值
  * @retval	ERR_OK,创建成功;ERR_MEM,内存错误
  */
err_t sys_sem_new(sys_sem_t* sem, u8_t count)
{   
	*sem = xSemaphoreCreateCounting(0xFF, count);
	if(*sem == NULL)  return ERR_MEM; 
	LWIP_ASSERT("OSSemCreate ",*sem != NULL );
	return ERR_OK;
} 

/**
  * @brief	等待信号量
  * @param	sem:信号量句柄
  * @param	timeout:超时时间
  * @retval	等待时间
  */
u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{ 
	u32_t rtos_timeout, timeout_new;
	BaseType_t temp;
  if(xSemaphoreTake(*sem, 0) == pdPASS) return 0;

	if(	timeout != 0) {
		rtos_timeout = (timeout * configTICK_RATE_HZ) / 1000;
		if(rtos_timeout < 1)  rtos_timeout = 1;
	}else rtos_timeout = 0; 
	timeout = HAL_GetTick(); 

  if(rtos_timeout	!= 0) temp = xSemaphoreTake(*sem, rtos_timeout);
	else  temp = xSemaphoreTake(*sem, portMAX_DELAY);

 	if(temp != pdPASS)  timeout = SYS_ARCH_TIMEOUT;	
	else{     
 		timeout_new = HAL_GetTick(); 
		if (timeout_new>=timeout) timeout_new = timeout_new - timeout;
		else timeout_new = 0xffffffff - timeout + timeout_new;
 		timeout = (timeout_new*1000/configTICK_RATE_HZ + 1);
	}
	return timeout;
}


/**
  * @brief	发送信号量
  * @param	sem:信号箱句柄
  * @retval	
  */
void sys_sem_signal(sys_sem_t *sem)
{
	while(xSemaphoreGive(*sem) != pdTRUE);
}

/**
  * @brief	释放信号箱
  * @param	sem:信号量句柄
  * @retval	
  */
void sys_sem_free(sys_sem_t *sem)
{
	vSemaphoreDelete(*sem); 
	*sem = NULL;
}

/**
  * @brief	检查信号量是否有效
  * @param	sem:信号量句柄
  * @retval	1,无效；0,有效
  */
int sys_sem_valid(sys_sem_t *sem)
{
	if(*sem != NULL)  return 1;
  else  return 0;		
} 

/**
  * @brief	将信号量设置为无效
  * @param	sem:信号量
  * @retval	
  */
void sys_sem_set_invalid(sys_sem_t *sem)
{
	*sem = NULL;
} 

/**
  * @brief	空函数
  * @param	
  * @retval	
  */
void sys_init(void)
{ 
  
} 

TaskHandle_t LWIP_ThreadHandler;
/**
  * @brief	创建lwip内核进程
  * @param	
  * @retval	
  */
sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
{
	taskENTER_CRITICAL(); 
	xTaskCreate((TaskFunction_t)thread,
						(const char*  )name,
						(uint16_t     )stacksize,
						(void*        )NULL,
						(UBaseType_t  )prio,
						(TaskHandle_t*)&LWIP_ThreadHandler);
	taskEXIT_CRITICAL();  
	return 0;
} 

/**
  * @brief	获取当前系统时间
  * @param	
  * @retval	当前时间(ms)
  */
u32_t sys_now(void)
{
	u32_t lwip_time;
	lwip_time = (HAL_GetTick()*1000/configTICK_RATE_HZ+1);
	return lwip_time; 	
}

/********************************--ADD--************************************/
/*用在cc.h的SYS_ARCH_PROTECT(lev)*/
uint32_t Enter_Critical(void)
{
	if(SCB_ICSR_REG & 0xFF)//���ж���
	{
		return taskENTER_CRITICAL_FROM_ISR();
	}
	else  //���߳�
	{
		taskENTER_CRITICAL();
		return 0;
	}
}
/*用在cc.YS_ARCH_UNPROTECT(lev)*/
void Exit_Critical(uint32_t lev)
{
	if(SCB_ICSR_REG & 0xFF)//���ж���
	{
		taskEXIT_CRITICAL_FROM_ISR(lev);
	}
	else  //���߳�
	{
		taskEXIT_CRITICAL();
	}
}


