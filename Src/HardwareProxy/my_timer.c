/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-01-14 11:23:10
 * @LastEditors: PanHan
 * @LastEditTime: 2020-01-14 13:58:38
 * @FilePath: \water_keil\Src\HardwareProxy\my_timer.c
 */
#include "my_timer.h"
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"

TIM_HandleTypeDef TIM3_Handler;      //定时器句柄 
TIM_HandleTypeDef TIM4_Handler;      //定时器句柄 
TIM_HandleTypeDef TIM6_Handler;      //定时器句柄 
/**定时器溢出时间计算方法:Tout=((arr+1)*(psc+1))/Ft us
   Ft=定时器工作频率,单位:Mhz
*/
void TIM3_Init(u16 arr,u16 psc)
{  
	TIM3_Handler.Instance = TIM3;                          //通用定时器3
	TIM3_Handler.Init.Prescaler = psc;                     //分频系数
	TIM3_Handler.Init.CounterMode = TIM_COUNTERMODE_UP;    //向上计数器
	TIM3_Handler.Init.Period = arr;                        //自动装载值
	TIM3_Handler.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;//时钟分频因子
	HAL_TIM_Base_Init(&TIM3_Handler);
	HAL_TIM_Base_Start_IT(&TIM3_Handler); //使能定时器3和定时器3更新中断：TIM_IT_UPDATE   
}

static void TIM4_Init(u16 arr,u16 psc)
{  
	TIM4_Handler.Instance = TIM4;                          //通用定时器3
	TIM4_Handler.Init.Prescaler = psc;                     //分频系数
	TIM4_Handler.Init.CounterMode = TIM_COUNTERMODE_UP;    //向上计数器
	TIM4_Handler.Init.Period = arr;                        //自动装载值
	TIM4_Handler.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;//时钟分频因子
	HAL_TIM_Base_Init(&TIM4_Handler);
	HAL_TIM_Base_Start_IT(&TIM4_Handler); //使能定时器3和定时器3更新中断：TIM_IT_UPDATE   
}

void TIM6_Init(u16 arr,u16 psc)
{
	TIM6_Handler.Instance = TIM3;                          //通用定时器3
	TIM6_Handler.Init.Prescaler = psc;                     //分频系数
	TIM6_Handler.Init.CounterMode = TIM_COUNTERMODE_UP;    //向上计数器
	TIM6_Handler.Init.Period = arr;                        //自动装载值
	TIM6_Handler.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;//时钟分频因子
	HAL_TIM_Base_Init(&TIM6_Handler);
	HAL_TIM_Base_Start_IT(&TIM6_Handler); //使能定时器3和定时器3更新中断：TIM_IT_UPDATE   
}

/*
*函数功能：PWM初始化
*输入参数：arr:自动重装值;psc:时钟预分频数
*返 回 值：无
*/
TIM_HandleTypeDef TIM3_Handler;         //定时器句柄 
TIM_OC_InitTypeDef TIM3_CH4Handler;     //定时器3通道4句柄
void TIM3_PWM_Init(u16 arr,u16 psc)
{
	TIM3_Handler.Instance = TIM3;            //定时器3
	TIM3_Handler.Init.Prescaler = psc;       //定时器分频
	TIM3_Handler.Init.CounterMode = TIM_COUNTERMODE_UP;//向上计数模式
	TIM3_Handler.Init.Period = arr;          //自动重装载值
	TIM3_Handler.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	HAL_TIM_PWM_Init(&TIM3_Handler);       //初始化PWM
	
	TIM3_CH4Handler.OCMode = TIM_OCMODE_PWM1; //模式选择PWM1
	TIM3_CH4Handler.Pulse = arr / 2;            //设置比较值,此值用来确定占空比，
																					//默认比较值为自动重装载值的一半,即占空比为50%
	TIM3_CH4Handler.OCPolarity = TIM_OCPOLARITY_LOW; //输出比较极性为低 
	HAL_TIM_PWM_ConfigChannel(&TIM3_Handler, &TIM3_CH4Handler, TIM_CHANNEL_4);//配置TIM3通道4
	HAL_TIM_PWM_Start(&TIM3_Handler, TIM_CHANNEL_4);//开启PWM通道4
}

/*
*函数功能：设置PWM占空比
*输入参数：compare:自动重装值
*返 回 值：无
*/
void TIM_SetTIM3Compare4(u32 compare)
{
	TIM3->CCR4 = compare; 
}


//定时器底层驱动，时钟使能，引脚配置
//此函数会被HAL_TIM_PWM_Init()调用
//htim:定时器句柄
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
	GPIO_InitTypeDef GPIO_Initure;
	__HAL_RCC_TIM3_CLK_ENABLE();			//使能定时器3
	__HAL_RCC_GPIOB_CLK_ENABLE();			//开启GPIOB时钟
	
	GPIO_Initure.Pin = GPIO_PIN_1;           	//PB1
	GPIO_Initure.Mode = GPIO_MODE_AF_PP;  	//复用推完输出
	GPIO_Initure.Pull = GPIO_PULLUP;          //上拉
	GPIO_Initure.Speed = GPIO_SPEED_HIGH;     //高速
	GPIO_Initure.Alternate = GPIO_AF2_TIM3;	//PB1复用为TIM3_CH4
	HAL_GPIO_Init(GPIOB, &GPIO_Initure);
}
/**定时器底册驱动，开启时钟，设置中断优先级
   此函数会被HAL_TIM_Base_Init()函数调用
*/
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
	if(htim->Instance == TIM3)
	{
		__HAL_RCC_TIM3_CLK_ENABLE();            //使能TIM3时钟
		HAL_NVIC_SetPriority(TIM3_IRQn,2,0);    //设置中断优先级，抢占优先级1，子优先级0
		HAL_NVIC_EnableIRQ(TIM3_IRQn);          //开启ITM3中断   
	}
	if(htim->Instance == TIM4)
	{
		__HAL_RCC_TIM4_CLK_ENABLE();            //使能TIM4时钟
		HAL_NVIC_SetPriority(TIM4_IRQn,2,0);    //设置中断优先级，抢占优先级1，子优先级0
		HAL_NVIC_EnableIRQ(TIM4_IRQn);          //开启ITM3中断   
	}
}
/*定时器中断处理函数，每中断一次就会给freeRTOS的时钟加一。
*/
void TIM4_IRQHandler(void)
{
	freeRTOSRunTimeTicks ++;
	__HAL_TIM_CLEAR_IT(&TIM4_Handler, TIM_IT_UPDATE);
}

/*freeRTOS任务运行时间统计函数，函数定义在FreeRTOSConfig.h中*/
volatile unsigned long long freeRTOSRunTimeTicks = 0;
void configTimeForRunTimeStats(void)
{
	freeRTOSRunTimeTicks = 0;
	TIM4_Init(50-1, 84-1);
}
