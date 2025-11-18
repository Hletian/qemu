/*
 * Horizon Robotics J6M Board Emulation
 *
 * Copyright (c) 2025
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * Horizon J6M SoC features dual Cortex-R52 cores
 * Reference: Based on ARM MPS3 AN536 implementation
 */

#include "qemu/osdep.h"
#include "qemu/units.h"
#include "qapi/error.h"
#include "qobject/qlist.h"
#include "system/address-spaces.h"
#include "cpu.h"
#include "system/system.h"
#include "hw/boards.h"
#include "hw/or-irq.h"
#include "hw/qdev-clock.h"
#include "hw/qdev-properties.h"
#include "hw/arm/boot.h"
#include "hw/arm/bsa.h"
#include "hw/arm/machines-qom.h"
#include "hw/char/cmsdk-apb-uart.h"
#include "hw/i2c/arm_sbcon_i2c.h"
#include "hw/intc/arm_gicv3.h"
#include "hw/misc/mps2-scc.h"
#include "hw/misc/mps2-fpgaio.h"
#include "hw/misc/unimp.h"
#include "hw/net/lan9118.h"
#include "hw/rtc/pl031.h"
#include "hw/ssi/pl022.h"
#include "hw/timer/cmsdk-apb-dualtimer.h"
#include "hw/watchdog/cmsdk-apb-watchdog.h"

/* Define the layout of RAM in the board */
typedef struct RAMInfo {
    const char *name;
    hwaddr base;
    hwaddr size;
    int mrindex; /* index into rams[]; -1 for the system RAM block */
    int flags;
} RAMInfo;

/*
 * Flag values:
 * IS_MAIN: this is the main machine RAM
 * IS_ROM: this area is read-only
 */
#define IS_MAIN 1
#define IS_ROM 2

#define HORIZON_J6M_RAM_MAX 8
#define HORIZON_J6M_CPU_MAX 2
#define HORIZON_J6M_UART_MAX 4

#define PERIPHBASE 0xF0000000
#define NUM_SPIS 128

/*
 * Main clock frequency: 1GHz for Cortex-R52 cores
 * This is a typical high-performance clock for automotive AI SoCs
 */
#define CLK_FRQ 1000000000

struct HorizonJ6MMachineClass {
    MachineClass parent;
    const RAMInfo *raminfo;
    hwaddr loader_start;
};

struct HorizonJ6MMachineState {
    MachineState parent;
    struct arm_boot_info bootinfo;
    MemoryRegion ram[HORIZON_J6M_RAM_MAX];
    Object *cpu[HORIZON_J6M_CPU_MAX];
    MemoryRegion cpu_sysmem[HORIZON_J6M_CPU_MAX];
    MemoryRegion sysmem_alias[HORIZON_J6M_CPU_MAX];
    MemoryRegion cpu_tcm[HORIZON_J6M_CPU_MAX];
    GICv3State gic;
    CMSDKAPBUART uart[HORIZON_J6M_UART_MAX];
    OrIRQState uart_oflow;
    CMSDKAPBWatchdog watchdog;
    CMSDKAPBDualTimer dualtimer;
    Clock *clk;
};

#define TYPE_HORIZON_J6M_MACHINE MACHINE_TYPE_NAME("horizon-j6m")
OBJECT_DECLARE_TYPE(HorizonJ6MMachineState, HorizonJ6MMachineClass, HORIZON_J6M_MACHINE)

/* 
 * Memory map for Horizon J6M (placeholder - adjust based on actual SoC)
 * TODO: Update these addresses based on J6M datasheet
 */
