/**
 * @file nsx_power.h
 * @author Carlos Morales
 * @brief neuralSPOT Power Management Library
 * @version 0.1
 * @date 2022-09-02
 *
 * @copyright Copyright (c) 2022
 *
 * \addtogroup ns-power
 *  @{
 */

//*****************************************************************************
//
// Copyright (c) 2021, Ambiq Micro, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
//
// Third party software included in this distribution is subject to the
// additional license terms as defined in the /docs/licenses directory.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// This is part of revision release_sdk_4_0_1-bef824fa27 of the AmbiqSuite Development Package.
//
//*****************************************************************************
#ifndef NS_POWER
    #define NS_POWER

    #ifdef __cplusplus
extern "C" {
    #endif
    #include "ns_core.h"
    #include "am_bsp.h"
    #include "am_mcu_apollo.h"
    #include "am_util.h"

    #define NS_POWER_V0_0_1                                                                        \
        { .major = 0, .minor = 0, .revision = 1 }
    #define NS_POWER_V1_0_0                                                                        \
        { .major = 1, .minor = 0, .revision = 0 }

    #define NS_POWER_OLDEST_SUPPORTED_VERSION NS_POWER_V0_0_1
    #define NS_POWER_CURRENT_VERSION NS_POWER_V1_0_0
    #define NS_POWER_API_ID 0xCA0007

extern const ns_core_api_t ns_power_V0_0_1;
extern const ns_core_api_t ns_power_V1_0_0;
extern const ns_core_api_t ns_power_oldest_supported_version;
extern const ns_core_api_t ns_power_current_version;

/// CPU performance / clock-speed mode.
typedef enum {
    NS_PERF_NOT_SET = -1,
    NS_PERF_LOW     = 0, ///< LP 96 MHz (HFRC)
    NS_PERF_HIGH    = 1, ///< HP 250 MHz (HFRC2, Apollo5) / 192 MHz (Apollo4)
    NS_PERF_MAX     = 2  ///< HP2 (Apollo510L only, otherwise same as HIGH)
} ns_perf_mode_e;

/// Power configuration — controls which subsystems stay powered.
typedef struct {
    const ns_core_api_t *api;       ///< API version
    ns_perf_mode_e perf_mode;       ///< CPU clock mode
    bool need_audadc;               ///< Keep audio ADC powered
    bool need_ssram;                ///< Keep shared SRAM powered
    bool need_crypto;               ///< Keep crypto block powered
    bool need_ble;                  ///< Keep BLE powered
    bool need_usb;                  ///< Keep USB powered
    bool need_iom;                  ///< Keep IOM interfaces powered
    bool need_uart;                 ///< Keep alternative UART powered
    bool small_tcm;                 ///< Use 32K ITCM + 128K DTCM (vs full)
    bool need_tempco;               ///< Enable temperature compensation
    bool need_itm;                  ///< Keep ITM/SWO debug output
    bool need_xtal;                 ///< Keep XTAL oscillator
    bool spotmgr_collapse;          ///< SpotManager STM state collapse (Apollo5 family)
} ns_power_config_t;

extern const ns_power_config_t ns_power_all_on;   ///< Everything enabled — development/debug
extern const ns_power_config_t ns_power_minimal;   ///< Minimal peripherals — good starting point
extern const ns_power_config_t ns_power_audio;     ///< Audio ADC on, most else off

/**
 * @brief Set SOC Power Mode
 *
 * @param ns_power_config_t Desired power mode
 * @return uint32_t success/failure
 */
extern uint32_t ns_power_config(const ns_power_config_t *);

/**
 * @brief neuralSPOT-aware deep_sleep - turns off certain systems off before sleeping
 * and turns them back upon waking.
 *
 */
extern void ns_deep_sleep(void);

/**
 * @brief Sets CPU frequency to one of the ns_power_modes
 *
 * @param mode  Desired performance mode
 * @return uint32_t status
 */
uint32_t ns_set_performance_mode(ns_perf_mode_e mode);

/* ===================================================================
 * Power-down helpers
 *
 * Building blocks for low-power operation.  Each does one thing with
 * the correct register sequence.
 *
 * General-purpose (safe to call from any app):
 *   ns_power_shutdown_peripherals() — disable all peripheral domains
 *   ns_power_minimize_memory()      — smallest TCM, no SSRAM
 *   ns_power_tristate_gpios()       — float unused pins
 *   ns_power_disable_debug()        — disable SWO/ITM + debug domain
 *   ns_power_disable_caches()       — invalidate + disable I/D cache
 *
 * NVM shutdown (irreversible — power measurement only):
 *   ns_power_disable_nvm()          — request MRAM power-off
 *   NS_POWER_DRAIN_NVM()            — finalize NVM shutdown
 *
 * Recommended full sequence for minimum active current:
 *   1. ns_power_disable_debug()
 *   2. ns_power_shutdown_peripherals()
 *   3. ns_power_minimize_memory()
 *   4. ns_power_tristate_gpios(keep_pins, n)
 *   5. ns_set_performance_mode()        — select clock LAST
 *   6. (from ITCM) ns_power_disable_nvm() + NS_POWER_DRAIN_NVM()
 *   7. (from ITCM) ns_power_disable_caches() — if no MRAM calls
 * =================================================================== */

/**
 * @brief Shut down all non-core peripherals and timers.
 *
 * Disables all device power domains (USB, audio, crypto, IOM, etc.),
 * XTAL oscillator, voltage comparator, and all 16 hardware timers.
 * Does NOT touch memory config, GPIO, or debug — call the dedicated
 * helpers for those.
 *
 * Safe to call from any app that no longer needs peripheral I/O.
 *
 * @return 0 on success.
 */
uint32_t ns_power_shutdown_peripherals(void);

/**
 * @brief Configure minimal memory for lowest power.
 *
 * Sets 32 KB ITCM + 128 KB DTCM, single NVM bank, no shared SRAM,
 * and enables MRAM low-power read mode.  Suitable for small
 * ITCM-resident workloads with data in DTCM only.
 *
 * Any code or data outside these regions will be inaccessible
 * after this call.  Ensure your stack, heap, and working buffers
 * fit within 128 KB DTCM.
 *
 * @return 0 on success.
 */
uint32_t ns_power_minimize_memory(void);

/**
 * @brief Request NVM (MRAM) power-off.
 *
 * Clears PWRENNVM for both NVM banks and issues barrier.
 * The NVM controller will not finalize power-down until an MRAM bus
 * transaction completes — call NS_POWER_DRAIN_NVM() afterward.
 *
 * @warning After NS_POWER_DRAIN_NVM(), MRAM is inaccessible.
 *          The caller must already be executing from ITCM/TCM.
 */
void ns_power_disable_nvm(void);

/**
 * @brief Force one MRAM read to drain the NVM pipeline.
 *
 * After ns_power_disable_nvm(), the NVM controller waits for
 * outstanding MRAM fetches before powering down.  If the CPU never
 * touches MRAM (e.g. all code inlined into ITCM), NVM stays on.
 * This macro forces one volatile read to complete the shutdown.
 *
 * Must be called from ITCM-resident code.  After this macro,
 * MRAM is powered off — no further MRAM access is possible.
 */
    #define NS_POWER_DRAIN_NVM() do { \
        volatile uint32_t _nvm_drain = *(volatile uint32_t *)0x00430000; \
        (void)_nvm_drain; \
    } while (0)

