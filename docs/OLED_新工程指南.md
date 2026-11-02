# 新工程让 OLED 一次点亮：操作清单

适用：STM32F103C8T6 + 4 针 I2C OLED（0.96 寸 SSD1306 / 1.3 寸 SH1106），
工具链：STM32CubeMX + Keil MDK（ARMCC V5）。

---

## 0. 先准备“已验证过的驱动”

把本工程的这 4 个文件复制出来，当成你自己的 OLED 库（不要再重写底层）：

```
Core/Src/OLED.c
Core/Src/OLED_Data.c
Core/Inc/OLED.h
Core/Inc/OLED_Data.h
```

这套驱动里已经内置了 4 个“血泪补丁”：

| 补丁 | 解决什么 |
|---|---|
| 数据按 32 字节分包发送 + 超时 100 ms | 整页 129 字节超时导致 I2C 卡死（雪花屏） |
| 失败后打 9 个时钟 + STOP + 重新初始化 | I2C 外设卡在 BUSY，后续全部发不出去 |
| 上电延时 150 ms + 自动试 20k/10k 速率、0x78/0x7A 地址 | 屏没准备好 / 速率不匹配 / 地址不是 0x3C（全黑） |
| 初始化里同时发 `0x8D 0x14` 和 `0xAD 0x8B` | 1.3 寸 SH1106 的 DC-DC 不打开（全黑但 I2C 有应答） |

---

## 1. CubeMX 新建工程（约 5 分钟）

1. 选芯片：**STM32F103C8Tx**。
2. **RCC** → HSE：`Crystal/Ceramic Resonator`。
3. **Clock Configuration**：HSE 8 MHz → PLL ×9 → SYSCLK **72 MHz**，
   APB1 = 36 MHz，APB2 = 72 MHz（F103 标准配置）。
4. **I2C1**：Mode 选 `I2C`；Speed Mode `Standard Mode`；Clock Speed **100000**
   （如果跑不通，代码里会自动降到 20k/10k，不用改 CubeMX）。
   → PB6=SCL、PB7=SDA 会自动配成“复用开漏”。
5. **PB14**：`GPIO_Input` + `Pull-up`（对射传感器输入）。
6. **PC13**：`GPIO_Output`（心跳灯）。
7. **SYS** → Debug：`Serial Wire`（保留 SWD 下载）。
8. **Project Manager**
   - Toolchain/IDE：`MDK-ARM`，版本 V5；
   - Code Generator：勾选 **“Keep User Code when re-generating”**。
9. Generate Code。

> 提示：CubeMX 里给 I2C 引脚加不加 label（I2C1_SCL/I2C1_SDA）都行，
> 本驱动用的是 OLED.h 里自己定义的引脚宏，不依赖 label。

---

## 2. 把驱动加进 Keil 工程

1. 把 4 个文件按目录放好（`Core/Src`、`Core/Inc`）。
2. Keil 里右键目标 → **Add Existing Files to Group 'Application/User/Core'**
   → 选中 `OLED.c`、`OLED_Data.c`。
3. `Options for Target` → **C/C++** → 勾选 **C99 Mode**（很重要，代码里有 C99 写法）。
4. Include Paths 里确认有 `..\Core\Inc`（CubeMX 默认就有）。
5. 编译，应该是 0 Error 0 Warning。

---

## 3. main.c 里只需要这几段

```c
/* USER CODE BEGIN Includes */
#include "OLED.h"
/* USER CODE END Includes */
```

```c
  /* USER CODE BEGIN 2 */
    OLED_Init();          // 内部已包含：150ms上电延时 + 自动试速率/地址 + 清屏

    /* ↓↓↓ 第一次用一块新屏时，把下面这行打开做“白光自检”，确认屏能亮 ↓↓↓ */
    // OLED_Reverse(); OLED_Update(); HAL_Delay(800); OLED_Clear();

    OLED_ShowString(0, 0,  "Hello", OLED_8X16);   // 字号用宏，不要写 16！
    OLED_ShowNum   (0, 16, 1234,   4, OLED_8X16);

    OLED_Update();        // ★ 改完显存必须调用，否则屏上不会变
  /* USER CODE END 2 */
```

主循环里刷新：

```c
  while (1)
  {
    count++;
    OLED_ShowNum(0, 16, count, 4, OLED_8X16);
    OLED_UpdateArea(0, 16, 32, 16);   // 只刷数字区域，比整屏快很多
    HAL_Delay(200);
  }
```

---

## 4. 第一次上电：按顺序做三层验证

