### 描述问题
LD14P 雷达在 STM32F103 上运行时，0° 和 270° 角度始终无有效数据输出（偶现 0° 有数据但 270° 持续缺失），90°,180°数据偶尔正常偶尔无数据。

### 复现步骤
1. 硬件连接：STM32F103C8T6 的 USART3 (PB10/PB11) 连接 LD14P 雷达，雷达由 5V 单独供电。
2. 烧录固件：使用 master 最新代码，开启 DMA + Idle 中断接收模式。
3. 上电运行：雷达旋转正常，串口打印初始化提示后开始输出角度数据。
4. 观察现象：270° 始终显示无数据，0° 数据不稳定。

### 预期行为
串口稳定打印 0°/90°/180°/270° 的有效距离和置信度数据，无频繁“无数据”提示。

### 实际行为
270° 角度持续显示“No valid data”，0°/90°/180° 数据时有时无。

### 串口日志
```c
=========================================
LD14P Valid Data (ASCII Format)
=========================================
180 degree: Distance=197 mm, Confidence=78
0 degree: Distance=158 mm, Confidence=218
90 degree: Distance=158 mm, Confidence=218
-----------------------------------------
270 degree: No valid data (Distance=0 or low confidence)

=========================================
LD14P Valid Data (ASCII Format)
=========================================
0 degree: Distance=1842 mm, Confidence=212
90 degree: Distance=266 mm, Confidence=216
-----------------------------------------
180 degree: No valid data (Distance=0 or low confidence)
270 degree: No valid data (Distance=0 or low confidence)

=========================================
LD14P Valid Data (ASCII Format)
=========================================
0 degree: Distance=1078 mm, Confidence=212
90 degree: Distance=268 mm, Confidence=216
-----------------------------------------
180 degree: No valid data (Distance=0 or low confidence)
270 degree: No valid data (Distance=0 or low confidence)
```
### 环境信息
- **硬件型号**: STM32F103C8T6
- **雷达型号**: LD14P
- **编译器**: STM32CubeCLT / CLion
- **核心代码逻辑**:
  - 使用 `HAL_UARTEx_ReceiveToIdle_DMA` 接收数据。
  - 在 `HAL_UARTEx_RxEventCallback` 中调用 `lidar_parse_data`。
  - 主循环遍历 `Dataprocess[720]`，通过 `float_abs(angle - 0) < 2.0` 筛选角度。

### 排查尝试
- [x] 硬件接线反复确认，TX/RX 正确，无虚焊。
- [x] 更换了全新的 LD14P 雷达，问题依旧。
- [x] 将角度判断阈值从 `2.0` 改为 `5.0`，无效。
- [x] 检查 `float_abs` 函数实现，逻辑正确。
- [ ] 尚未打印所有 720 个点的原始角度数据进行核对。

### 怀疑点
怀疑是 `Dataprocess` 数组中的角度值与实际物理角度的映射存在偏差，或者 `lidar_parse_data` 中的角度计算逻辑有误。