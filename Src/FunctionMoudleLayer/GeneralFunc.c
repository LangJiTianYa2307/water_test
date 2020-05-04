/*
 * @Descripttion: 
 * @version: 
 * @Author: PanHan
 * @Date: 2020-02-28 08:55:30
 * @LastEditors: PanHan
 * @LastEditTime: 2020-04-11 14:27:09
 * @FilePath: \water_gcc\Src\FunctionMoudleLayer\GeneralFunc.c
 */
#include "GeneralFunc.h"
#include "math.h"


/**
  * @brief	均值滤波（去除最大最小值）
  * @param	arr:指向保存数据的指针
  * @param  len:数据长度
  * @param  dataType:数据类型
  * @param  res:保存结果的指针
  * @retval	
  */
void meanFilter(void * const arr, u32 len, u32 dataType, void * res)
{
  DataUnion valueMax;
	DataUnion valueMin;
	DataUnion Sum;
	u32 i = 0;

  if (len <= 2) return;
  switch (dataType)
  {
  case GF_FLOAT_DATA:
    Sum.f_data = 0;
    valueMax.f_data = ((float *)arr)[0];
	  valueMin.f_data = ((float *)arr)[0];
    for (i = 0; i < len; i ++){
      Sum.f_data = Sum.f_data + ((float *)arr)[i];
      if (valueMax.f_data < ((float *)arr)[i])  valueMax.f_data = ((float *)arr)[i];
      if (valueMin.f_data > ((float *)arr)[i])  valueMin.f_data = ((float *)arr)[i];
    }
    *((float *)res) = (Sum.f_data - valueMax.f_data - valueMin.f_data) / (len - 2);
    break;
  case GF_U32_DATA:
    Sum.u32_data = 0;
    valueMax.u32_data = ((u32 *)arr)[0];
	  valueMin.u32_data = ((u32 *)arr)[0];
    for (i = 0; i < len; i ++){
      Sum.u32_data = Sum.u32_data + ((u32 *)arr)[i];
      if (valueMax.u32_data < ((u32 *)arr)[i])  valueMax.u32_data = ((u32 *)arr)[i];
      if (valueMin.u32_data > ((u32 *)arr)[i])  valueMin.u32_data = ((u32 *)arr)[i];
    }
    *((u32 *)res) = (Sum.u32_data - valueMax.u32_data - valueMin.u32_data) / (len - 2);
    break;
  case GF_INT_DATA:
    Sum.int_data = 0;
    valueMax.int_data = ((int *)arr)[0];
	  valueMin.int_data = ((int *)arr)[0];
    for (i = 0; i < len; i ++){
      Sum.int_data = Sum.int_data + ((int *)arr)[i];
      if (valueMax.int_data < ((int *)arr)[i])  valueMax.int_data = ((int *)arr)[i];
      if (valueMin.int_data > ((int *)arr)[i])  valueMin.int_data = ((int *)arr)[i];
    }
    *((int *)res) = (Sum.int_data - valueMax.int_data - valueMin.int_data) / (len - 2);
    break;
  default:
    break;
  }
}

/**
 * @brief	最小二乘法拟合数据
 * @param	xArr:X数据
 * @param yArr:Y数据
 * @param len:数据长度
 * @param a:指向拟合后的斜率
 * @param b:指向拟合后的截距
 * @param mse:拟合结果的均方根误差
 * @retval	
 */
void leastSquareLinearFit(const float * xArr, const float * yArr, const int len, float * a, float * b, float * mse)
{
  int i = 0;
  float sum_x2 = 0.0;
  float sum_y = 0.0;
  float sum_x = 0.0;
  float sum_xy = 0.0;
  float sum_e2 = 0.0;

  //计算所需要的中间系数
  for (i = 0; i < len; i ++){
    sum_x2 += xArr[i] * xArr[i];
    sum_y += yArr[i];
    sum_x += xArr[i];
    sum_xy += xArr[i] * yArr[i];
  }

  * a = (len*sum_xy - sum_x*sum_y) / (len*sum_x2 - sum_x*sum_x);
  * b = (sum_x2*sum_y - sum_x*sum_xy) / (len*sum_x2 - sum_x*sum_x);
  
  //计算均方差
  for (i = 0; i < len; i ++){
    sum_e2 += (*a * xArr[i] + *b - yArr[i]) * (*a * xArr[i] + *b - yArr[i]);
  }
  * mse = sqrtf(sum_e2);
}

/**
 * @brief	将浮点数按照U32格式编码进行转换
 * @param	data:需要转换的浮点数
 * @retval	u32 format
 */
u32 convFloat2u32(float data)
{
  float temp = data;
  return *((u32 *)&temp);
}