static const RAMInfo j6m_raminfo[] = {
    {
        /* TCM for Core 0 */
        .name = "tcm0",
        .base = 0x00000000,
        .size = 0x00020000,  /* 128KB */
        .mrindex = 0,
    }, {
        /* TCM for Core 1 */
        .name = "tcm1",
        .base = 0x00100000,
        .size = 0x00020000,  /* 128KB */
        .mrindex = 1,
    }, {
        /* Internal SRAM */
        .name = "sram",
        .base = 0x10000000,
        .size = 0x00100000,  /* 1MB */
        .mrindex = 2,
    }, {
        /* Main DDR (system memory) */
        .name = "ddr",
        .base = 0x40000000,
        .size = 0x80000000,  /* 2GB */
        .mrindex = -1,
        .flags = IS_MAIN,
    }, {
        .name = NULL,
    }
};

/*
 * Oscillator clock frequencies for Horizon J6M SoC
 * Based on typical automotive AI SoC clock architecture
 *
 * Clock tree structure:
 * - REFCLK: Reference oscillator for PLLs and RTC
 * - CPUCLK: CPU core clock (1GHz for dual Cortex-R52)
 * - PERIPHCLK: High-speed peripheral bus clock
 * - AXCLK: AXI bus clock for DMA and interconnect
 * - CANCLK: CAN bus clock (80MHz typical for automotive)
 * - DDR4CLK: DDR4 memory interface clock
 * - UARTCLK: UART reference clock
 *
 * These frequencies represent a realistic automotive AI SoC design
 * suitable for ADAS (Advanced Driver Assistance Systems) applications.
 */
static const int j6m_oscclk[] = {
    24000000,   /* [0] REFCLK: 24MHz reference oscillator */
    1000000000, /* [1] CPUCLK: 1GHz CPU clock (Cortex-R52) */
    500000000,  /* [2] PERIPHCLK: 500MHz peripheral bus clock */
    400000000,  /* [3] AXCLK: 400MHz AXI interconnect clock */
    80000000,   /* [4] CANCLK: 80MHz CAN bus clock */
    800000000,  /* [5] DDR4CLK: 800MHz DDR4 reference clock */
    48000000,   /* [6] UARTCLK: 48MHz UART clock */
};

static MemoryRegion *mr_for_raminfo(HorizonJ6MMachineState *mms,
                                    const RAMInfo *raminfo)
{
    MemoryRegion *ram;

    if (raminfo->mrindex < 0) {
        /* This is the main system RAM */
        MachineState *machine = MACHINE(mms);
        assert(!(raminfo->flags & IS_ROM));
        return machine->ram;
    }

    assert(raminfo->mrindex < HORIZON_J6M_RAM_MAX);
    ram = &mms->ram[raminfo->mrindex];

    memory_region_init_ram(ram, NULL, raminfo->name,
                           raminfo->size, &error_fatal);
    if (raminfo->flags & IS_ROM) {
        memory_region_set_readonly(ram, true);
    }
    return ram;
}

/*
 * Secondary CPU boot handling
 * For now, we follow the MPS3 approach where secondary CPU
 * is powered off by default (can be controlled with -smp)
 */
static void j6m_write_secondary_boot(ARMCPU *cpu,
                                    const struct arm_boot_info *info)
{
    /* Power off secondary CPUs - they will be started by firmware */
    for (CPUState *cs = first_cpu; cs; cs = CPU_NEXT(cs)) {
        if (cs != first_cpu) {
            object_property_set_bool(OBJECT(cs), "start-powered-off", true,
                                     &error_abort);
        }
    }
}

static void j6m_secondary_cpu_reset(ARMCPU *cpu,
                                   const struct arm_boot_info *info)
{
    /* Secondary CPU reset hook - currently no special handling needed */
}

