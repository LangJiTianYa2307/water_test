/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-01-14 11:23:10
 * @LastEditors: PanHan
 * @LastEditTime: 2020-01-14 13:58:50
 * @FilePath: \water_keil\Src\HardwareProxy\my_spi.c
 */
#include "my_spi.h"
#include "stm32f7xx_hal.h"

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  file: The file name as string.
  * @param  line: The line in file as a number.
  * @retval None
  */
static void _Error_Handler(char *file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

/*SPI1初始化，配置为主机模式*/
SPI_HandleTypeDef hspi1;
void SPI1_Init(void)
{	 
	/* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }
	__HAL_SPI_ENABLE(&hspi1);                    //使能SPI1
	SPI1_ReadWriteByte(0Xff);                           //启动传输
	
}  
/*SPI2初始化，配置为主机模式*/
SPI_HandleTypeDef hspi2;
void SPI2_Init(void)
{	 
	/* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 7;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }
	__HAL_SPI_ENABLE(&hspi2);                    //使能SPI2
} 
/*SPI3初始化，配置为主机模式*/
SPI_HandleTypeDef hspi3;
void SPI3_Init(void)
{	 
	/* SPI1 parameter configuration*/
  hspi3.Instance = SPI3;
  hspi3.Init.Mode = SPI_MODE_MASTER;
  hspi3.Init.Direction = SPI_DIRECTION_2LINES;
  hspi3.Init.DataSize = SPI_DATASIZE_16BIT;
  hspi3.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi3.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi3.Init.NSS = SPI_NSS_SOFT;
  hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi3.Init.CRCPolynomial = 7;
  if (HAL_SPI_Init(&hspi3) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }
	__HAL_SPI_ENABLE(&hspi3);                    //使能SPI3
	
}

/*回调函数*/
void HAL_SPI_MspInit(SPI_HandleTypeDef* hspi)
{

  GPIO_InitTypeDef GPIO_InitStruct;
  if(hspi->Instance==SPI1)
  {
    /* Peripheral clock enable */
    __HAL_RCC_SPI1_CLK_ENABLE();
		__HAL_RCC_GPIOB_CLK_ENABLE();
    /**SPI1 GPIO Configuration    
    PB3     ------> SPI1_SCK
    PB4     ------> SPI1_MISO
    PB5     ------> SPI1_MOSI 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  }
  if (hspi->Instance == SPI2)
	{
		/* Peripheral clock enable */
    __HAL_RCC_SPI2_CLK_ENABLE();
		__HAL_RCC_GPIOB_CLK_ENABLE();
		/**SPI2 GPIO Configuration    
    PB13     ------> SPI2_SCK
    PB14     ------> SPI2_MISO
    */
		GPIO_InitStruct.Pin = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	}
	if (hspi->Instance == SPI3)
	{
		/* Peripheral clock enable */
    __HAL_RCC_SPI3_CLK_ENABLE();
		__HAL_RCC_GPIOC_CLK_ENABLE();
		/**SPI1 GPIO Configuration    
    PC10     ------> SPI3_SCK
    PC11     ------> SPI1_MISO
    */
		GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
	}

}

/*回调函数*/
void HAL_SPI_MspDeInit(SPI_HandleTypeDef* hspi)
{

  if(hspi->Instance==SPI1)
  {
    /* Peripheral clock disable */
    __HAL_RCC_SPI1_CLK_DISABLE();
  
    /**SPI1 GPIO Configuration    
    PB3     ------> SPI1_SCK
    PB4     ------> SPI1_MISO
    PB5     ------> SPI1_MOSI 
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5);

  }
	if(hspi->Instance==SPI3)
  {
    /* Peripheral clock disable */
    __HAL_RCC_SPI3_CLK_DISABLE();
  
    /**SPI1 GPIO Configuration    
    PC10     ------> SPI3_SCK
    PC11     ------> SPI1_MISO
    */
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_10 | GPIO_PIN_11);

  }

}

//SPI速度设置函数
//SPI速度=fAPB1/分频系数
//@ref SPI_BaudRate_Prescaler:SPI_BAUDRATEPRESCALER_2~SPI_BAUDRATEPRESCALER_2 256
//fAPB1时钟一般为45Mhz：
void SPI1_SetSpeed(u8 SPI_BaudRatePrescaler)
{
    assert_param(IS_SPI_BAUDRATE_PRESCALER(SPI_BaudRatePrescaler));//判断有效性
    __HAL_SPI_DISABLE(&hspi1);            //关闭SPI
    hspi1.Instance->CR1&=0XFFC7;          //位3-5清零，用来设置波特率
    hspi1.Instance->CR1|=SPI_BaudRatePrescaler;//设置SPI速度
    __HAL_SPI_ENABLE(&hspi1);             //使能SPI
    
}

//SPI1 读写一个字节
//TxData:要写入的字节
//返回值:读取到的字节
u8 SPI1_ReadWriteByte(u8 TxData)
{		 			 
	u8 Rxdata;
  HAL_SPI_TransmitReceive(&hspi1,&TxData,&Rxdata,1, 1000);       
 	return Rxdata;
}

////SPI3写数据
//void SPI3_writeBytes(u16 *p_data, u16 len)
//{
//	while (! __HAL_SPI_GET_FLAG(&hspi3, SPI_FLAG_TXE))
//	{
//	}
//	SPI3->DR = *((uint16_t *)p_data);
//}
////SPI3读数据
//void SPI3_readBytes(u16 *p_data, u16 len)
//{
//	while (! __HAL_SPI_GET_FLAG(&hspi3, SPI_FLAG_RXNE))
//	{
//	}
//	*(p_data) = SPI3->DR;
//}

////SPI2写数据
//void SPI2_writeBytes(u16 *p_data, u16 len)
//{
//  while (! __HAL_SPI_GET_FLAG(&hspi2, SPI_FLAG_TXE))
//	{
//	}
//  hspi2.Instance->DR = *((uint16_t *)p_data);
//}
////SPI2读数据
//void SPI2_readBytes(u16 *p_data, u16 len)
//{
//	while (! __HAL_SPI_GET_FLAG(&hspi2, SPI_FLAG_RXNE))
//	{
//	}
//	*(p_data) = SPI2->DR;
//}

//SPI2 读写一个字节
//TxData:要写入的字节
//返回值:读取到的字节
u16 SPI2_ReadWriteByte(u16 TxData)
{
    static u16 Rxdata;
    HAL_SPI_TransmitReceive(&hspi2,(u8 *)&TxData,(u8 *)&Rxdata, 1, 1000);       
    return Rxdata;          		    //返回收到的数据		
}

u8 SPI2_ReadByte(void)
{
	static u8 RxData = 0;
	HAL_SPI_Receive(&hspi2,(u8 *)&RxData, 1, 1000);
	return RxData;
}

void SPI2_WriteByte(u8 data)
{
	HAL_SPI_Transmit(&hspi2,(u8 *)&data, 1, 1000);
}

u8 SPI2_readWriteByte(u8 TxData)
{
	static u8 Rxdata;
	HAL_SPI_TransmitReceive(&hspi2,(u8 *)&TxData,(u8 *)&Rxdata, 1, 1000);       
	return Rxdata;
}
































