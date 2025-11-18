# Horizon Robotics J6M Board Emulation

## 概述

本文档描述了 Horizon Robotics J6M SoC 在 QEMU 中的模拟实现。J6M 是地平线机器人公司开发的车规级 AI 芯片，包含双核 ARM Cortex-R52 处理器。

### 时钟树实现

本实现采用了基于典型汽车 AI SoC 的时钟树架构，包括：
- **REFCLK** (24MHz): 参考振荡器，用于 PLL 和 RTC
- **CPUCLK** (1GHz): CPU 核心时钟，适用于双核 Cortex-R52
- **PERIPHCLK** (500MHz): 高速外设总线时钟
- **AXCLK** (400MHz): AXI 总线互连时钟，用于 DMA
- **CANCLK** (80MHz): CAN 总线时钟（汽车应用典型频率）
- **DDR4CLK** (800MHz): DDR4 内存接口时钟
- **UARTCLK** (48MHz): UART 参考时钟

这些频率代表了适用于 ADAS（高级驾驶辅助系统）应用的真实设计。

## 硬件特性

### CPU
- 双核 ARM Cortex-R52 (ARMv8-R 架构)
- 主频：1 GHz
- 支持单核或双核模式 (`-smp 1` 或 `-smp 2`)

### 内存配置
根据 `j6m_raminfo[]` 数组定义的默认内存布局：

| 名称 | 基地址 | 大小 | 说明 |
|------|--------|------|------|
| TCM0 | 0x00000000 | 128KB | CPU0 紧耦合内存 |
| TCM1 | 0x00100000 | 128KB | CPU1 紧耦合内存 |
| SRAM | 0x10000000 | 1MB | 内部 SRAM |
| DDR | 0x40000000 | 2GB | 主系统内存 (可配置) |

### 外设 (当前实现)

| 外设 | 基地址 | IRQ | 说明 |
|------|--------|-----|------|
| GIC Distributor | 0xF0000000 | - | GICv3 中断控制器 |
| GIC Redistributor | 0xF0100000 | - | GICv3 重分发器 |
| UART0 | 0xA0100000 | 5,6,13 | 串口 0 |
| UART1 | 0xA0101000 | 7,8,14 | 串口 1 |
| UART2 | 0xA0102000 | 9,10,15 | 串口 2 |
| UART3 | 0xA0103000 | 11,12,16 | 串口 3 |
| Watchdog | 0xA0200000 | 0 | 看门狗定时器 |
| Dual Timer | 0xA0201000 | 1,2,3 | 双定时器 |

**注意**: 以上内存映射和 IRQ 编号为占位符，需要根据 J6M 实际数据手册更新。

## 编译

### 配置 QEMU
```bash
cd qemu_j6
mkdir -p build
cd build
../configure --target-list=arm-softmmu --enable-debug
```

### 编译
```bash
make -j$(nproc)
```

## 使用方法

### 基本启动

#### 单核模式
```bash
./qemu-system-arm -M horizon-j6m \
    -m 2G \
    -smp 1 \
    -nographic \
    -serial stdio
```

#### 双核模式
```bash
./qemu-system-arm -M horizon-j6m \
    -m 2G \
    -smp 2 \
    -nographic \
    -serial stdio
```

### 加载内核镜像
```bash
./qemu-system-arm -M horizon-j6m \
    -m 2G \
    -kernel zImage \
    -dtb horizon-j6m.dtb \
    -append "console=ttyAMA0,115200" \
    -nographic
```

### 调试模式
```bash
# 启用 GDB 调试 (等待 GDB 连接在端口 1234)
./qemu-system-arm -M horizon-j6m \
    -m 2G \
    -kernel zImage \
    -s -S \
    -nographic
```

在另一个终端：
```bash
arm-none-eabi-gdb zImage
(gdb) target remote :1234
(gdb) continue
```

### 查看机器信息
```bash
# 列出所有支持的机器
./qemu-system-arm -M help | grep horizon

# 查看机器详细信息
./qemu-system-arm -M horizon-j6m,help
```

## 开发指南

### 需要实现的功能

当前实现是基于 MPS3 AN536 的最小化模板，以下功能需要根据 J6M 数据手册补充：