static void create_gic(HorizonJ6MMachineState *mms, MemoryRegion *sysmem)
{
    MachineState *machine = MACHINE(mms);
    DeviceState *gicdev;
    QList *redist_region_count;

    object_initialize_child(OBJECT(mms), "gic", &mms->gic, TYPE_ARM_GICV3);
    gicdev = DEVICE(&mms->gic);
    qdev_prop_set_uint32(gicdev, "num-cpu", machine->smp.cpus);
    qdev_prop_set_uint32(gicdev, "num-irq", NUM_SPIS + GIC_INTERNAL);
    redist_region_count = qlist_new();
    qlist_append_int(redist_region_count, machine->smp.cpus);
    qdev_prop_set_array(gicdev, "redist-region-count", redist_region_count);
    object_property_set_link(OBJECT(&mms->gic), "sysmem",
                             OBJECT(sysmem), &error_fatal);
    sysbus_realize(SYS_BUS_DEVICE(&mms->gic), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&mms->gic), 0, PERIPHBASE);
    sysbus_mmio_map(SYS_BUS_DEVICE(&mms->gic), 1, PERIPHBASE + 0x100000);

    /* Connect CPU timer and GIC interrupts */
    for (int i = 0; i < machine->smp.cpus; i++) {
        DeviceState *cpudev = DEVICE(mms->cpu[i]);
        SysBusDevice *gicsbd = SYS_BUS_DEVICE(&mms->gic);
        int intidbase = NUM_SPIS + i * GIC_INTERNAL;
        const int timer_irq[] = {
            [GTIMER_PHYS] = ARCH_TIMER_NS_EL1_IRQ,
            [GTIMER_VIRT] = ARCH_TIMER_VIRT_IRQ,
            [GTIMER_HYP]  = ARCH_TIMER_NS_EL2_IRQ,
        };

        for (int irq = 0; irq < ARRAY_SIZE(timer_irq); irq++) {
            qdev_connect_gpio_out(cpudev, irq,
                                  qdev_get_gpio_in(gicdev,
                                                   intidbase + timer_irq[irq]));
        }

        qdev_connect_gpio_out_named(cpudev, "gicv3-maintenance-interrupt", 0,
                                    qdev_get_gpio_in(gicdev,
                                                     intidbase + ARCH_GIC_MAINT_IRQ));

        qdev_connect_gpio_out_named(cpudev, "pmu-interrupt", 0,
                                    qdev_get_gpio_in(gicdev,
                                                     intidbase + VIRTUAL_PMU_IRQ));

        sysbus_connect_irq(gicsbd, i,
                           qdev_get_gpio_in(cpudev, ARM_CPU_IRQ));
        sysbus_connect_irq(gicsbd, i + machine->smp.cpus,
                           qdev_get_gpio_in(cpudev, ARM_CPU_FIQ));
        sysbus_connect_irq(gicsbd, i + 2 * machine->smp.cpus,
                           qdev_get_gpio_in(cpudev, ARM_CPU_VIRQ));
        sysbus_connect_irq(gicsbd, i + 3 * machine->smp.cpus,
                           qdev_get_gpio_in(cpudev, ARM_CPU_VFIQ));
    }
}

static void create_uart(HorizonJ6MMachineState *mms, int uartno,
                        MemoryRegion *mem, hwaddr baseaddr,
                        qemu_irq txirq, qemu_irq rxirq,
                        qemu_irq txoverirq, qemu_irq rxoverirq,
                        qemu_irq combirq)
{
    g_autofree char *s = g_strdup_printf("uart%d", uartno);
    SysBusDevice *sbd;

    assert(uartno < ARRAY_SIZE(mms->uart));
    object_initialize_child(OBJECT(mms), s, &mms->uart[uartno],
                            TYPE_CMSDK_APB_UART);
    qdev_prop_set_uint32(DEVICE(&mms->uart[uartno]), "pclk-frq", CLK_FRQ);
    qdev_prop_set_chr(DEVICE(&mms->uart[uartno]), "chardev", serial_hd(uartno));
    sbd = SYS_BUS_DEVICE(&mms->uart[uartno]);
    sysbus_realize(sbd, &error_fatal);
    memory_region_add_subregion(mem, baseaddr,
                                sysbus_mmio_get_region(sbd, 0));
    sysbus_connect_irq(sbd, 0, txirq);
    sysbus_connect_irq(sbd, 1, rxirq);
    sysbus_connect_irq(sbd, 2, txoverirq);
    sysbus_connect_irq(sbd, 3, rxoverirq);
    sysbus_connect_irq(sbd, 4, combirq);
}

