# Horizon J6M QEMU 适配快速指南

## 文件清单

已创建以下文件用于 Horizon J6M 双核 Cortex-R52 的 QEMU 模拟：

### 源代码文件
1. `hw/arm/horizon-j6m.c` - 主板实现 (约 500 行)
2. `include/hw/arm/horizon-j6m.h` - 头文件定义 (可选，当前使用内联定义)

### 配置文件
3. `hw/arm/Kconfig` - 添加了 `CONFIG_HORIZON_J6M` 配置项
4. `hw/arm/meson.build` - 添加了编译规则

### 文档
5. `docs/horizon-j6m-board.md` - 完整使用文档
6. 本文件 - 快速参考

## 核心特性

### 基于 MPS3 AN536 模板
- ✅ 双核 Cortex-R52 支持
- ✅ GICv3 中断控制器
- ✅ 4 个 UART (CMSDK APB UART)
- ✅ 看门狗定时器
- ✅ 双定时器
- ✅ 灵活的内存布局配置

### 待补充功能
根据 J6M 实际硬件规格，需要更新：
- ⚠️ 内存映射地址 (当前为占位符)
- ⚠️ 时钟频率 (当前默认 800MHz)
- ⚠️ 外设 MMIO 地址
- ⚠️ IRQ 编号映射
- ❌ CAN 控制器
- ❌ 高速 SPI
- ❌ I2C 总线
- ❌ GPIO
- ❌ DMA
- ❌ AI 加速器接口

## 关键代码位置

### 1. 内存映射配置
文件: `hw/arm/horizon-j6m.c`  
位置: `j6m_raminfo[]` 数组 (约 110 行)

```c
static const RAMInfo j6m_raminfo[] = {
    {
        .name = "tcm0",
        .base = 0x00000000,  // ⚠️ 根据数据手册修改
        .size = 0x00020000,
        .mrindex = 0,
    },
    // ... 其他内存区域
};
```

### 2. 时钟配置
文件: `hw/arm/horizon-j6m.c`  
位置: `j6m_oscclk[]` 数组 (约 149 行) 和 `CLK_FRQ` 宏 (约 64 行)

```c
#define CLK_FRQ 800000000  // ⚠️ 主频设置

static const int j6m_oscclk[] = {
    24000000,   // ⚠️ 参考时钟
    800000000,  // ⚠️ 主时钟
    400000000,  // ⚠️ 外设时钟
};
```

### 3. 外设初始化
文件: `hw/arm/horizon-j6m.c`  
位置: `horizon_j6m_init()` 函数 (约 340 行)

UART 创建示例 (约 375 行):
```c
for (int i = 0; i < HORIZON_J6M_UART_MAX; i++) {
    hwaddr baseaddr = 0xA0100000 + i * 0x1000;  // ⚠️ 修改基地址
    int rxirq = 5 + i * 2;  // ⚠️ 修改 IRQ 编号
    // ...
}
```

### 4. GIC 配置
文件: `hw/arm/horizon-j6m.c`  
位置: `create_gic()` 函数 (约 185 行)

```c
#define PERIPHBASE 0xF0000000  // ⚠️ GIC 基地址
#define NUM_SPIS 128           // ⚠️ SPI 中断数量
```

## 修改指南

### 步骤 1: 获取 J6M 数据手册
需要以下关键信息：
- [ ] CPU 主频和时钟树
- [ ] 完整内存映射
- [ ] 外设列表及 MMIO 地址
- [ ] GIC 配置 (基地址、中断数量)
- [ ] 各外设的 IRQ 编号

### 步骤 2: 更新内存映射
在 `j6m_raminfo[]` 中：
1. 更新 TCM 地址和大小
2. 更新 SRAM 配置
3. 设置正确的 DDR 基地址和大小

### 步骤 3: 配置时钟
1. 修改 `CLK_FRQ` 宏 (主时钟)
2. 更新 `j6m_oscclk[]` 数组 (振荡器频率)
3. 在 `horizon_j6m_init()` 中使用 `clock_set_hz()` 设置

### 步骤 4: 添加外设

