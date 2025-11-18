# MPS3-AN536 地址映射表

**文档生成日期**: 2025年11月18日  
**源文件**: `hw/arm/mps3r.c`  
**平台**: ARM MPS3 with AN536 FPGA image for Cortex-R52

---

## 📝 内存区域 (Memory Regions)

### ATCM (A-Tightly Coupled Memory)
- **起始地址**: `0x00000000`
- **大小**: 32 KB (`0x8000`)
- **类型**: RAM
- **源文件**: `hw/arm/mps3r.c:135-139`

```c
{
    .name = "ATCM",
    .base = 0x00000000,
    .size = 0x00008000,
    .mrindex = 0,
}
```

---

### QSPI Flash
- **起始地址**: `0x08000000`
- **大小**: 8 MB (`0x800000`)
- **类型**: ROM (只读)
- **说明**: We model the QSPI flash as simple ROM for now
- **源文件**: `hw/arm/mps3r.c:140-145`

```c
{
    /* We model the QSPI flash as simple ROM for now */
    .name = "QSPI",
    .base = 0x08000000,
    .size = 0x00800000,
    .flags = IS_ROM,
    .mrindex = 1,
}
```

---

### BRAM (Block RAM)
- **起始地址**: `0x10000000`
- **大小**: 512 KB (`0x80000`)
- **类型**: RAM
- **源文件**: `hw/arm/mps3r.c:146-150`

```c
{
    .name = "BRAM",
    .base = 0x10000000,
    .size = 0x00080000,
    .mrindex = 2,
}
```

---

### DDR (主系统内存)
- **起始地址**: `0x20000000`
- **大小**: 
  - 32位主机: 1 GB (`0x40000000`)
  - 64位主机: 3 GB (`0xC0000000`)
- **类型**: RAM (系统主内存)
- **源文件**: `hw/arm/mps3r.c:151-155`
- **大小定义**: `hw/arm/mps3r.c:61-69`

```c
// 内存定义 (行 151-155)
{
    .name = "DDR",
    .base = 0x20000000,
    .size = MPS3_DDR_SIZE,
    .mrindex = -1,
}

// 大小定义 (行 61-69)
/*
 * The MPS3 DDR is 3GiB, but on a 32-bit host QEMU doesn't permit
 * emulation of that much guest RAM, so artificially make it smaller.
 */
#if HOST_LONG_BITS == 32
#define MPS3_DDR_SIZE (1 * GiB)
#else
#define MPS3_DDR_SIZE (3 * GiB)
#endif
```

---

### CPU0 Tightly Coupled Memory

#### ATCM0
- **起始地址**: `0xee000000`
- **大小**: 32 KB (`0x8000`)
- **类型**: RAM
- **源文件**: `hw/arm/mps3r.c:156-160`

```c
{
    .name = "ATCM0",
    .base = 0xee000000,
    .size = 0x00008000,
    .mrindex = 3,
}
```

#### BTCM0
- **起始地址**: `0xee100000`
- **大小**: 32 KB (`0x8000`)
- **类型**: RAM
- **源文件**: `hw/arm/mps3r.c:161-165`

```c
{
    .name = "BTCM0",
    .base = 0xee100000,
    .size = 0x00008000,
    .mrindex = 4,
}
```

#### CTCM0
- **起始地址**: `0xee200000`
- **大小**: 32 KB (`0x8000`)
- **类型**: RAM
- **源文件**: `hw/arm/mps3r.c:166-170`

```c
{
    .name = "CTCM0",
    .base = 0xee200000,
    .size = 0x00008000,
    .mrindex = 5,
}
```

---

### CPU1 Tightly Coupled Memory

#### ATCM1
- **起始地址**: `0xee400000`
- **大小**: 32 KB (`0x8000`)
- **类型**: RAM
- **源文件**: `hw/arm/mps3r.c:171-175`

```c
{
    .name = "ATCM1",
    .base = 0xee400000,
    .size = 0x00008000,
    .mrindex = 6,
}
```

#### BTCM1
- **起始地址**: `0xee500000`
- **大小**: 32 KB (`0x8000`)
- **类型**: RAM
- **源文件**: `hw/arm/mps3r.c:176-180`

```c
{
    .name = "BTCM1",
    .base = 0xee500000,
    .size = 0x00008000,
    .mrindex = 7,
}
```

#### CTCM1
- **起始地址**: `0xee600000`
- **大小**: 32 KB (`0x8000`)
- **类型**: RAM
- **源文件**: `hw/arm/mps3r.c:181-185`

```c
{
    .name = "CTCM1",
    .base = 0xee600000,
    .size = 0x00008000,
    .mrindex = 8,
}
```

---

## 🖥️ Per-CPU 外设 (每个CPU专用)

### CPU Private RAM
- **起始地址**: `0xe7c01000`
- **大小**: 4 KB (`0x1000`)
- **类型**: RAM
- **源文件**: `hw/arm/mps3r.c:375-377`

```c
/* Per-CPU RAM */
memory_region_init_ram(&mms->cpu_ram[i], NULL, ramname,
                       0x1000, &error_fatal);
memory_region_add_subregion(&mms->cpu_sysmem[i], 0xe7c01000,
                            &mms->cpu_ram[i]);
```

---

### CPU Private UART (UART0/UART1)
- **起始地址**: `0xe7c00000`
- **类型**: CMSDK APB UART
- **时钟频率**: 50 MHz
- **中断**: 
  - TX: PPI 17
  - RX: PPI 16
  - Combined: PPI 18
  - Overflow: PPI 19 (OR-gated)
- **源文件**: `hw/arm/mps3r.c:398-406`

```c
create_uart(mms, i, &mms->cpu_sysmem[i], 0xe7c00000,
            qdev_get_gpio_in(gicdev, intidbase + 17), /* tx */
            qdev_get_gpio_in(gicdev, intidbase + 16), /* rx */
            qdev_get_gpio_in(orgate, 0), /* txover */
            qdev_get_gpio_in(orgate, 1), /* rxover */
            qdev_get_gpio_in(gicdev, intidbase + 18) /* combined */);
```

---

## 🔧 GPIO 控制器

### GPIO0
- **起始地址**: `0xe0000000`
- **大小**: 4 KB (`0x1000`)
- **类型**: CMSDK GPIO (未实现)
- **源文件**: `hw/arm/mps3r.c:424-427`

