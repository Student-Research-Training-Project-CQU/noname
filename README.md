# CLion + CubeMx

本仓库包含基于 STM32F1 的 Lidar（激光雷达）模块固件工程，采用 CMake 构建并在 CLion 中使用 CubeMX 生成的配置进行开发。

本 README 旨在快速说明项目结构、重要文件/目录的作用、常见的构建与刷写方式，以及调试与扩展提示，方便开发者快速上手。

---

## 主要特性
- 基于 STM32F103 系列 MCU 的固件工程
- 使用 CMake 管理工程（兼容 CLion）
- 集成 CubeMX 生成的外设配置（.ioc 文件保存在工程根目录）
- 包含激光雷达点云处理（在 `user/algorithm` 下）与驱动代码（在 `device/lidar` 下）

---

## 环境与依赖（建议）
- CLion（或任意支持 CMake 的 IDE）
- GNU Arm Embedded Toolchain（gcc-arm-none-eabi）或等效交叉编译器
- CMake >= 3.18（CLion 内置 CMake 即可）
- ST 官方工具（可选，用于刷写/调试）：STM32CubeProgrammer 或 ST-Link Utility
- （可选）stlink 工具（如 `st-flash`）或 OpenOCD。若使用OpenOCD烧录，需要使用配置 `stlink.cfg`

工程已包含适配交叉编译器的 CMake 工具链文件：`cmake/gcc-arm-none-eabi.cmake`。

---

## 快速构建（命令行，PowerShell）
下面给出一个使用 CMake 的最小构建示例（在没有使用 CLion UI 的情况下）：

```powershell
# 在项目根目录中运行（PowerShell）
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/gcc-arm-none-eabi.cmake -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
```

构建输出（例如 ELF）通常位于 `cmake-build-debug/` 或上面指定的 `build` 目录下。CLion 使用 CMakePresets.json 或 CMakeLists.txt 打开工程会自动配置并触发构建。

---

## 刷写与调试（建议流程）
- 推荐使用 STM32CubeProgrammer（GUI 或 CLI）或 CLion 的调试配置（使用 ST-Link），通过 ELF 或 BIN 刷写到设备。
- 也可以使用 `st-flash`（来自 stlink 项目）或 OpenOCD，根据你的工具链选择对应命令将 `cmake-build-debug/LidarModule.elf` 或 `LidarModule.bin` 刷写入 MCU。

> 注意：刷写/调试前请确认连接线、供电和目标型号（STM32F103）无误，且 `STM32F103XX_FLASH.ld` 与启动文件 `startup_stm32f103xb.s` 与工程匹配。

---

## 项目结构（主要目录与文件说明）
以下按照工程实际目录对重要文件/目录做解释，便于定位代码与扩展：

- CMakeLists.txt
  - 顶层 CMake 配置，定义工程构建目标、源码路径以及链接规则。
- CMakePresets.json
  - CLion/VSCode 等 IDE 的预设构建配置（可直接在 CLion 中选择）。
- New_Lidar_Module_CLion.ioc
  - CubeMX 的工程配置文件，保存了外设时钟、GPIO、DMA、USART/TIM 配置等。可在 CubeMX 中打开以可视化修改外设。
- startup_stm32f103xb.s
  - 启动汇编文件，定义中断向量与复位入口，必须与目标 MCU 型号一致。
- STM32F103XX_FLASH.ld
  - 链接脚本，定义 Flash/ RAM 布局，影响程序的地址映射与大小限制。
- stlink.cfg
  - 与 ST-Link 相关的配置，供 OpenOCD 或自定义脚本使用（视具体刷写工具而定）。

- cmake/
  - 存放工具链文件和 CMake 扩展脚本（例如 `gcc-arm-none-eabi.cmake`）。
  - 子目录 `stm32cubemx/` 包含 CubeMX 生成的 CMake 辅助脚本。

- cmake-build-debug/（由 CLion / CMake 生成）
  - 构建产物目录（可忽略入版本控制），包含编译输出、map 文件、链接信息等。

- Core/
  - MCU 相关核心代码（通常为 CMSIS/System 与 HAL 初始化接口）
  - `Inc/`：头文件（例如 `main.h`, `tim.h`, `usart.h` 等）
  - `Src/`：源文件（例如 `main.c`, `system_stm32f1xx.c`, 中断与外设初始化代码）

- Drivers/
  - 来自 STM32 Cube 或第三方的驱动与库，通常包含 CMSIS、HAL 驱动源代码与头文件。

- user/
  - 用户工程代码（业务逻辑与算法）
  - `user/algorithm/`：放置点云处理、障碍检测等算法模块
    - `src/`：算法实现文件（例如 `obstacle_detect.c`）
    - `inc/`：对应的头文件
    - 说明：`obstacle_detect.c` 实现了把雷达一圈点云分为四个扇区（前/右/后/左），提取每个扇区的最小距离并做简单平滑滤波的逻辑，提供 `Obstacle_Detect_Update()` 与 `Obstacle_Get()` 等接口供上层调用。
  - `user/device/`：和传感器或外设相关的封装（例如 `lidar` 驱动、WS2812 控制等）
    - `device/lidar/`：与雷达通信、点云数据结构（例如 `Dataprocess` 数组）相关的驱动代码与头文件。

- README.md
  - 项目说明（就是当前文件）

---

## 代码阅读提示与关键点
- 点云数据结构与来源：
  - 雷达驱动模块会把原始测距点转换成统一结构（例如 `Dataprocess[]`），包含每点的 `distance`（毫米）、`angle`（度）与 `confidence`（置信度）等字段。算法模块直接遍历该数组进行方向统计。
- 障碍检测策略：
  - 将 360° 划分为四个扇区（每个扇区有中心角和半宽度），在扇区内取最小距离作为该方向的障碍物距离，并使用指数平滑降低抖动。
- 可配置项：
  - 置信度阈值、扇区半角、滤波系数、最小采样点数等可以在算法源文件中作为宏或在头文件中暴露为可配置参数，便于调参。

---

## 常见问题与排查建议
- 构建失败（找不到交叉编译器）
  - 检查 `gcc-arm-none-eabi` 是否安装并且在 PATH 中，或在 CLion 的 CMake Toolchain 中指向正确的工具链文件 `cmake/gcc-arm-none-eabi.cmake`。
- 刷写失败或设备未响应
  - 检查 ST-Link 连接、供电、复位线路；尝试使用 STM32CubeProgrammer GUI 查看设备连接状态。
- 雷达无数据或数据异常
  - 检查串口 / SPI / I2C（视雷达接口而定）与 DMA 配置；确认 `Dataprocess` 数组是否被正确填充以及置信度字段是否合理。

---

## 贡献与下一步改进建议
- 将关键阈值（滤波系数、置信度阈值、扇区宽度等）移动到公共头文件，并通过 Kconfig / CMake option 或运行时配置暴露，方便快速调参。
- 添加示例数据与单元测试（在主机上运行的仿真 test harness），用于验证算法在异常/边界情况下的稳定性。
- 补充更多文档（例如 Doxygen 注释、接口说明文档、通信协议说明），便于多人协作。
- 优化算法性能（例如使用定长数组、减少不必要的计算、引入更高效的数据结构）以适应更高频率的雷达数据输入。
- 增加对其他类型雷达（例如 16 线、64 线）或不同通信接口（例如 CAN）的支持，提升代码的通用性和适应性。
- 完善错误处理与日志输出机制，便于调试和运行时监控系统状态。
---