#### 添加 SPI 示例
```c
// 1. 在结构体中添加
struct HorizonJ6MMachineState {
    // ...
    PL022State spi[2];  // 添加 SPI
};

// 2. 在 horizon_j6m_init() 中初始化
for (int i = 0; i < 2; i++) {
    g_autofree char *s = g_strdup_printf("spi%d", i);
    hwaddr baseaddr = 0xA0500000 + i * 0x1000;  // 根据数据手册设置
    
    object_initialize_child(OBJECT(mms), s, &mms->spi[i], TYPE_PL022);
    sysbus_realize(SYS_BUS_DEVICE(&mms->spi[i]), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&mms->spi[i]), 0, baseaddr);
    sysbus_connect_irq(SYS_BUS_DEVICE(&mms->spi[i]), 0,
                       qdev_get_gpio_in(gicdev, 30 + i));  // 根据数据手册设置 IRQ
}

// 3. 在 Kconfig 中添加依赖
config HORIZON_J6M
    select PL022
```

### 步骤 5: 测试
```bash
# 重新配置和编译
cd build
rm -rf *
../configure --target-list=arm-softmmu
make -j$(nproc)

# 测试启动
./qemu-system-arm -M horizon-j6m -m 2G -nographic
```

## 常用 QEMU 对象类型

### 外设类型常量
```c
TYPE_CMSDK_APB_UART          // UART
TYPE_CMSDK_APB_WATCHDOG      // 看门狗
TYPE_CMSDK_APB_DUALTIMER     // 定时器
TYPE_ARM_GICV3               // GICv3
TYPE_PL022                   // SPI
TYPE_ARM_SBCON_I2C           // I2C
TYPE_IMX_GPIO                // GPIO
TYPE_PL061                   // GPIO (PL061)
```

### 常用 API
```c
// 时钟
clock_new(parent, name)
clock_set_hz(clk, freq)
qdev_connect_clock_in(dev, name, clk)

// 设备创建
object_initialize_child(parent, name, child, type)
sysbus_realize(sbd, errp)
sysbus_mmio_map(sbd, region, addr)
sysbus_connect_irq(sbd, irq_num, qemu_irq)

// 中断
qdev_get_gpio_in(dev, irq)
qdev_connect_gpio_out(dev, n, irq)

// 内存
memory_region_init_ram(mr, owner, name, size, errp)
memory_region_add_subregion(mr, offset, submr)
```

## 编译和调试技巧

### 启用详细日志
```bash
./qemu-system-arm -M horizon-j6m \
    -d guest_errors,unimp \
    -D qemu.log \
    -nographic
```

### GDB 调试
```bash
# 终端 1: 启动 QEMU
./qemu-system-arm -M horizon-j6m -s -S -nographic

# 终端 2: GDB
arm-none-eabi-gdb
(gdb) target remote :1234
(gdb) b horizon_j6m_init
(gdb) c
```

### 检查代码风格
```bash
./scripts/checkpatch.pl --no-tree -f hw/arm/horizon-j6m.c
```

## 下一步

1. **获取文档**: 联系地平线获取 J6M TRM
2. **更新地址**: 根据文档修改所有 `⚠️` 标记的地址
3. **添加外设**: 根据需求添加 CAN、SPI、I2C 等
4. **编写测试**: 在 `tests/qtest/` 下添加测试用例
5. **设备树**: 生成或适配 J6M 的设备树文件

## 参考实现

查看以下文件获取灵感：
- `hw/arm/mps3r.c` - 双核 R52 参考
- `hw/arm/sabrelite.c` - i.MX6 参考 (更复杂的 SoC)
- `hw/arm/virt.c` - 完整的虚拟化平台实现

## 问题排查

如果遇到问题，检查：
1. ✅ Kconfig 中已添加 `CONFIG_HORIZON_J6M=y`
2. ✅ meson.build 中已添加编译规则
3. ✅ 所有 `#include` 头文件正确
4. ✅ 结构体大小和对齐正确
5. ✅ 中断编号在有效范围内 (0 到 NUM_SPIS + GIC_INTERNAL)
6. ✅ 内存区域没有重叠

祝适配顺利！ 🚀