```c
for (int i = 0; i < 4; i++) {
    /* CMSDK GPIO controllers */
    g_autofree char *s = g_strdup_printf("gpio%d", i);
    create_unimplemented_device(s, 0xe0000000 + i * 0x1000, 0x1000);
}
```

### GPIO1
- **起始地址**: `0xe0001000`
- **大小**: 4 KB (`0x1000`)
- **类型**: CMSDK GPIO (未实现)
- **源文件**: `hw/arm/mps3r.c:424-427`

### GPIO2
- **起始地址**: `0xe0002000`
- **大小**: 4 KB (`0x1000`)
- **类型**: CMSDK GPIO (未实现)
- **源文件**: `hw/arm/mps3r.c:424-427`

### GPIO3
- **起始地址**: `0xe0003000`
- **大小**: 4 KB (`0x1000`)
- **类型**: CMSDK GPIO (未实现)
- **源文件**: `hw/arm/mps3r.c:424-427`

---

## ⏱️ 定时器和看门狗

### Watchdog
- **起始地址**: `0xe0100000`
- **类型**: CMSDK APB Watchdog
- **时钟**: WDOGCLK (50 MHz)
- **中断**: IRQ 0
- **源文件**: `hw/arm/mps3r.c:429-435`

```c
object_initialize_child(OBJECT(mms), "watchdog", &mms->watchdog,
                        TYPE_CMSDK_APB_WATCHDOG);
qdev_connect_clock_in(DEVICE(&mms->watchdog), "WDOGCLK", mms->clk);
sysbus_realize(SYS_BUS_DEVICE(&mms->watchdog), &error_fatal);
sysbus_connect_irq(SYS_BUS_DEVICE(&mms->watchdog), 0,
                   qdev_get_gpio_in(gicdev, 0));
sysbus_mmio_map(SYS_BUS_DEVICE(&mms->watchdog), 0, 0xe0100000);
```

---

### Dual Timer
- **起始地址**: `0xe0101000`
- **类型**: CMSDK APB Dual Timer
- **时钟**: TIMCLK (50 MHz)
- **中断**: 
  - Timer 0: IRQ 3
  - Timer 1: IRQ 1
  - Combined: IRQ 2
- **源文件**: `hw/arm/mps3r.c:437-446`

```c
object_initialize_child(OBJECT(mms), "dualtimer", &mms->dualtimer,
                        TYPE_CMSDK_APB_DUALTIMER);
qdev_connect_clock_in(DEVICE(&mms->dualtimer), "TIMCLK", mms->clk);
sysbus_realize(SYS_BUS_DEVICE(&mms->dualtimer), &error_fatal);
sysbus_connect_irq(SYS_BUS_DEVICE(&mms->dualtimer), 0,
                   qdev_get_gpio_in(gicdev, 3));
sysbus_connect_irq(SYS_BUS_DEVICE(&mms->dualtimer), 1,
                   qdev_get_gpio_in(gicdev, 1));
sysbus_connect_irq(SYS_BUS_DEVICE(&mms->dualtimer), 2,
                   qdev_get_gpio_in(gicdev, 2));
sysbus_mmio_map(SYS_BUS_DEVICE(&mms->dualtimer), 0, 0xe0101000);
```

---

## 🔌 I2C 控制器

### I2C0 (Touch)
- **起始地址**: `0xe0102000`
- **类型**: ARM SBCON I2C
- **用途**: Touch (内部总线)
- **源文件**: `hw/arm/mps3r.c:448-461`

```c
static const hwaddr i2cbase[] = {0xe0102000,    /* Touch */
                                 0xe0103000,    /* Audio */
                                 0xe0107000,    /* Shield0 */
                                 0xe0108000,    /* Shield1 */
                                 0xe0109000};   /* DDR4 EEPROM */
```

### I2C1 (Audio)
- **起始地址**: `0xe0103000`
- **类型**: ARM SBCON I2C
- **用途**: Audio (内部总线)
- **源文件**: `hw/arm/mps3r.c:448-461`

### I2C2 (Shield0)
- **起始地址**: `0xe0107000`
- **类型**: ARM SBCON I2C
- **用途**: Shield0 (用户可用)
- **源文件**: `hw/arm/mps3r.c:448-461`

### I2C3 (Shield1)
- **起始地址**: `0xe0108000`
- **类型**: ARM SBCON I2C
- **用途**: Shield1 (用户可用)
- **源文件**: `hw/arm/mps3r.c:448-461`

### I2C4 (DDR4 EEPROM)
- **起始地址**: `0xe0109000`
- **类型**: ARM SBCON I2C
- **用途**: DDR4 EEPROM (内部总线)
- **源文件**: `hw/arm/mps3r.c:448-461`

---

## 📡 SPI 控制器

### SPI0
- **起始地址**: `0xe0104000`
- **类型**: PL022 SPI
- **中断**: IRQ 22
- **源文件**: `hw/arm/mps3r.c:463-472`

```c
for (int i = 0; i < ARRAY_SIZE(mms->spi); i++) {
    g_autofree char *s = g_strdup_printf("spi%d", i);
    hwaddr baseaddr = 0xe0104000 + i * 0x1000;

    object_initialize_child(OBJECT(mms), s, &mms->spi[i], TYPE_PL022);
    sysbus_realize(SYS_BUS_DEVICE(&mms->spi[i]), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&mms->spi[i]), 0, baseaddr);
    sysbus_connect_irq(SYS_BUS_DEVICE(&mms->spi[i]), 0,
                       qdev_get_gpio_in(gicdev, 22 + i));
}
```

### SPI1
- **起始地址**: `0xe0105000`
- **类型**: PL022 SPI
- **中断**: IRQ 23
- **源文件**: `hw/arm/mps3r.c:463-472`

### SPI2
- **起始地址**: `0xe0106000`
- **类型**: PL022 SPI
- **中断**: IRQ 24
- **源文件**: `hw/arm/mps3r.c:463-472`

---

## 📟 系统控制和配置

### SCC (System Configuration Controller)
- **起始地址**: `0xe0200000`
- **类型**: MPS2 SCC
- **配置**:
  - scc-cfg0: 0
  - scc-cfg4: 0x2
  - scc-aid: 0x00200008
  - scc-id: 0x41055360