不要一上来就写字符串，按下面顺序，每步只验证一件事：

| 步骤 | 代码 | 要看到的 |
|---|---|---|
| ① 屏能亮 | `OLED_Clear(); OLED_Reverse(); OLED_Update();` | **整屏白光** |
| ② 能画点 | `OLED_DrawPoint(64,32); OLED_Update();` | 屏幕中间一个点 |
| ③ 能写字 | `OLED_ShowString(0,0,"ABC", OLED_8X16); OLED_Update();` | 左上角 ABC |
| ④ 能刷数字 | `OLED_ShowNum(0,16,123,3,OLED_8X16); OLED_UpdateArea(...)` | 数字正常变化 |

哪一步失败，问题就锁定在哪一层，不用猜。

---

## 5. 换屏 / 换 I2C 口时要改的 3 个地方（OLED.h）

| 情况 | 改哪里 |
|---|---|
| 用 I2C2 而不是 I2C1 | `#define OLED_I2C  hi2c2`（并确认 `extern I2C_HandleTypeDef hi2c2;`） |
| I2C 引脚不是 PB6/PB7 | `OLED_I2C_GPIO_PORT / OLED_I2C_SCL_PIN / OLED_I2C_SDA_PIN`（总线恢复用） |
| 1.3 寸 SH1106（132 列，图像偏 2 像素） | 打开 `#define OLED_13` |
| 0.91 寸 128×32 | 改初始化：`0xA8 0x1F`、`0xDA 0x02`、`0xD3 0x00` |
| 地址不是 0x3C | 不用改，`OLED_Init()` 会自动试 0x3D |

---

## 6. 十条“不再踩坑”的军规

1. **FontSize 只写 `OLED_8X16`(8) 或 `OLED_6X8`(6)**，写 16 会静默什么都不画。
2. **改完显存必须 `OLED_Update()` 或 `OLED_UpdateArea()`**，显示函数只改内存不刷屏。
3. **一次 I2C 传输别超过 32 字节**（驱动已自动分包，不要改回整页发送）。
4. **I2C 超时给 100 ms**，别用 10 ms（ST 的 F1 HAL 超时后不释放总线）。
5. **上电延时 100~150 ms**，等屏内部复位完成再发命令。
6. **Y 坐标尽量取 8 的倍数**（0/8/16/24…），正好对齐一页，最不容易出问题。
7. **显示屏型号先确认**：0.96 寸基本都是 SSD1306；1.3 寸多为 SH1106（驱动已兼容）。
8. **只改 `USER CODE` 区域**，CubeMX 重新生成代码时才不会被覆盖。
9. **一次只改一个变量**：改完速率测一次，改完参数测一次，别一次改三处。
10. **新屏先白光自检，再画点，再写字**——分层验证，永远比“一把梭”快。

---

## 7. 现象 → 原因速查

| 现象 | 优先怀疑 |
|---|---|
| 全黑、LED 快闪（通信失败） | 接线/供电/地址/速率/上拉 |
| 全黑、LED 慢闪（通信正常） | 控制器型号（SH1106 要 `0xAD 0x8B`）、上电延时、面板供电 |
| 雪花 / 乱码 | 传输超时、没有分包、速率过高 |
| 有背景、没有字 | `FontSize` 传错、坐标超出屏幕、没调 `OLED_Update()` |
| 字偏 2 像素 | 1.3 寸 132 列 → 打开 `OLED_13` |
| 字上下/左右反 | 方向命令 `0xA0/0xA1`、`0xC0/0xC8` |
| 只显示一半 | 128×32 屏，MUX/COM 参数要改 |
| 中文显示成方框问号 | 字符集宏（UTF8/GB2312）与字模不匹配，或字库里没有该字 |

---

## 8. 验收清单（新工程做完逐条打勾）

- [ ] CubeMX：I2C1 标准模式，PB6/PB7，时钟 72 MHz
- [ ] Keil：C99 打开，OLED.c / OLED_Data.c 已加入，0 Error 0 Warning
- [ ] `OLED_Init()` 后能整屏点亮（白光自检通过）
- [ ] 能画点、能显示一个字符
- [ ] 显示字符串 + 数字，`OLED_Update()` 后屏幕变化
- [ ] 遮挡传感器，计数能加 1（若不加：把 `GPIO_PIN_SET` 改成 `GPIO_PIN_RESET`）
- [ ] LED 心跳慢闪（表示 OLED 通信正常）
- [ ] 断电重启一次，仍然正常（排除“只复位不重init”的偶发问题）
