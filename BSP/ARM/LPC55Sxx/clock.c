/*
 * Copyright 2017-2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/*
 This file is ported from NXP example.
 */

#include "bbase.h"
#include "bsp.h"

#if SMX_CFG_SSMX
#pragma section_prefix = ".sys"
#endif

#ifdef __cplusplus
extern "C" {
#endif

void ClockConfig(void);
void pinConfig(void);

#ifdef __cplusplus
}
#endif

/*----------------------------------------------------------------------------
 *
 *  Description: Initialize Clocks
 *
 ----------------------------------------------------------------------------*/
void ClockConfig(void)
{
   /*!< Set up the clock sources */
    /*!< Configure FRO192M */
    POWER_DisablePD(kPDRUNCFG_PD_FRO192M); /*!< Ensure FRO is on  */
    CLOCK_SetupFROClocking(12000000U);     /*!< Set up FRO to the 12 MHz, just for sure */
    CLOCK_AttachClk(kFRO12M_to_MAIN_CLK);  /*!< Switch to FRO 12MHz first to ensure we can change the clock setting */

    /*!< Configure XTAL32M */
    POWER_DisablePD(kPDRUNCFG_PD_XTAL32M);                                /* Ensure XTAL32M is powered */
    POWER_DisablePD(kPDRUNCFG_PD_LDOXO32M);                               /* Ensure XTAL32M is powered */
    CLOCK_SetupExtClocking(16000000U);                                    /* Enable clk_in clock */
    SYSCON->CLOCK_CTRL |= SYSCON_CLOCK_CTRL_CLKIN_ENA_MASK;               /* Enable clk_in from XTAL32M clock  */
    ANACTRL->XO32M_CTRL |= ANACTRL_XO32M_CTRL_ENABLE_SYSTEM_CLK_OUT_MASK; /* Enable clk_in to system  */

    //POWER_SetVoltageForFreq(
    //    150000000U); /*!< Set voltage for the one of the fastest clock outputs: System clock output */
    CLOCK_SetFLASHAccessCyclesForFreq(150000000U); /*!< Set FLASH wait states for core */

    /*!< Set up PLL */
    CLOCK_AttachClk(kEXT_CLK_to_PLL0);  /*!< Switch PLL0CLKSEL to EXT_CLK */
    POWER_DisablePD(kPDRUNCFG_PD_PLL0); /* Ensure PLL is on  */
    POWER_DisablePD(kPDRUNCFG_PD_PLL0_SSCG);
    const pll_setup_t pll0Setup = {
        .pllctrl = SYSCON_PLL0CTRL_CLKEN_MASK | SYSCON_PLL0CTRL_SELI(53U) | SYSCON_PLL0CTRL_SELP(31U),
        .pllndec = SYSCON_PLL0NDEC_NDIV(PLL0_N),
        .pllpdec = SYSCON_PLL0PDEC_PDIV(PLL0_P),
        .pllsscg = {0x0U, (SYSCON_PLL0SSCG1_MDIV_EXT(PLL0_M) | SYSCON_PLL0SSCG1_SEL_EXT_MASK)},
        .pllRate = PLL0_CLK,
        .flags   = PLL_SETUPFLAG_WAITLOCK};
    CLOCK_SetPLL0Freq(&pll0Setup); /*!< Configure PLL0 to the desired values */

    /*!< Set up dividers */
    CLOCK_SetClkDiv(kCLOCK_DivAhbClk, 1U, false); /*!< Set AHBCLKDIV divider to value 1 */

    /*!< Set up clock selectors - Attach clocks to the peripheries */
    CLOCK_AttachClk(kPLL0_to_MAIN_CLK); /*!< Switch MAIN_CLK to PLL0 */
}