- **源文件**: `hw/arm/mps3r.c:474-484`

```c
object_initialize_child(OBJECT(mms), "scc", &mms->scc, TYPE_MPS2_SCC);
qdev_prop_set_uint32(DEVICE(&mms->scc), "scc-cfg0", 0);
qdev_prop_set_uint32(DEVICE(&mms->scc), "scc-cfg4", 0x2);
qdev_prop_set_uint32(DEVICE(&mms->scc), "scc-aid", 0x00200008);
qdev_prop_set_uint32(DEVICE(&mms->scc), "scc-id", 0x41055360);
oscclk = qlist_new();
for (int i = 0; i < ARRAY_SIZE(an536_oscclk); i++) {
    qlist_append_int(oscclk, an536_oscclk[i]);
}
qdev_prop_set_array(DEVICE(&mms->scc), "oscclk", oscclk);
sysbus_realize(SYS_BUS_DEVICE(&mms->scc), &error_fatal);
sysbus_mmio_map(SYS_BUS_DEVICE(&mms->scc), 0, 0xe0200000);
```

---

### I2S Audio
- **起始地址**: `0xe0201000`
- **大小**: 4 KB (`0x1000`)
- **类型**: 未实现
- **源文件**: `hw/arm/mps3r.c:486`

```c
create_unimplemented_device("i2s-audio", 0xe0201000, 0x1000);
```

---

### FPGA IO
- **起始地址**: `0xe0202000`
- **类型**: MPS2 FPGA IO
- **配置**:
  - prescale-clk: 50 MHz
  - num-leds: 10
  - has-switches: true
  - has-dbgctrl: false
- **源文件**: `hw/arm/mps3r.c:488-495`

```c
object_initialize_child(OBJECT(mms), "fpgaio", &mms->fpgaio,
                        TYPE_MPS2_FPGAIO);
qdev_prop_set_uint32(DEVICE(&mms->fpgaio), "prescale-clk", an536_oscclk[1]);
qdev_prop_set_uint32(DEVICE(&mms->fpgaio), "num-leds", 10);
qdev_prop_set_bit(DEVICE(&mms->fpgaio), "has-switches", true);
qdev_prop_set_bit(DEVICE(&mms->fpgaio), "has-dbgctrl", false);
sysbus_realize(SYS_BUS_DEVICE(&mms->fpgaio), &error_fatal);
sysbus_mmio_map(SYS_BUS_DEVICE(&mms->fpgaio), 0, 0xe0202000);
```

---

## 📱 共享串口 (UART 2-5)

### UART2
- **起始地址**: `0xe0205000`
- **大小**: 4 KB (`0x1000`)
- **类型**: CMSDK APB UART
- **时钟频率**: 50 MHz
- **中断**:
  - RX: IRQ 5
  - TX: IRQ 6
  - Combined: IRQ 13
  - Overflow: IRQ 17 (OR-gated)
- **源文件**: `hw/arm/mps3r.c:417-423`

```c
for (int i = 0; i < MPS3R_UART_MAX; i++) {
    hwaddr baseaddr = 0xe0205000 + i * 0x1000;
    int rxirq = 5 + i * 2, txirq = 6 + i * 2, combirq = 13 + i;

    create_uart(mms, i + MPS3R_CPU_MAX, sysmem, baseaddr,
                qdev_get_gpio_in(gicdev, txirq),
                qdev_get_gpio_in(gicdev, rxirq),
                qdev_get_gpio_in(DEVICE(&mms->uart_oflow), i * 2),
                qdev_get_gpio_in(DEVICE(&mms->uart_oflow), i * 2 + 1),
                qdev_get_gpio_in(gicdev, combirq));
}
```

### UART3
- **起始地址**: `0xe0206000`
- **大小**: 4 KB (`0x1000`)
- **中断**:
  - RX: IRQ 7
  - TX: IRQ 8
  - Combined: IRQ 14
  - Overflow: IRQ 17 (OR-gated)
- **源文件**: `hw/arm/mps3r.c:417-423`

### UART4
- **起始地址**: `0xe0207000`
- **大小**: 4 KB (`0x1000`)
- **中断**:
  - RX: IRQ 9
  - TX: IRQ 10
  - Combined: IRQ 15
  - Overflow: IRQ 17 (OR-gated)
- **源文件**: `hw/arm/mps3r.c:417-423`

### UART5
- **起始地址**: `0xe0208000`
- **大小**: 4 KB (`0x1000`)
- **中断**:
  - RX: IRQ 11
  - TX: IRQ 12
  - Combined: IRQ 16
  - Overflow: IRQ 17 (OR-gated)
- **源文件**: `hw/arm/mps3r.c:417-423`

---

## 📺 显示和时钟

### CLCD (Color LCD Controller)
- **起始地址**: `0xe0209000`
- **大小**: 4 KB (`0x1000`)
- **类型**: 未实现
- **源文件**: `hw/arm/mps3r.c:497`

```c
create_unimplemented_device("clcd", 0xe0209000, 0x1000);
```

---

### RTC (Real Time Clock)
- **起始地址**: `0xe020a000`
- **类型**: PL031
- **中断**: IRQ 4
- **源文件**: `hw/arm/mps3r.c:499-503`

```c
object_initialize_child(OBJECT(mms), "rtc", &mms->rtc, TYPE_PL031);
sysbus_realize(SYS_BUS_DEVICE(&mms->rtc), &error_fatal);
sysbus_mmio_map(SYS_BUS_DEVICE(&mms->rtc), 0, 0xe020a000);
sysbus_connect_irq(SYS_BUS_DEVICE(&mms->rtc), 0,
                   qdev_get_gpio_in(gicdev, 4));
```

---

## 🌐 网络和USB

### Ethernet (LAN9118)
- **起始地址**: `0xe0300000`
- **类型**: LAN9118 (模拟LAN9220，软件兼容)
- **中断**: IRQ 18
- **源文件**: `hw/arm/mps3r.c:505-510`

```c
/*
 * In hardware this is a LAN9220; the LAN9118 is software compatible
 * except that it doesn't support the checksum-offload feature.
 */
lan9118_init(0xe0300000,
             qdev_get_gpio_in(gicdev, 18));
```

---

