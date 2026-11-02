# STM32 I2C OLED 驱动（SSD1306 / SH1106）

在 **STM32F103C8T6 + Keil MDK + HAL 库** 上验证过的 4 针 I2C OLED 驱动。
基于江协科技 v2.0 驱动，修掉了 4 个会导致「雪花屏 / 全黑 / 不出字」的坑，并附带一份
[新工程接 OLED 的操作指南](docs/OLED_新工程指南.md)。

## 特性

- 0.96 寸 **SSD1306**、1.3 寸 **SH1106** 都能用（SH1106 打开 `OLED_13` 补 2 列偏移）
- 默认硬件 I2C，可切换软件 I2C
- 显存按 **32 字节分包**发送，单包耗时远小于超时
- 超时 100 ms；失败后自动恢复总线（9 个 SCL 脉冲 + STOP + 重新初始化）
- 上电延时 150 ms，等屏内部复位完成再发命令
- 初始化自动尝试 **速率**（工程配置值 → 10 kHz）和 **从机地址**（0x78 → 0x7A）
- 同时发送 SSD1306 的 `0x8D 0x14` 与 SH1106 的 `0xAD 0x8B`（DC-DC 打开）
- `OLED_HasError()` 诊断接口，可配合 LED 快慢闪提示通信状态
- 支持 UTF‑8 / GB2312 中文字库（需在 `OLED_Data.c` 中添加字模）

## 已修复的 4 个问题

| 现象 | 原因 | 修复 |
|---|---|---|
| 满屏雪花 | 一页 129 字节 @20 kHz 需 58 ms，超过 HAL 的 10 ms 超时；超时后 HAL 不发 STOP、不释放锁，I2C 永久 BUSY | 数据按 32 字节分包，超时放宽到 100 ms |
| 全黑（通信正常） | 1.3 寸 SH1106 的 DC-DC 需要 `0xAD 0x8B` 才打开 | 初始化里补发该命令 |
| 全黑（通信失败） | 速率过高（如 100 kHz）或命令发得太早（屏未复位完成） | 上电延时 150 ms；初始化自动降速、换地址 |
| 屏亮着但没有字 | `FontSize` 传了 16，而驱动只认 `OLED_8X16`(8) / `OLED_6X8`(6)，两个分支都不进，一个像素都不画 | 调用处改用 `OLED_8X16` |

## 接线（STM32F103C8T6）

| OLED | STM32 |
|---|---|
| VCC | 3.3 V |
| GND | GND |
| SCL | PB6（I2C1_SCL） |
| SDA | PB7（I2C1_SDA） |

> 模块一般自带 4.7k~10k 上拉；杜邦线尽量短（< 20 cm）。

## 快速开始

1. 把 `inc/*.h` 放到工程的 `Core/Inc`，`src/*.c` 放到 `Core/Src`；
2. CubeMX：I2C1 选 I2C、Standard Mode（100 kHz），PB6/PB7 自动复用开漏；
3. Keil：把 `OLED.c`、`OLED_Data.c` 加入工程，**勾选 C99 Mode**；
4. 代码：

```c
#include "OLED.h"

OLED_Init();                                    /* 上电延时/自检都在里面 */

OLED_ShowString(0, 0,  "Hello", OLED_8X16);     /* 字号用宏，不要写 16 */
OLED_ShowNum   (0, 16, 1234,   4, OLED_8X16);

OLED_Update();                                  /* 改完显存必须刷新 */
```

循环里只刷新数字区域会快很多：

```c
OLED_ShowNum(0, 16, count, 4, OLED_8X16);
OLED_UpdateArea(0, 16, 32, 16);
```

详细的建工程流程、三层验证方法、常见现象速查，见
[docs/OLED_新工程指南.md](docs/OLED_新工程指南.md)。

## 目录结构

```
.
├── README.md
├── inc/
│   ├── OLED.h          # API、引脚/地址/超时/分包等宏
│   └── OLED_Data.h     # 字模数据结构与字符集选择
├── src/
│   ├── OLED.c          # 驱动实现（含分包、总线恢复、自检）
│   └── OLED_Data.c     # ASCII 8x16 / 6x8 字模与示例中文字模
└── docs/
    └── OLED_新工程指南.md
```

## 测试环境

- MCU：STM32F103C8T6，72 MHz（HSE 8 MHz × 9）
- 显示屏：0.96 寸 SSD1306 4 针 I2C（1.3 寸 SH1106 亦验证过）
- 工具：STM32CubeMX、Keil MDK 5（ARMCC V5.06）、STM32Cube FW_F1 V1.8.7

## 换成其它 I2C 口 / 引脚

修改 `inc/OLED.h`：

```c
#define OLED_I2C             hi2c2      /* 使用的 I2C 句柄 */
#define OLED_I2C_GPIO_PORT   GPIOB
#define OLED_I2C_SCL_PIN     GPIO_PIN_6 /* 总线恢复时用 */
#define OLED_I2C_SDA_PIN     GPIO_PIN_7
```

## 版权与致谢

- 原始「0.96 寸 OLED 显示屏驱动程序（4 针脚 I2C 接口）v2.0」：**江协科技**
  （[jiangxiekeji.com](https://jiangxiekeji.com)），版权归江协科技所有；
- HAL 库移植：**zeruns**（blog.zeruns.tech）；
- v2.0 更新与代码格式化：**qlqqs**（blog.qlqqs.com）；
- 本次修复：分包发送 / 超时与总线恢复 / 初始化自检 / 字体参数修正。

请保留源文件头部的版权说明，不要据为己有。

## 免责声明

代码按「原样」提供，仅供学习交流；使用前请自行确认硬件与供电安全。
