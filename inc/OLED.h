#ifndef __OLED_H
#define __OLED_H

#include "main.h"
#include <stdint.h>
#include "OLED_Data.h"
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>

/************************************* 以下为参数宏定义*******************************************/

/**
 * FontSize参数取值
 * 此参数值不仅用于判断，而且用于计算横向字符偏移，默认值为字体像素宽度
 */
#define OLED_8X16               8
#define OLED_6X8                6

/*IsFilled参数数值*/
#define OLED_UNFILLED           0
#define OLED_FILLED             1

/*******************移植需要修改的宏定义******************/
/* 如果用到中文，编译器附加选项需要加 --no-multibyte-chars (用AC6编译器的不用加) */

/**
 * 选择OLED驱动方式，默认使用硬件I2C。
 * 如果要用软件I2C就将硬件I2C那行的宏定义注释掉，将软件I2C那行的注释取消。
 * 不能同时两个都同时取消注释！
 */

//#define OLED_13                                      /* 1.3寸OLED */
#define OLED_96                                        /* 0.96寸OLED */

#define OLED_USE_HW_I2C                                /* 硬件I2C */
//#define OLED_USE_SW_I2C                              /* 软件I2C */

#ifdef OLED_USE_HW_I2C
#define OLED_I2C                hi2c1                  /* 定义OLED屏使用hi2cx接口 */
extern  I2C_HandleTypeDef       hi2c1;                 /* HAL库使用，指定硬件IIC接口 */
#endif

/* STM32F103C8T6芯片的硬件I2C1: PB6 -- SCL; PB7 -- SDA
 * 需要在stm32cubemx代码初始化时将SCL和SDA引脚的"user lable"分别设置为对应的I2Cx_SCL和I2Cx_SDA。
 */

#define OLED_SCL                I2C_SCL_Pin            /* I2Cx_SCL引脚 */
#define OLED_SDA                I2C_SDA_Pin            /* I2Cx_SDA引脚 */
#define OLED_SCL_GPIO_Port      I2C_SCL_GPIO_Port      /* I2Cx_SCL端口 */
#define OLED_SDA_GPIO_Port      I2C_SDA_GPIO_Port      /* I2Cx_SDA端口 */

/*******************移植需要修改的宏定义******************/

/**
 * OLED从机地址
 * 0x3C是OLED的7位地址，左移1位最后位做读写位变成0x78
 * 有的模块是0x3D（0x7A），初始化时会自动探测这两个地址
 */
#define OLED_ADDRESS      0x3C << 1
#define OLED_ADDRESS_ALT  0x3D << 1

/*
 * 硬件I2C使用的引脚（必须与CubeMX里I2C1的引脚一致：PB6=SCL，PB7=SDA）
 * 这两个宏只在“总线恢复”时使用：把引脚临时切成普通开漏GPIO，手工打时钟脉冲，
 * 把被从机拉死的总线救回来，然后再重新初始化I2C外设。
 */
#define OLED_I2C_GPIO_PORT      GPIOB
#define OLED_I2C_SCL_PIN        GPIO_PIN_6
#define OLED_I2C_SDA_PIN        GPIO_PIN_7

/*
 * 上电后等待OLED内部复位/电源稳定的时间（ms）
 * SSD1306在电源稳定后需要一段时间才能正确接收命令，
 * 等得太短会出现“命令发了但屏没亮”（表现为全黑）
 */
#define OLED_POWER_ON_DELAY 150

/*
 * I2C超时时间（单位：ms）
 * 注意：HAL库的这个超时是“一次调用”的总时间，一旦超时，HAL会直接返回，
 * 既不发送STOP也不释放锁，I2C外设会永久停在BUSY状态。
 * 所以这里必须给足够大的值，同时下面的OLED_I2C_CHUNK要保证单次传输时间远小于它。
 */
#define OLED_I2C_TIMEOUT 100

/*
 * 单次I2C传输的最大数据字节数（不含控制字节0x40）
 * 显存一页是128字节，一次性发出耗时太长，容易触发超时；
 * 这里拆成多包发送，SSD1306的GDDRAM地址指针会自动递增，
 * 分包写入和一次写入的效果完全相同。
 */
#define OLED_I2C_CHUNK 32

/**
 * 软件I2C用的延时时间
 * 下面数值为170MHz主频要延时的值，如果你的主频不一样可以修改一下
 * 100MHz以内的主频改成0就行
 */
#define Delay_time 0

/***************************************以下为函数声明*******************************************/

/*初始化函数*/
void OLED_Init(void);

/*诊断函数：返回1表示最近一次刷新/初始化时有I2C传输错误*/
uint8_t OLED_HasError(void);

/*更新函数*/
void OLED_Update(void);
void OLED_UpdateArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height);

/*显存控制函数*/
void OLED_Clear(void);
void OLED_ClearArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height);
void OLED_Reverse(void);
void OLED_ReverseArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height);

/*显示函数*/
void OLED_ShowChar(int16_t X, int16_t Y, char Char, uint8_t FontSize);
void OLED_ShowString(int16_t X, int16_t Y, char *String, uint8_t FontSize);
void OLED_ShowNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowSignedNum(int16_t X, int16_t Y, int32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowHexNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowBinNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowFloatNum(int16_t X, int16_t Y, double Number, uint8_t IntLength, uint8_t FraLength, uint8_t FontSize);
void OLED_ShowImage(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, const uint8_t *Image);
void OLED_Printf(int16_t X, int16_t Y, uint8_t FontSize, char *format, ...);

/*绘图函数*/
void OLED_DrawPoint(int16_t X, int16_t Y);
uint8_t OLED_GetPoint(int16_t X, int16_t Y);
void OLED_DrawLine(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1);
void OLED_DrawRectangle(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, uint8_t IsFilled);
void OLED_DrawTriangle(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1, int16_t X2, int16_t Y2, uint8_t IsFilled);
void OLED_DrawCircle(int16_t X, int16_t Y, uint8_t Radius, uint8_t IsFilled);
void OLED_DrawEllipse(int16_t X, int16_t Y, uint8_t A, uint8_t B, uint8_t IsFilled);
void OLED_DrawArc(int16_t X, int16_t Y, uint8_t Radius, int16_t StartAngle, int16_t EndAngle, uint8_t IsFilled);

#endif


/*****************江协科技|版权所有****************/
/*****************jiangxiekeji.com*****************/