### USB
- **起始地址**: `0xe0301000`
- **大小**: 4 KB (`0x1000`)
- **类型**: 未实现
- **源文件**: `hw/arm/mps3r.c:512`

```c
create_unimplemented_device("usb", 0xe0301000, 0x1000);
```

---

## 💾 QSPI 配置

### QSPI Write Config
- **起始地址**: `0xe0600000`
- **大小**: 4 KB (`0x1000`)
- **类型**: 未实现
- **源文件**: `hw/arm/mps3r.c:513`

```c
create_unimplemented_device("qspi-write-config", 0xe0600000, 0x1000);
```

---

## ⚡ GICv3 中断控制器

### PERIPHBASE (GIC Base)
- **基地址**: `0xf0000000`
- **定义**: `hw/arm/mps3r.c:82`

```c
#define PERIPHBASE 0xf0000000
```

### GIC Distributor
- **起始地址**: `0xf0000000`
- **类型**: ARM GICv3 Distributor
- **源文件**: `hw/arm/mps3r.c:276`

```c
sysbus_mmio_map(SYS_BUS_DEVICE(&mms->gic), 0, PERIPHBASE);
```

### GIC Redistributor
- **起始地址**: `0xf0100000`
- **类型**: ARM GICv3 Redistributor
- **源文件**: `hw/arm/mps3r.c:277`

```c
sysbus_mmio_map(SYS_BUS_DEVICE(&mms->gic), 1, PERIPHBASE + 0x100000);
```

### GIC 配置
- **CPU数量**: 1-2 (可配置)
- **中断数量**: 96 SPIs + 32 内部中断
- **源文件**: `hw/arm/mps3r.c:83, 268-275`

```c
#define NUM_SPIS 96

object_initialize_child(OBJECT(mms), "gic", &mms->gic, TYPE_ARM_GICV3);
gicdev = DEVICE(&mms->gic);
qdev_prop_set_uint32(gicdev, "num-cpu", machine->smp.cpus);
qdev_prop_set_uint32(gicdev, "num-irq", NUM_SPIS + GIC_INTERNAL);
redist_region_count = qlist_new();
qlist_append_int(redist_region_count, machine->smp.cpus);
qdev_prop_set_array(gicdev, "redist-region-count", redist_region_count);
```

---

## 📊 时钟频率配置

所有时钟频率定义在 `hw/arm/mps3r.c:187-195`：

```c
static const int an536_oscclk[] = {
    24000000, /* 24MHz reference for RTC and timers */
    50000000, /* 50MHz ACLK */
    50000000, /* 50MHz MCLK */
    50000000, /* 50MHz GPUCLK */
    24576000, /* 24.576MHz AUDCLK */
    23750000, /* 23.75MHz HDLCDCLK */
    100000000, /* 100MHz DDR4_REF_CLK */
};
```

| 时钟名称 | 频率 | 索引 | 用途 |
|----------|------|------|------|
| **主时钟 (CLK)** | 50 MHz | - | 主系统时钟 (`hw/arm/mps3r.c:131`) |
| **RTC参考时钟** | 24 MHz | 0 | RTC和定时器 |
| **ACLK** | 50 MHz | 1 | 系统总线时钟 |
| **MCLK** | 50 MHz | 2 | 内存时钟 |
| **GPUCLK** | 50 MHz | 3 | GPU时钟 |
| **AUDCLK** | 24.576 MHz | 4 | 音频时钟 |
| **HDLCDCLK** | 23.75 MHz | 5 | LCD时钟 |
| **DDR4_REF_CLK** | 100 MHz | 6 | DDR4参考时钟 |

主时钟定义：
```c
/*
 * Main clock frequency CLK in Hz (50MHz). In the image there are also
 * ACLK, MCLK, GPUCLK and PERIPHCLK at the same frequency; for our
 * model we just roll them all into one.
 */
#define CLK_FRQ 50000000
```

---

## 💻 CPU 配置

### CPU 类型和数量
- **CPU 类型**: Cortex-R52
- **CPU 数量**: 1-2个 (默认1个)
- **最小CPU数**: 1
- **最大CPU数**: 2
- **源文件**: `hw/arm/mps3r.c:596-615`

```c
mc->desc = "ARM MPS3 with AN536 FPGA image for Cortex-R52";
/*
 * In the real FPGA image there are always two cores, but the standard
 * initial setting for the SCC SYSCON 0x000 register is 0x21, meaning
 * that the second core is held in reset and halted. Many images built for
 * the board do not expect the second core to run at startup (especially
 * since on the real FPGA image it is not possible to use LDREX/STREX
 * in RAM between the two cores, so a true SMP setup isn't supported).
 *
 * As QEMU's equivalent of this, we support both -smp 1 and -smp 2,
 * with the default being -smp 1. This seems a more intuitive UI for
 * QEMU users than, for instance, having a machine property to allow
 * the user to set the initial value of the SYSCON 0x000 register.
 */
mc->default_cpus = 1;
mc->min_cpus = 1;
mc->max_cpus = 2;
mc->default_cpu_type = ARM_CPU_TYPE_NAME("cortex-r52");
```

### CPU Private Base
- **PERIPHBASE**: `0xf0000000`
- **源文件**: `hw/arm/mps3r.c:82, 370`

```c
#define PERIPHBASE 0xf0000000

object_property_set_int(mms->cpu[i], "reset-cbar",
                        PERIPHBASE, &error_abort);
```

---

## 📋 宏定义和常量

### 内存相关
- **MPS3R_RAM_MAX**: 9 (`hw/arm/mps3r.c:79`)
- **HOST_LONG_BITS**: 系统相关 (32或64)
- **IS_MAIN**: 1 (主内存标志, `hw/arm/mps3r.c:76`)
- **IS_ROM**: 2 (只读标志, `hw/arm/mps3r.c:77`)

### CPU和外设
- **MPS3R_CPU_MAX**: 2 (`hw/arm/mps3r.c:80`)
- **MPS3R_UART_MAX**: 4 (共享UART数量, `hw/arm/mps3r.c:81`)
- **NUM_SPIS**: 96 (SPI中断数, `hw/arm/mps3r.c:83`)

---

## 内存属性详解

### 内存类型对比

MPS3-AN536 中定义了多种不同类型的内存区域，每种内存都有其特定的用途和性能特点：

#### 1. TCM (Tightly Coupled Memory) - 紧耦合内存