static void horizon_j6m_init(MachineState *machine)
{
    HorizonJ6MMachineState *mms = HORIZON_J6M_MACHINE(machine);
    HorizonJ6MMachineClass *mmc = HORIZON_J6M_MACHINE_GET_CLASS(mms);
    MemoryRegion *sysmem = get_system_memory();
    DeviceState *gicdev;

    /* Create main clock */
    mms->clk = clock_new(OBJECT(machine), "CLK");
    clock_set_hz(mms->clk, CLK_FRQ);

    /* Initialize RAM regions */
    for (const RAMInfo *ri = mmc->raminfo; ri->name; ri++) {
        MemoryRegion *mr = mr_for_raminfo(mms, ri);
        memory_region_add_subregion(sysmem, ri->base, mr);
    }

    /* Create CPUs */
    assert(machine->smp.cpus <= HORIZON_J6M_CPU_MAX);
    for (int i = 0; i < machine->smp.cpus; i++) {
        g_autofree char *sysmem_name = g_strdup_printf("cpu-%d-memory", i);
        g_autofree char *ramname = g_strdup_printf("cpu-%d-tcm", i);
        g_autofree char *alias_name = g_strdup_printf("sysmem-alias-%d", i);

        /* Create per-CPU memory view */
        memory_region_init(&mms->cpu_sysmem[i], OBJECT(machine),
                           sysmem_name, UINT64_MAX);
        memory_region_init_alias(&mms->sysmem_alias[i], OBJECT(machine),
                                 alias_name, sysmem, 0, UINT64_MAX);
        memory_region_add_subregion_overlap(&mms->cpu_sysmem[i], 0,
                                            &mms->sysmem_alias[i], -1);

        mms->cpu[i] = object_new(machine->cpu_type);
        object_property_set_link(mms->cpu[i], "memory",
                                 OBJECT(&mms->cpu_sysmem[i]), &error_abort);
        object_property_set_int(mms->cpu[i], "reset-cbar",
                                PERIPHBASE, &error_abort);
        qdev_realize(DEVICE(mms->cpu[i]), NULL, &error_fatal);
        object_unref(mms->cpu[i]);

        /* Per-CPU TCM (if needed, adjust based on actual memory map) */
        memory_region_init_ram(&mms->cpu_tcm[i], NULL, ramname,
                               0x1000, &error_fatal);
        memory_region_add_subregion(&mms->cpu_sysmem[i], 0xA0000000 + i * 0x1000,
                                    &mms->cpu_tcm[i]);
    }

    /* Create GIC */
    create_gic(mms, sysmem);
    gicdev = DEVICE(&mms->gic);

    /* Create UARTs
     * TODO: Adjust addresses and IRQ numbers based on J6M datasheet
     */
    object_initialize_child(OBJECT(mms), "uart-oflow-orgate",
                            &mms->uart_oflow, TYPE_OR_IRQ);
    qdev_prop_set_uint32(DEVICE(&mms->uart_oflow), "num-lines",
                         HORIZON_J6M_UART_MAX * 2);
    qdev_realize(DEVICE(&mms->uart_oflow), NULL, &error_fatal);
    qdev_connect_gpio_out(DEVICE(&mms->uart_oflow), 0,
                          qdev_get_gpio_in(gicdev, 17));

    for (int i = 0; i < HORIZON_J6M_UART_MAX; i++) {
        hwaddr baseaddr = 0xA0100000 + i * 0x1000;
        int rxirq = 5 + i * 2, txirq = 6 + i * 2, combirq = 13 + i;

        create_uart(mms, i, sysmem, baseaddr,
                    qdev_get_gpio_in(gicdev, txirq),
                    qdev_get_gpio_in(gicdev, rxirq),
                    qdev_get_gpio_in(DEVICE(&mms->uart_oflow), i * 2),
                    qdev_get_gpio_in(DEVICE(&mms->uart_oflow), i * 2 + 1),
                    qdev_get_gpio_in(gicdev, combirq));
    }

    /* Create watchdog timer */
    object_initialize_child(OBJECT(mms), "watchdog", &mms->watchdog,
                            TYPE_CMSDK_APB_WATCHDOG);
    qdev_connect_clock_in(DEVICE(&mms->watchdog), "WDOGCLK", mms->clk);
    sysbus_realize(SYS_BUS_DEVICE(&mms->watchdog), &error_fatal);
    sysbus_connect_irq(SYS_BUS_DEVICE(&mms->watchdog), 0,
                       qdev_get_gpio_in(gicdev, 0));
    sysbus_mmio_map(SYS_BUS_DEVICE(&mms->watchdog), 0, 0xA0200000);

    /* Create dual timer */
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
    sysbus_mmio_map(SYS_BUS_DEVICE(&mms->dualtimer), 0, 0xA0201000);

    /* TODO: Add more J6M-specific peripherals here:
     * - CAN controllers
     * - SPI controllers
     * - I2C controllers
     * - GPIO controllers
     * - DMA controllers
     * - etc.
     */

    /* Create placeholder for unimplemented devices */
    create_unimplemented_device("j6m-periph", 0xA0300000, 0x100000);

    /* Setup boot info */
    mms->bootinfo.ram_size = machine->ram_size;
    mms->bootinfo.board_id = -1;
    mms->bootinfo.loader_start = mmc->loader_start;
    mms->bootinfo.write_secondary_boot = j6m_write_secondary_boot;
    mms->bootinfo.secondary_cpu_reset_hook = j6m_secondary_cpu_reset;
    arm_load_kernel(ARM_CPU(mms->cpu[0]), machine, &mms->bootinfo);
}

