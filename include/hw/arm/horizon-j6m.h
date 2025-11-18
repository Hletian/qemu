/*
 * Horizon Robotics J6M SoC
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
 */

#ifndef HW_ARM_HORIZON_J6M_H
#define HW_ARM_HORIZON_J6M_H

#include "hw/boards.h"
#include "hw/arm/boot.h"
#include "qom/object.h"

#define TYPE_HORIZON_J6M_MACHINE MACHINE_TYPE_NAME("horizon-j6m")
#define TYPE_HORIZON_J6M "horizon-j6m"

typedef struct HorizonJ6MClass HorizonJ6MClass;
typedef struct HorizonJ6MState HorizonJ6MState;
DECLARE_OBJ_CHECKERS(HorizonJ6MState, HorizonJ6MClass,
                     HORIZON_J6M, TYPE_HORIZON_J6M)

/* Maximum number of CPUs (dual Cortex-R52) */
#define HORIZON_J6M_CPU_MAX 2

/* Maximum number of RAM regions */
#define HORIZON_J6M_RAM_MAX 8

/* Number of UARTs */
#define HORIZON_J6M_UART_MAX 4

/* GIC base address and parameters */
#define HORIZON_J6M_PERIPHBASE 0xF0000000
#define HORIZON_J6M_NUM_SPIS 128

/* Memory map (placeholder - adjust based on J6M datasheet) */
#define HORIZON_J6M_TCM_BASE    0x00000000
#define HORIZON_J6M_TCM_SIZE    0x00020000  /* 128KB per core */
#define HORIZON_J6M_SRAM_BASE   0x10000000
#define HORIZON_J6M_SRAM_SIZE   0x00100000  /* 1MB */
#define HORIZON_J6M_DDR_BASE    0x40000000
#define HORIZON_J6M_DDR_SIZE    0x80000000  /* 2GB */
#define HORIZON_J6M_PERIPH_BASE 0xA0000000

/* Clock frequency (placeholder - adjust based on J6M specs) */
#define HORIZON_J6M_CLK_FRQ 800000000  /* 800 MHz */

typedef struct {
    const char *name;
    hwaddr base;
    hwaddr size;
    int mrindex;  /* -1 for system RAM */
    int flags;
} RAMInfo;

/* Flags for RAMInfo */
#define IS_MAIN 1
#define IS_ROM  2

struct HorizonJ6MClass {
    MachineClass parent;
    const RAMInfo *raminfo;
    hwaddr loader_start;
};

struct HorizonJ6MState {
    /*< private >*/
    MachineState parent;
    
    /*< public >*/
    /* Boot info */
    ARMBootInfo bootinfo;
    
    /* Memory regions */
    MemoryRegion ram[HORIZON_J6M_RAM_MAX];
    MemoryRegion cpu_sysmem[HORIZON_J6M_CPU_MAX];
    MemoryRegion sysmem_alias[HORIZON_J6M_CPU_MAX];
    MemoryRegion cpu_tcm[HORIZON_J6M_CPU_MAX];
    
    /* CPUs */
    Object *cpu[HORIZON_J6M_CPU_MAX];
    
    /* Peripherals (defined opaquely to avoid header dependencies) */
    void *gic;
    void *uart;
    void *watchdog;
    void *dualtimer;
    
    /* Clocks */
    void *sysclk;
};

#endif /* HW_ARM_HORIZON_J6M_H */