**特点**:
- **零等待访问**: CPU可以在单个时钟周期内访问
- **确定性延迟**: 没有缓存未命中的风险
- **低功耗**: 相比访问外部内存更省电
- **专用访问**: 每个CPU核心有自己的TCM，避免总线竞争

**类型**:
- **ATCM** (0x00000000, 32KB): 
  - Instruction TCM，优化用于存储代码
  - CPU启动时默认从此地址开始执行
  - 可配置为数据存储
  
- **BTCM/CTCM** (每个CPU各32KB):
  - Data TCM，专门用于数据存储
  - B和C是两个独立的数据TCM区域
  - 适合存储关键数据、栈、堆等

**Per-CPU TCM** (0xee000000 - 0xee600000):
- CPU0: ATCM0, BTCM0, CTCM0
- CPU1: ATCM1, BTCM1, CTCM1
- 每个CPU独享，不会有缓存一致性问题

**适用场景**:
- 中断服务程序 (ISR)
- 实时任务代码和数据
- 关键算法的热点代码
- 栈和临时变量

---

#### 2. BRAM (Block RAM) - 块内存

**起始地址**: 0x10000000  
**大小**: 512 KB

**特点**:
- **片上SRAM**: 位于FPGA内部，访问速度快
- **共享访问**: 所有CPU核心可以访问
- **缓存友好**: 通常通过缓存访问
- **中等延迟**: 比TCM慢，比DDR快

**适用场景**:
- 共享数据结构
- DMA缓冲区
- 中等优先级的代码和数据
- 临时工作区

**性能对比**:
```
TCM      < 1 cycle (零等待)
BRAM     ~ 几个 cycles
DDR      ~ 数十到上百 cycles
```

---

#### 3. DDR (Double Data Rate SDRAM) - 主系统内存

**起始地址**: 0x20000000  
**大小**: 1GB (32位主机) / 3GB (64位主机)

**特点**:
- **大容量**: 系统主内存，存储操作系统和应用程序
- **可扩展**: 通过HOST_LONG_BITS编译时配置大小
- **较高延迟**: 需要经过内存控制器
- **带宽高**: 支持burst传输，顺序访问效率高
- **需要刷新**: DRAM需要定期刷新，会影响访问

**源文件**: `hw/arm/mps3r.c:61-69, 151-155`

```c
#if HOST_LONG_BITS == 32
#define MPS3_DDR_SIZE (1 * GiB)  // 32位主机限制
#else
#define MPS3_DDR_SIZE (3 * GiB)  // 64位主机支持更大内存
#endif
```

**适用场景**:
- 操作系统内核
- 应用程序代码和数据
- 文件系统缓存
- 大型数据集

**地址范围计算**:
- 32位主机: 0x20000000 - 0x5FFFFFFF (1GB)
- 64位主机: 0x20000000 - 0xDFFFFFFF (3GB)

---

#### 4. QSPI Flash - 只读存储器

**起始地址**: 0x08000000  
**大小**: 8 MB

**特点**:
- **非易失性**: 掉电后数据保存
- **只读属性**: 在QEMU中配置为ROM (`IS_ROM` flag)
- **启动代码**: 通常存储bootloader和固件
- **XIP (Execute In Place)**: 可以直接执行，无需复制到RAM

**源文件**: `hw/arm/mps3r.c:140-145`

```c
{
    /* We model the QSPI flash as simple ROM for now */
    .name = "QSPI",
    .base = 0x08000000,
    .size = 0x00800000,
    .flags = IS_ROM,  // 只读标志
    .mrindex = 1,
}
```

**适用场景**:
- Bootloader代码
- 固件映像
- 配置数据
- 常量表和字符串

---

#### 5. CPU Private RAM - CPU专用内存

**起始地址**: 0xe7c01000  
**大小**: 4 KB (每个CPU)

**特点**:
- **完全私有**: 每个CPU独立的地址空间
- **不可共享**: 其他CPU无法访问
- **快速访问**: 通过CPU专用总线访问
- **小容量**: 仅4KB，用于关键私有数据

**源文件**: `hw/arm/mps3r.c:375-377`

**适用场景**:
- CPU私有的控制数据
- Per-CPU统计信息
- 临时缓冲区
- CPU本地状态

---

### 内存属性标志

在 `hw/arm/mps3r.c:76-77` 定义：

```c
#define IS_MAIN 1  // 主内存标志 (系统RAM)
#define IS_ROM  2  // 只读内存标志
```

| 内存区域 | IS_MAIN | IS_ROM | mrindex | 说明 |
|---------|---------|--------|---------|------|
| **ATCM** | - | - | 0 | 普通RAM |
| **QSPI** | - | ✓ | 1 | 只读ROM |
| **BRAM** | - | - | 2 | 普通RAM |
| **DDR** | ✓ | - | -1 | 系统主内存 (特殊处理) |
| **ATCM0-1** | - | - | 3,6 | CPU专用RAM |
| **BTCM0-1** | - | - | 4,7 | CPU专用RAM |
| **CTCM0-1** | - | - | 5,8 | CPU专用RAM |

**mrindex = -1** 表示这是系统RAM，由QEMU的机器RAM对象管理，而不是单独的MemoryRegion。

---

### 内存访问性能对比

基于Cortex-R52架构特点：

| 内存类型 | 访问延迟 | 带宽 | 容量 | 缓存 | 用途 |
|---------|---------|------|------|------|------|
| **TCM** | < 1 cycle | 高 | 小 (32KB×6) | 不经过缓存 | 实时代码/数据 |
| **CPU Private RAM** | 几个 cycles | 中 | 极小 (4KB×2) | 视配置 | CPU私有数据 |
| **BRAM** | ~10 cycles | 中高 | 中 (512KB) | 通常经过缓存 | 共享缓冲区 |
| **DDR** | 50-100 cycles | 最高 | 大 (1-3GB) | 必须经过缓存 | 主存储 |
| **QSPI** | 100+ cycles | 低 | 中 (8MB) | 可缓存 | 固件/常量 |

---

### 内存映射策略建议

#### 实时系统配置

