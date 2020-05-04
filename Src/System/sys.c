/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-02-25 13:55:19
 * @LastEditors: PanHan
 * @LastEditTime: 2020-02-25 15:24:37
 * @FilePath: \water_gcc\Src\System\sys.c
 */
#include "sys.h"

void Cache_Enable(void)
{
  SCB_EnableICache();//使能I-Cache
  SCB_EnableDCache();//使能D-Cache    
	SCB->CACR |= 1<<2;   //强制D-Cache透写,如不开启,实际使用中可能遇到各种问题
}

/**Fvco:VCO频率；Fsys:系统时钟频率；
   Fusb:USB,SDIO,RNG等的时钟频率
   Fs:PLL输入时钟频率,可以是HSI,HSE等；
   Fvco=Fs*(plln/pllm);
   Fsys=Fvco/pllp=Fs*(plln/(pllm*pllp));
   Fusb=Fvco/pllq=Fs*(plln/(pllm*pllq));
   外部晶振为25M的时候,推荐值:plln=432,pllm=25,pllp=2,pllq=9.
   得到:Fvco=25*(432/25)=432Mhz
   Fsys=432/2=216Mhz
   Fusb=432/9=48Mhz
*/
void Stm32_Clock_Init(u32 plln,u32 pllm,u32 pllp,u32 pllq)
{
    HAL_StatusTypeDef ret = HAL_OK;
    RCC_OscInitTypeDef RCC_OscInitStructure; 
    RCC_ClkInitTypeDef RCC_ClkInitStructure;
	
    __HAL_RCC_PWR_CLK_ENABLE(); //使能PWR时钟
 
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);   //设置调压器输出电压级别，以便在器件未以最大频率工作
      
    RCC_OscInitStructure.OscillatorType = RCC_OSCILLATORTYPE_HSE;    //时钟源为HSE
    RCC_OscInitStructure.HSEState = RCC_HSE_ON;                      //打开HSE
    RCC_OscInitStructure.PLL.PLLState = RCC_PLL_ON;				           //打开PLL
    RCC_OscInitStructure.PLL.PLLSource = RCC_PLLSOURCE_HSE;          //PLL时钟源选择HSE
    RCC_OscInitStructure.PLL.PLLM = pllm;	                           //主PLL和音频PLL分频系数(PLL之前的分频)
    RCC_OscInitStructure.PLL.PLLN = plln;                            //主PLL倍频系数(PLL倍频)
    RCC_OscInitStructure.PLL.PLLP = pllp;                            //系统时钟的主PLL分频系数(PLL之后的分频)
    RCC_OscInitStructure.PLL.PLLQ = pllq;                            //USB/SDIO/随机数产生器等的主PLL分频系数(PLL之后的分频)
    ret = HAL_RCC_OscConfig(&RCC_OscInitStructure);                    //初始化
    if(ret != HAL_OK) while(1);
    
    ret = HAL_PWREx_EnableOverDrive();                                 //开启Over-Driver功能
    if(ret != HAL_OK) while(1);
    
    /*选中PLL作为系统时钟源并且配置HCLK,PCLK1和PCLK2*/
    RCC_ClkInitStructure.ClockType = (RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStructure.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;  //设置系统时钟时钟源为PLL
    RCC_ClkInitStructure.AHBCLKDivider = RCC_SYSCLK_DIV1;         //AHB分频系数为1
    RCC_ClkInitStructure.APB1CLKDivider = RCC_HCLK_DIV4;          //APB1分频系数为4
    RCC_ClkInitStructure.APB2CLKDivider = RCC_HCLK_DIV2;          //APB2分频系数为2
    
    ret = HAL_RCC_ClockConfig(&RCC_ClkInitStructure,FLASH_LATENCY_7);//同时设置FLASH延时周期为7WS，也就是8个CPU周期。
    if(ret != HAL_OK) while(1);
}


#ifdef  USE_FULL_ASSERT
//当编译提示出错的时候此函数用来报告错误的文件和所在行
//file：指向源文件
//line：指向在文件中的行数
void assert_failed(uint8_t* file, uint32_t line)
{ 
	while (1)
	{
	}
}
#endif

u8 Get_ICahceSta(void)
{
    u8 sta;
    sta=((SCB->CCR)>>17)&0X01;
    return sta;
}

u8 Get_DCahceSta(void)
{
    u8 sta;
    sta=((SCB->CCR)>>16)&0X01;
    return sta;
}

void INTX_DISABLE(void)
{  
  __asm volatile
	(
		"	cpsid i													\n" \
		"	bx lr											\n" 
	);
}

void INTX_ENABLE(void)
{
	__asm volatile
	(
		"	cpsie i													\n" \
		"	bx lr											\n" 
	);
}