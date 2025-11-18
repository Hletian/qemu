# QEMU Development Guidelines for AI Agents

## Project Overview
QEMU is a generic machine emulator and virtualizer supporting full system emulation (softmmu), user-mode emulation (linux-user/bsd-user), and hypervisor acceleration (KVM/Xen). This branch (`adpt_j6`) appears focused on ARM/i.MX6 SabreLite board adaptations.

## Build System Architecture

### Two-Stage Build Process
1. **Configure**: Shell script that detects host, sets up Python venv, finds cross-compilers, and invokes Meson
   - Run: `../configure` from a build directory (out-of-tree builds required)
   - Key files: `configure`, `config-host.mak`, `pythondeps.toml`
2. **Meson/Ninja**: Actual compilation orchestrated by `meson.build` and `build.ninja`
   - Build: `make` (wraps ninja)
   - Test: `make check-qtest` for device tests

### Critical Commands
```bash
mkdir build && cd build
../configure --target-list=arm-softmmu  # Specific target
make -j$(nproc)
make check-qtest  # Run QTest framework tests
```

## Coding Style (Enforced by `scripts/checkpatch.pl`)
- **Indentation**: 4 spaces, NO tabs (except Makefiles)
- **Line width**: 80 characters (warn at 100)
- **Naming**:
  - Variables: `lower_case_with_underscores`
  - Types: `CamelCase` (structs, enums, classes)
  - Scalar types: `lower_case_with_underscores_ending_with_t`
  - Functions: Subsystem prefix (e.g., `memory_region_*`, `qdev_*`)
- **Common abbreviations**: `CPUState *cs`, `CPUArchState *env`, `DeviceState *dev`

Always run `scripts/checkpatch.pl` on patches before submission.

## Core Architecture Patterns

### QEMU Object Model (QOM)
All devices inherit from TYPE_DEVICE via QOM. Standard pattern:
```c
#define TYPE_MY_DEVICE "my-device"
typedef struct MyDevice {
    DeviceState parent_obj;
    MemoryRegion io;
    int reg0, reg1;
} MyDevice;

static const TypeInfo my_device_info = {
    .name = TYPE_MY_DEVICE,
    .parent = TYPE_DEVICE,
    .instance_size = sizeof(MyDevice),
};

type_init(my_device_register_types)
```

### Memory Regions (System-Wide MMIO/RAM Model)
Memory is hierarchical trees of MemoryRegion objects. Key types:
- **RAM**: `memory_region_init_ram()` - guest memory backed by host RAM
- **MMIO**: `memory_region_init_io()` - device registers with callbacks
- **Container**: Groups subregions at offsets
- **Alias**: Window into another region (e.g., PCI BAR mappings)

Common device pattern:
```c
memory_region_init_io(&s->iomem, OBJECT(s), &my_ops, s, "my-device", 0x1000);
sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->iomem);
// Later mapped: memory_region_add_subregion(get_system_memory(), 0x10000000, &s->iomem);
```

### QAPI (QMP/Monitor Interface Code Generation)
- Schema files: `qapi/*.json` define commands, events, types
- Generated code: `qapi/qapi-types-*.h`, `qapi/qapi-visit-*.h`
- Run `scripts/qapi-gen.py` to regenerate (happens automatically during build)
- Monitor commands must use QAPI, not direct implementation

## Testing Framework (QTest)

### libqtest & libqos
Device tests use QTest protocol to control QEMU from external process:
```c
QTestState *qts = qtest_init("-M sabrelite");
qtest_writel(qts, 0x02190000, 0x1234);  // Write to device
uint32_t val = qtest_readl(qts, 0x02190000);
qtest_quit(qts);
```

### Qgraph (Dependency Graph for Tests)
Auto-generates test combinations for machine/device/interface:
```c
static void my_test_register(void) {
    qos_node_create_machine("arm/sabrelite", qos_create_machine_arm_sabrelite);
    qos_node_contains("arm/sabrelite", "generic-sdhci", NULL);
}
libqos_init(my_test_register);
```

Tests go in `tests/qtest/`, add to `tests/qtest/meson.build`.

## Hardware Emulation Specifics

### ARM SoC Pattern (e.g., `hw/arm/fsl-imx6.c`)
- SoC object contains CPU + peripherals
- Board file (`hw/arm/sabrelite.c`) instantiates SoC, adds RAM, boots
- Memory map defined in `include/hw/arm/fsl-imx6.h`:
  ```c
  #define FSL_IMX6_MMDC_ADDR    0x10000000
  #define FSL_IMX6_MMDC_SIZE    0x20000000  // 512MB
  ```

### Device Lifecycle
1. `instance_init`: Allocate/init child objects
2. `realize`: Wire up IRQs, map memory regions, connect to bus
3. `reset`: Set registers to power-on state
4. `unrealize`: Cleanup

## Key Directories
- `hw/*/`: Device emulation (organized by bus/architecture)
- `target/*/`: CPU emulation (TCG translators, helpers)
- `system/`: Core emulation (memory.c, cpus.c, qtest.c)
- `include/hw/`: Device headers
- `tests/qtest/`: Device functional tests
- `docs/devel/`: Developer documentation (read `memory.rst`, `qom.rst`)

## Migration & State
Use VMState descriptors for device state migration:
```c
static const VMStateDescription vmstate_my_device = {
    .name = "my-device",
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(reg0, MyDevice),
        VMSTATE_END_OF_LIST()
    }
};
```

## Debugging Tips
- QEMU monitor: `-monitor stdio` for interactive debugging
- Tracing: Enable with `scripts/tracetool.py`, events in `trace-events`
- GDB stub: `-s -S` (wait on port 1234)
- QTest logs: `QTEST_LOG=1` for protocol dumps

## Common Mistakes to Avoid
- Don't mix tabs and spaces (use `scripts/checkpatch.pl`)
- Don't call `memory_region_init_*` in instance_init, use realize
- Don't forget to register migration state with VMState
- For cross-architecture code, use `qemu/bswap.h` (ldl_le_p, stw_be_p)
- Always check `errp` propagation in device realize chains

## Upstream Contribution
- Patches to `qemu-devel@nongnu.org` with `Signed-off-by:` line
- Use `git format-patch` or `git-publish` tool
- Follow `docs/devel/submitting-a-patch.rst`
- Tag with subsystem prefix: `[PATCH] arm/sabrelite: fix memory size`