```
0x00000000 - ATCM    : 中断向量表、ISR代码
0xee000000 - ATCM0   : CPU0实时任务代码
0xee100000 - BTCM0   : CPU0实时数据、栈
0x10000000 - BRAM    : DMA缓冲区、共享数据
0x20000000 - DDR     : 操作系统、应用程序
0x08000000 - QSPI    : Bootloader、固件
```

#### 通用应用配置

```
0x08000000 - QSPI    : Bootloader
0x20000000 - DDR     : 主程序和数据
0x10000000 - BRAM    : 缓存、临时数据
0x00000000 - ATCM    : 启动代码
```

---

### 内存初始化顺序

源文件: `hw/arm/mps3r.c:348-350`

```c
for (const RAMInfo *ri = mmc->raminfo; ri->name; ri++) {
    MemoryRegion *mr = mr_for_raminfo(mms, ri);
    memory_region_add_subregion(sysmem, ri->base, mr);
}
```

初始化按照 `an536_raminfo` 数组顺序：
1. ATCM (0x00000000)
2. QSPI (0x08000000) - 配置为只读
3. BRAM (0x10000000)
4. DDR (0x20000000) - 使用machine->ram
5. CPU0 TCMs (0xee000000 - 0xee200000)
6. CPU1 TCMs (0xee400000 - 0xee600000)

---

## �📚 参考文档