static void j6m_set_default_ram_info(HorizonJ6MMachineClass *mmc)
{
    MachineClass *mc = MACHINE_CLASS(mmc);
    const RAMInfo *p;

    for (p = mmc->raminfo; p->name; p++) {
        if (p->mrindex < 0) {
            /* Found the entry for "system memory" */
            mc->default_ram_size = p->size;
            mc->default_ram_id = p->name;
            mmc->loader_start = p->base;
            return;
        }
    }
    g_assert_not_reached();
}

static void horizon_j6m_class_init(ObjectClass *oc, const void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);

    mc->init = horizon_j6m_init;
    mc->max_cpus = HORIZON_J6M_CPU_MAX;
}

static void horizon_j6m_machine_class_init(ObjectClass *oc, const void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);
    HorizonJ6MMachineClass *mmc = HORIZON_J6M_MACHINE_CLASS(oc);
    static const char * const valid_cpu_types[] = {
        ARM_CPU_TYPE_NAME("cortex-r52"),
        NULL
    };

    mc->desc = "Horizon Robotics J6M Board (dual Cortex-R52)";
    mc->default_cpus = 2;
    mc->min_cpus = 1;
    mc->max_cpus = 2;
    mc->default_cpu_type = ARM_CPU_TYPE_NAME("cortex-r52");
    mc->valid_cpu_types = valid_cpu_types;
    mmc->raminfo = j6m_raminfo;
    j6m_set_default_ram_info(mmc);
}

static const TypeInfo horizon_j6m_machine_types[] = {
    {
        .name = TYPE_HORIZON_J6M_MACHINE,
        .parent = TYPE_MACHINE,
        .instance_size = sizeof(HorizonJ6MMachineState),
        .class_size = sizeof(HorizonJ6MMachineClass),
        .class_init = horizon_j6m_class_init,
    }, {
        .name = MACHINE_TYPE_NAME("horizon-j6m"),
        .parent = TYPE_HORIZON_J6M_MACHINE,
        .class_init = horizon_j6m_machine_class_init,
    },
};

DEFINE_TYPES(horizon_j6m_machine_types);