#### 高优先级
1. **更新内存映射**: 修改 `j6m_raminfo[]` 以匹配实际 J6M 内存布局
2. ~~**时钟配置**: 在 `j6m_oscclk[]` 中设置正确的时钟频率~~ ✅ **已完成** - 已实现基于 Cortex-R52 的时钟树
3. **外设基地址**: 更新所有外设的 MMIO 地址
4. **中断号**: 修正 GIC 中断编号映射

#### 需要添加的外设
根据 J6M 数据手册，可能需要添加：

- **CAN 控制器**: 车规级 CAN 总线
- **SPI 控制器**: 高速 SPI 接口
- **I2C 控制器**: I2C 总线控制器
- **GPIO 控制器**: 通用 I/O
- **DMA 控制器**: 直接内存访问
- **以太网控制器**: 网络接口
- **USB 控制器**: USB 主机/设备
- **Camera 接口**: 摄像头输入
- **Display 控制器**: 显示输出
- **AI 加速器接口**: J6M 特有的 AI 处理单元

### 代码结构

```
hw/arm/horizon-j6m.c       - 主板级实现
include/hw/arm/horizon-j6m.h - (可选) 头文件定义
hw/arm/Kconfig             - Kconfig 配置
hw/arm/meson.build         - 构建系统配置
```

### 添加新外设的步骤

1. 在 `horizon_j6m_init()` 中初始化外设对象
2. 配置外设属性 (时钟、IRQ 等)
3. 调用 `sysbus_realize()` 实例化
4. 使用 `sysbus_mmio_map()` 映射到地址空间
5. 使用 `sysbus_connect_irq()` 连接中断

示例 (添加 I2C):
```c
// 在 HorizonJ6MMachineState 结构体中添加
ArmSbconI2CState i2c[2];

// 在 horizon_j6m_init() 中
for (int i = 0; i < 2; i++) {
    g_autofree char *s = g_strdup_printf("i2c%d", i);
    hwaddr baseaddr = 0xA0400000 + i * 0x1000;
    
    object_initialize_child(OBJECT(mms), s, &mms->i2c[i],
                            TYPE_ARM_SBCON_I2C);
    sysbus_realize(SYS_BUS_DEVICE(&mms->i2c[i]), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&mms->i2c[i]), 0, baseaddr);
}
```

## 参考资料

### QEMU 开发文档
- `docs/devel/qom.rst` - QEMU 对象模型
- `docs/devel/memory.rst` - 内存系统
- `docs/devel/clocks.rst` - 时钟模型
- `docs/system/arm/mps3r.rst` - MPS3 参考实现

### Horizon J6M 相关
- **需要**: J6M 技术参考手册 (TRM)
- **需要**: J6M 数据手册
- **需要**: J6M 内存映射文档

### ARM 架构
- ARM Cortex-R52 TRM
- ARM GICv3 Architecture Specification
- ARMv8-R Architecture Reference Manual

## 故障排除

### 常见问题

**Q: 启动时提示 "Unknown machine type"**  
A: 确保已重新编译 QEMU 并且 `CONFIG_HORIZON_J6M=y` 在配置中启用

**Q: 串口无输出**  
A: 检查是否使用了 `-nographic -serial stdio` 参数

**Q: 双核模式下第二个核心未启动**  
A: 这是正常的，第二个核心默认处于断电状态，需要引导代码唤醒

**Q: GDB 无法连接**  
A: 确保使用了 `-s -S` 参数，并且防火墙允许 1234 端口

## TODO 清单

- [ ] 根据 J6M 数据手册更新内存映射
- [ ] 添加 CAN 控制器支持
- [ ] 添加 SPI 控制器支持
- [ ] 添加 I2C 控制器支持
- [ ] 添加 GPIO 控制器支持
- [ ] 添加 DMA 控制器支持
- [x] 实现时钟树配置 ✅
- [ ] 添加设备树生成支持
- [ ] 编写单元测试 (tests/qtest/horizon-j6m-test.c)
- [ ] 更新 IRQ 路由表
- [ ] 实现电源管理功能
- [ ] 添加 AI 加速器模拟接口

## 许可证

本代码遵循 GNU General Public License v2.0 或更高版本。

## 贡献

欢迎提交 Pull Request 来完善 J6M 的模拟实现。请确保：

1. 代码符合 QEMU 编码规范 (`scripts/checkpatch.pl`)
2. 添加适当的注释和文档
3. 更新此 README 文档
4. 提供测试用例（如果可能）

## 联系方式

如有问题，请在项目 Issue 中提出。