- **Application Note**: [ARM AN536](https://developer.arm.com/documentation/dai0536/latest/)
- **Cortex-R52 TRM**: [ARM Cortex-R52 Technical Reference Manual](https://developer.arm.com/documentation/100026/latest/)
- **源文件**: `hw/arm/mps3r.c`
- **相关文件**: 
  - M-profile images: `hw/arm/mps2.c`, `hw/arm/mps2tz.c`
  - 设备类型定义: 各个 `hw/` 子目录下的头文件
  - 内存管理: `system/memory.c`, `include/system/memory.h`

---

## 🖥️ QEMU模拟特性说明

### QEMU中的内存访问特性

**重要提示**: QEMU是功能模拟器，不是性能模拟器。在QEMU中，所有内存访问的实际速度是相同的！

#### 访问速度差异

| 内存类型 | 真实硬件延迟 | QEMU模拟延迟 | QEMU实现方式 |
|---------|-------------|-------------|-------------|
| **TCM** | < 1 cycle | **相同** | 直接内存访问 |
| **BRAM** | ~10 cycles | **相同** | 直接内存访问 |
| **DDR** | 50-100 cycles | **相同** | 直接内存访问 |
| **QSPI ROM** | 100+ cycles | **相同** | 直接内存访问（只读） |

**源代码实现**: `hw/arm/mps3r.c:197-211`

```c
static MemoryRegion *mr_for_raminfo(MPS3RMachineState *mms,
                                    const RAMInfo *raminfo)
{
    MemoryRegion *ram;
    // ... 
    memory_region_init_ram(ram, NULL, raminfo->name,
                           raminfo->size, &error_fatal);
    if (raminfo->flags & IS_ROM) {
        memory_region_set_readonly(ram, true);  // 唯一的访问控制差异
    }
    return ram;
}
```

#### QEMU模拟的差异点

##### 1. **访问权限控制** ✅ 已实现

QEMU **确实**模拟了不同的访问权限：

| 内存类型 | 读权限 | 写权限 | QEMU实现 |
|---------|--------|--------|----------|
| **TCM** | ✅ | ✅ | 可读写RAM |
| **BRAM** | ✅ | ✅ | 可读写RAM |
| **DDR** | ✅ | ✅ | 可读写RAM |
| **QSPI** | ✅ | ❌ | 只读ROM (`IS_ROM` flag) |
| **CPU Private RAM** | ✅ | ✅ | 可读写RAM |

**QSPI只读实现**:
```c
{
    .name = "QSPI",
    .base = 0x08000000,
    .size = 0x00800000,
    .flags = IS_ROM,  // 设置只读标志
    .mrindex = 1,
}
```

尝试写入QSPI会触发异常或被忽略（取决于CPU配置）。

##### 2. **访问速度/延迟** ❌ 未模拟

QEMU **不模拟**时序差异：

- ❌ 没有cycle-accurate的时序模拟
- ❌ TCM和DDR访问速度相同
- ❌ 不模拟缓存命中/未命中延迟
- ❌ 不模拟内存刷新周期
- ❌ 不模拟总线仲裁延迟

**实现原理**:
```c
// QEMU的内存访问都是通过这样的直接映射
memory_region_add_subregion(sysmem, ri->base, mr);
// 底层都是对host内存的直接读写，无延迟差异
```

##### 3. **地址空间隔离** ⚠️ 部分实现

**Per-CPU私有地址空间** - 已实现:

源文件: `hw/arm/mps3r.c:352-371`

```c
for (int i = 0; i < machine->smp.cpus; i++) {
    // 每个CPU创建自己的地址空间
    memory_region_init(&mms->cpu_sysmem[i], OBJECT(machine),
                       sysmem_name, UINT64_MAX);
    memory_region_init_alias(&mms->sysmem_alias[i], OBJECT(machine),
                             alias_name, sysmem, 0, UINT64_MAX);
    memory_region_add_subregion_overlap(&mms->cpu_sysmem[i], 0,
                                        &mms->sysmem_alias[i], -1);
    
    mms->cpu[i] = object_new(machine->cpu_type);
    object_property_set_link(mms->cpu[i], "memory",
                             OBJECT(&mms->cpu_sysmem[i]), &error_abort);
    
    // Per-CPU RAM在各自的地址空间
    memory_region_add_subregion(&mms->cpu_sysmem[i], 0xe7c01000,
                                &mms->cpu_ram[i]);
}
```

这意味着：
- ✅ CPU0访问0xe7c01000看到的是自己的Private RAM
- ✅ CPU1访问0xe7c01000看到的是自己的Private RAM
- ✅ 地址相同，但指向不同的物理内存
- ❌ 但没有访问速度差异（都是直接访问）

##### 4. **内存屏障和缓存一致性** ⚠️ 简化实现

QEMU实现：
- ✅ 模拟内存屏障指令（DMB, DSB, ISB）
- ✅ 保证多核间的内存访问顺序
- ❌ 不模拟缓存行为
- ❌ 不模拟缓存一致性协议的延迟

##### 5. **原子操作 (LDREX/STREX)** ✅ 已实现，但与真实硬件有重要差异

**QEMU的实现**:

QEMU **完全支持**ARM的独占访问指令（exclusive access），功能是可信的：

源文件: `docs/devel/multi-thread-tcg.rst:329-365`

```c
// QEMU使用TCG原子辅助函数实现LDREX/STREX
// 保证在多线程TCG环境下的原子性

LDREX指令: 
- 记录独占地址到 cpu_exclusive_addr
- 记录独占值到 cpu_exclusive_val
- 设置独占监视器标记

STREX指令:
- 检查 cpu_exclusive_addr 是否与当前地址匹配
- 检查内存值是否仍等于 cpu_exclusive_val
- 如果匹配，写入新值并返回成功(0)
- 如果不匹配，不写入并返回失败(1)
```

**实现机制**:

QEMU提供了两种原子操作实现：

1. **直接原子指令** (如x86 cmpxchg):
   - 使用主机CPU的原子指令
   - 真正的硬件级原子性
   - 性能好，无ABA问题

2. **独占加载/存储对** (如ARM LDREX/STREX):
   - 使用TCG原子辅助函数
   - 在TCG多线程环境下工作
   - **可能受ABA问题影响**（见下文）

**关键差异 - MPS3-AN536 真实硬件限制**:

源文件: `hw/arm/mps3r.c:227-235, 609-611`

```c
/*
 * There is no defined secondary boot protocol for Linux for the AN536,
 * because real hardware has a restriction that atomic operations between
 * the two CPUs do not function correctly, and so true SMP is not
 * possible.
 */

/*
 * since on the real FPGA image it is not possible to use LDREX/STREX
 * in RAM between the two cores, so a true SMP setup isn't supported.
 */
```

**重要结论**:

| 特性 | QEMU模拟 | MPS3-AN536真实硬件 |
|------|---------|-------------------|
| **单核LDREX/STREX** | ✅ 完全可信 | ✅ 正常工作 |
| **双核间LDREX/STREX (RAM)** | ✅ **可信！** | ❌ **不可靠！** |
| **双核间LDREX/STREX (TCM)** | ✅ 可信 | ⚠️ 未测试/不保证 |
| **原子操作正确性** | ✅ 功能正确 | ❌ 硬件限制 |

**QEMU比真实硬件更好** 🎉:

在这个特定的场景中，QEMU实际上**比真实硬件更强**：

- ✅ QEMU的双核原子操作是可信的
- ✅ QEMU支持真正的SMP（对称多处理）
- ✅ QEMU的LDREX/STREX在多核间工作正常
- ❌ 真实MPS3-AN536硬件有缺陷，双核间原子操作不可靠

**ABA问题说明**:

QEMU文档提到的ABA问题：

```
线程1: LDREX  [addr] -> 读到值A
线程2: STR   [addr], B  -> 写入B
线程2: STR   [addr], A  -> 又写回A
线程1: STREX [addr], C  -> 成功！（但中间值已变过）
```

但是：
- 🎯 实际应用中很少遇到
- 🎯 大多数锁定ABI假设cmpxchg语义
- 🎯 QEMU的实现对常见客户机工作良好

**降级处理机制**:

当TCG无法处理原子操作时（例如：guest原子宽度 > host原子宽度）：

```c
// 触发 EXCP_ATOMIC 异常
// 使用独占锁串行化所有模拟
// 保证绝对的原子性（牺牲性能）
```

---

### 为什么QEMU不模拟时序？

#### 设计目标

QEMU是**功能模拟器**，不是**性能模拟器**：

| 模拟器类型 | 目标 | 速度 | 精度 |
|-----------|------|------|------|
| **QEMU (功能模拟)** | 验证软件功能 | 快 (~100 MIPS) | 功能正确 |
| **Cycle-Accurate模拟器** | 性能分析 | 慢 (~1 MIPS) | 时序精确 |
| **硬件仿真器** | 芯片验证 | 极慢 | 完全精确 |

#### QEMU的优势

```bash
# QEMU可以快速运行完整的Linux系统
qemu-system-arm -M mps3-an536 -kernel zImage -nographic

# 如果模拟时序，速度会慢100倍以上，无法实用
```

#### 适用场景

**QEMU适合**:
- ✅ 开发和调试应用程序
- ✅ 验证功能正确性
- ✅ 测试中断处理逻辑
- ✅ 验证多核同步（功能层面）
- ✅ 快速原型开发
- ✅ **真正的SMP开发（比真实硬件更好！）**
- ✅ **多核原子操作测试（LDREX/STREX可信）**

**QEMU不适合**:
- ❌ 性能优化和调优
- ❌ 精确的实时性测试
- ❌ 缓存优化验证
- ❌ 功耗分析
- ❌ 时序关键的代码验证

---

### 实际影响

#### 对开发的影响

**好消息**: 
- 🎯 内存访问权限是正确的（ROM真的是只读）
- 🎯 地址映射是准确的
- 🎯 功能行为是正确的
- 🎯 多核隔离是有效的

**需要注意**:
- ⚠️ 在QEMU上运行流畅 ≠ 在真实硬件上性能好
- ⚠️ 需要在真实硬件上验证时序敏感代码
- ⚠️ TCM的性能优势在QEMU中体现不出来
- ⚠️ 实时性测试结果不可信

#### 示例代码影响

```c
// 这段代码在QEMU和真实硬件上功能相同，但性能不同

// QEMU: 两个循环速度相同
// 真实硬件: TCM版本快5-10倍

// 版本1: 使用DDR (0x20000000)
void process_data_ddr(void) {
    volatile uint32_t *data = (uint32_t *)0x20000000;
    for (int i = 0; i < 1000; i++) {
        data[i] = i * 2;  // QEMU: 快, 硬件: 慢 (DDR延迟)
    }
}

// 版本2: 使用TCM (0xee100000)
void process_data_tcm(void) {
    volatile uint32_t *data = (uint32_t *)0xee100000;
    for (int i = 0; i < 1000; i++) {
        data[i] = i * 2;  // QEMU: 快, 硬件: 快 (TCM零延迟)
    }
}
```

#### 只读保护验证

```c
// 这个在QEMU和真实硬件上行为相同 ✅

// 尝试写入QSPI ROM
volatile uint32_t *qspi = (uint32_t *)0x08000000;
*qspi = 0x12345678;  // ❌ 在QEMU和真实硬件都会失败

// 写入RAM
volatile uint32_t *ram = (uint32_t *)0x20000000;
*ram = 0x12345678;   // ✅ 在QEMU和真实硬件都会成功
```

#### 原子操作验证

```c
// 多核原子操作：QEMU比真实硬件更可靠！

// 版本1: 使用LDREX/STREX实现自旋锁 (ARM汇编)
void spinlock_acquire(volatile uint32_t *lock) {
    uint32_t tmp, result;
    asm volatile(
        "1: ldrex   %0, [%2]        \n"  // 独占加载
        "   cmp     %0, #0          \n"  // 检查是否已锁定
        "   bne     1b              \n"  // 如果锁定，重试
        "   mov     %0, #1          \n"  // 准备写入1
        "   strex   %1, %0, [%2]    \n"  // 独占存储
        "   cmp     %1, #0          \n"  // 检查是否成功
        "   bne     1b              \n"  // 如果失败，重试
        "   dmb                     \n"  // 内存屏障
        : "=&r" (tmp), "=&r" (result)
        : "r" (lock)
        : "cc", "memory"
    );
}

void spinlock_release(volatile uint32_t *lock) {
    asm volatile(
        "dmb        \n"              // 内存屏障
        "str %1, [%0] \n"            // 释放锁
        :
        : "r" (lock), "r" (0)
        : "memory"
    );
}

// 在QEMU中：
// CPU0和CPU1可以正确使用这个锁 ✅
// 原子操作保证互斥访问

// 在真实MPS3-AN536硬件中：
// CPU0和CPU1之间LDREX/STREX不可靠 ❌
// 可能导致两个核心同时获得锁！

// 版本2: 使用C11原子操作 (推荐用于QEMU)
#include <stdatomic.h>

atomic_uint shared_counter = 0;

void increment_counter(void) {
    // QEMU: 完全可信 ✅
    // 真实硬件: 取决于编译器实现
    atomic_fetch_add(&shared_counter, 1);
}

// 版本3: 比较交换示例
bool try_lock(atomic_uint *lock) {
    uint32_t expected = 0;
    // 如果*lock == 0，设置为1并返回true
    // 如果*lock != 0，返回false
    return atomic_compare_exchange_strong(lock, &expected, 1);
    // QEMU: 完全原子，多核安全 ✅
    // 真实MPS3-AN536: 双核间不保证 ❌
}
```

#### QEMU vs 真实硬件的SMP差异

```c
// 场景：双核共享数据
volatile uint32_t shared_data = 0;
volatile uint32_t lock = 0;

// CPU0执行:
void cpu0_task(void) {
    spinlock_acquire(&lock);
    shared_data = 0xDEADBEEF;
    spinlock_release(&lock);
}

// CPU1执行:
void cpu1_task(void) {
    spinlock_acquire(&lock);
    uint32_t value = shared_data;
    spinlock_release(&lock);
}

// QEMU环境 (qemu-system-arm -M mps3-an536 -smp 2):
// ✅ 锁工作正常
// ✅ 不会出现数据竞争
// ✅ shared_data访问是互斥的
// 🎯 可以用于开发真正的SMP应用

// 真实MPS3-AN536硬件 (双核模式):
// ❌ 锁可能失效
// ❌ 可能出现数据竞争
// ❌ shared_data可能被同时访问
// ⚠️ 官方建议：不要使用双核SMP模式

// 推荐策略:
// 1. 在QEMU中开发和测试SMP代码 ✅
// 2. 在真实硬件上只使用单核 ✅
// 3. 或使用消息传递而非共享内存（如果必须用双核）⚠️
```

---

### 性能测试建议

如果需要准确的性能数据：

1. **使用真实硬件**: MPS3开发板
2. **使用cycle-accurate模拟器**: ARM Fast Models
3. **使用性能分析工具**: ARM DS-5, Lauterbach TRACE32
4. **基于文档估算**: 参考Cortex-R52 TRM的时序数据

**QEMU的作用**: 快速开发和功能验证，然后再在真实硬件上进行性能优化。

---

## ⚠️ 注意事项

### QEMU模拟相关

1. **性能测试无效**: QEMU不模拟内存访问延迟，所有内存访问速度相同，性能测试结果不准确。
2. **功能验证有效**: 内存访问权限（读写/只读）、地址映射、中断处理等功能是准确的。
3. **时序不准确**: 实时性相关的代码需要在真实硬件上验证，QEMU的时序不可信。
4. **原子操作可信**: QEMU的LDREX/STREX等原子操作在多核环境下**完全可信**，甚至比真实MPS3-AN536硬件更可靠！
5. **SMP开发推荐**: 可以在QEMU中开发和测试真正的双核SMP应用，但在真实硬件上应使用单核模式。

### 硬件相关

6. **未实现的外设**: 标记为"未实现"的外设在QEMU中仅作为占位符存在，访问这些地址不会产生实际功能。
7. **DDR大小**: 根据主机架构不同，DDR大小会自动调整（32位主机1GB，64位主机3GB）。
8. **中断汇聚**: 所有UART溢出中断被OR门汇聚到IRQ 17。
9. **I2C总线**: I2C0、I2C1、I2C4为内部总线，不允许用户创建设备；I2C2、I2C3可供用户使用。
10. **双核支持**: 虽然硬件支持双核，但由于真实硬件的限制（CPU间原子操作不可靠），默认配置为单核，可通过 `-smp 2` 启用双核。
11. **以太网控制器**: 实际硬件使用LAN9220，QEMU模拟使用软件兼容的LAN9118（不支持校验和卸载功能）。

### 原子操作特别说明

12. **真实硬件限制**: MPS3-AN536的真实FPGA实现有硬件缺陷，**双核间的LDREX/STREX不可靠**，不支持真正的SMP。
13. **QEMU优势**: QEMU没有这个硬件限制，原子操作在多核间工作正常，可用于SMP开发和测试。
14. **部署策略**: 在QEMU中开发多核代码后，部署到真实硬件时应：
    - 使用单核模式（推荐）
    - 或改用消息传递机制而非共享内存同步
    - 或在真实硬件上进行充分的压力测试

---

**文档版本**: 1.0  
**最后更新**: 2025年11月18日  
**基于QEMU源码**: `hw/arm/mps3r.c`
