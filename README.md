# nsx-power

`nsx-power` provides NSX power-management helpers for Ambiq SoCs.

The module keeps power configuration as a first-class capability:
- MCU performance mode selection
- shutdown of unused peripheral blocks
- SRAM and memory retention configuration
- deep-sleep helpers
- Apollo4 temperature compensation support where available

Target-specific behavior is selected from `NSX_SOC_FAMILY` in CMake and from
the board/SoC compile definitions exported by `NSX_BOARD_FLAGS_TARGET`.

The public API is exported as `nsx_power.h`.