/**
 * @brief Invalidate and disable I-cache and D-cache.
 *
 * For ITCM-only execution, caches consume power with no benefit
 * since ITCM/DTCM are zero-wait-state.
 *
 * @note If any code still calls functions in MRAM (e.g. libc
 *       memset), keep the I-cache enabled — those calls are
 *       served from the warm cache even after NVM shutdown.
 */
void ns_power_disable_caches(void);

/**
 * @brief Disable SWO/ITM debug output and debug peripheral.
 *
 * Clears MCUCTRL DBGCTRL and powers down the debug domain.
 * Call before entering measurement — debug logic draws current.
 */
void ns_power_disable_debug(void);

/**
 * @brief Tristate all GPIO pads except specified pins.
 *
 * Unconfigured GPIOs can leak current through internal pull
 * resistors.  This sets every pad to disabled (high-impedance)
 * except those listed in @p keep_pins.
 *
 * Typical keep list: power-monitor GPIOs, wake-up pins, or
 * any pin that must remain driven during measurement or sleep.
 *
 * @param keep_pins  Array of GPIO numbers to leave configured.
 * @param n_keep     Number of entries in keep_pins.
 */
void ns_power_tristate_gpios(const uint32_t *keep_pins, uint32_t n_keep);

    #ifdef __cplusplus
}
    #endif
#endif // NS_POWER
/** @}*/
