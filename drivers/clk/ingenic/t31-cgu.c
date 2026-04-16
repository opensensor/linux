// SPDX-License-Identifier: GPL-2.0
/*
 * Ingenic T31 SoC CGU driver
 *
 * Copyright (C) 2024 Thingino Project
 *
 * Based on x1830-cgu.c and vendor clk-t31.c
 */

#include <linux/clk-provider.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of.h>

#include <dt-bindings/clock/ingenic,t31-cgu.h>

#include "cgu.h"
#include "pm.h"

/* CGU register offsets */
#define CGU_REG_CPCCR		0x00
#define CGU_REG_CPPCR		0x0c
#define CGU_REG_APLL		0x10
#define CGU_REG_MPLL		0x14
#define CGU_REG_CLKGR0		0x20
#define CGU_REG_OPCR		0x24
#define CGU_REG_CLKGR1		0x28
#define CGU_REG_DDRCDR		0x2c
#define CGU_REG_EL150CDR	0x30
#define CGU_REG_RSACDR		0x4c
#define CGU_REG_MACCDR		0x54
#define CGU_REG_LPCDR		0x64
#define CGU_REG_MSC0CDR		0x68
#define CGU_REG_I2STCDR		0x70
#define CGU_REG_SSICDR		0x74
#define CGU_REG_CIMCDR		0x7c
#define CGU_REG_ISPCDR		0x80
#define CGU_REG_I2SRCDR		0x84
#define CGU_REG_MSC1CDR		0xa4
#define CGU_REG_CPCSR		0xd4
#define CGU_REG_VPLL		0xe0

/* bits within the OPCR register */
#define OPCR_GATE_USBPHYCLK	BIT(23)
#define OPCR_SPENDN0		BIT(7)

/* bits within the USBPCR register (T31 doesn't have separate USBPCR) */

static struct ingenic_cgu *cgu;

static int t31_usb_phy_enable(struct clk_hw *hw)
{
	void __iomem *reg_opcr = cgu->base + CGU_REG_OPCR;

	writel(readl(reg_opcr) & ~OPCR_GATE_USBPHYCLK, reg_opcr);
	return 0;
}

static void t31_usb_phy_disable(struct clk_hw *hw)
{
	void __iomem *reg_opcr = cgu->base + CGU_REG_OPCR;

	writel(readl(reg_opcr) | OPCR_GATE_USBPHYCLK, reg_opcr);
}

static int t31_usb_phy_is_enabled(struct clk_hw *hw)
{
	void __iomem *reg_opcr = cgu->base + CGU_REG_OPCR;

	return !(readl(reg_opcr) & OPCR_GATE_USBPHYCLK);
}

static const struct clk_ops t31_otg_phy_ops = {
	.enable		= t31_usb_phy_enable,
	.disable	= t31_usb_phy_disable,
	.is_enabled	= t31_usb_phy_is_enabled,
};

/*
 * T31 PLL OD encoding: the OD field is 3 bits wide.
 * From vendor PLL_DESC: od_shift=11, od_bits=3
 * The vendor rate table uses OD values 2 and 3 (encoded directly).
 * T31 PLLs use: rate = (ext * M) / (N * OD)
 * where M = reg[31:20]+1, N = reg[19:14]+1, OD = reg[13:11] (encoded)
 *
 * OD encoding (from vendor rate table analysis):
 *   OD=1 -> encoded 0, OD=2 -> encoded 1, OD=4 -> encoded 2,
 *   OD=8 -> encoded 3
 * This matches powers of 2: od_encoding[actual_od] = log2(actual_od)
 */
static const s8 pll_od_encoding[9] = {
	 -1, 0x0, 0x1,  -1, 0x2,  -1,  -1,  -1, 0x3,
};

static const struct ingenic_cgu_clk_info t31_cgu_clocks[] = {

	/* External clocks */

	[T31_CLK_EXCLK] = { "ext", CGU_CLK_EXT },
	[T31_CLK_RTCLK] = { "rtc", CGU_CLK_EXT },

	/* PLLs */

	[T31_CLK_APLL] = {
		"apll", CGU_CLK_PLL,
		.parents = { T31_CLK_EXCLK, -1, -1, -1 },
		.pll = {
			.reg = CGU_REG_APLL,
			.rate_multiplier = 1,
			.m_shift = 20,
			.m_bits = 12,
			.m_offset = 1,
			.n_shift = 14,
			.n_bits = 6,
			.n_offset = 1,
			.od_shift = 11,
			.od_bits = 3,
			.od_max = 8,
			.od_encoding = pll_od_encoding,
			.bypass_reg = CGU_REG_CPPCR,
			.bypass_bit = 30,
			.enable_bit = 0,
			.stable_bit = 3,
		},
	},

	[T31_CLK_MPLL] = {
		"mpll", CGU_CLK_PLL,
		.parents = { T31_CLK_EXCLK, -1, -1, -1 },
		.pll = {
			.reg = CGU_REG_MPLL,
			.rate_multiplier = 1,
			.m_shift = 20,
			.m_bits = 12,
			.m_offset = 1,
			.n_shift = 14,
			.n_bits = 6,
			.n_offset = 1,
			.od_shift = 11,
			.od_bits = 3,
			.od_max = 8,
			.od_encoding = pll_od_encoding,
			.bypass_reg = CGU_REG_CPPCR,
			.bypass_bit = 28,
			.enable_bit = 0,
			.stable_bit = 3,
		},
	},

	[T31_CLK_VPLL] = {
		"vpll", CGU_CLK_PLL,
		.parents = { T31_CLK_EXCLK, -1, -1, -1 },
		.pll = {
			.reg = CGU_REG_VPLL,
			.rate_multiplier = 1,
			.m_shift = 20,
			.m_bits = 12,
			.m_offset = 1,
			.n_shift = 14,
			.n_bits = 6,
			.n_offset = 1,
			.od_shift = 11,
			.od_bits = 3,
			.od_max = 8,
			.od_encoding = pll_od_encoding,
			.bypass_reg = CGU_REG_CPPCR,
			.bypass_bit = 26,
			.enable_bit = 0,
			.stable_bit = 3,
		},
	},

	/* Custom (SoC-specific) OTG PHY */

	[T31_CLK_OTGPHY] = {
		"otg_phy", CGU_CLK_CUSTOM,
		.parents = { T31_CLK_EXCLK, -1, -1, -1 },
		.custom = { &t31_otg_phy_ops },
	},

	/* Muxes & dividers */

	[T31_CLK_SCLKA] = {
		"sclk_a", CGU_CLK_MUX,
		.parents = { -1, T31_CLK_EXCLK, T31_CLK_APLL, -1 },
		.mux = { CGU_REG_CPCCR, 30, 2 },
	},

	[T31_CLK_CPUMUX] = {
		"cpu_mux", CGU_CLK_MUX,
		.parents = { -1, T31_CLK_SCLKA, T31_CLK_MPLL, -1 },
		.mux = { CGU_REG_CPCCR, 28, 2 },
	},

	[T31_CLK_CPU] = {
		"cpu", CGU_CLK_DIV | CGU_CLK_GATE,
		.flags = CLK_IS_CRITICAL,
		.parents = { T31_CLK_CPUMUX, -1, -1, -1 },
		.div = { CGU_REG_CPCCR, 0, 1, 4, 22, -1, -1 },
		.gate = { CGU_REG_CLKGR1, 15 },
	},

	[T31_CLK_L2CACHE] = {
		"l2cache", CGU_CLK_DIV,
		.flags = CLK_IS_CRITICAL,
		.parents = { T31_CLK_CPUMUX, -1, -1, -1 },
		.div = { CGU_REG_CPCCR, 4, 1, 4, 22, -1, -1 },
	},

	[T31_CLK_AHB0] = {
		"ahb0", CGU_CLK_MUX | CGU_CLK_DIV,
		.parents = { -1, T31_CLK_SCLKA, T31_CLK_MPLL, -1 },
		.mux = { CGU_REG_CPCCR, 26, 2 },
		.div = { CGU_REG_CPCCR, 8, 1, 4, 21, -1, -1 },
	},

	[T31_CLK_AHB2MUX] = {
		"ahb2_apb_mux", CGU_CLK_MUX,
		.parents = { -1, T31_CLK_SCLKA, T31_CLK_MPLL, -1 },
		.mux = { CGU_REG_CPCCR, 24, 2 },
	},

	[T31_CLK_AHB2] = {
		"ahb2", CGU_CLK_DIV,
		.parents = { T31_CLK_AHB2MUX, -1, -1, -1 },
		.div = { CGU_REG_CPCCR, 12, 1, 4, 20, -1, -1 },
	},

	[T31_CLK_PCLK] = {
		"pclk", CGU_CLK_DIV | CGU_CLK_GATE,
		.parents = { T31_CLK_AHB2MUX, -1, -1, -1 },
		.div = { CGU_REG_CPCCR, 16, 1, 4, 20, -1, -1 },
		.gate = { CGU_REG_CLKGR1, 14 },
	},

	[T31_CLK_DDR] = {
		"ddr", CGU_CLK_MUX | CGU_CLK_DIV | CGU_CLK_GATE,
		.flags = CLK_IS_CRITICAL,
		.parents = { -1, T31_CLK_SCLKA, T31_CLK_MPLL, -1 },
		.mux = { CGU_REG_DDRCDR, 30, 2 },
		.div = { CGU_REG_DDRCDR, 0, 1, 4, 29, 28, 27 },
		.gate = { CGU_REG_CLKGR0, 31 },
	},

	[T31_CLK_MAC] = {
		"mac", CGU_CLK_MUX | CGU_CLK_DIV | CGU_CLK_GATE,
		.parents = { T31_CLK_SCLKA, T31_CLK_MPLL,
			     T31_CLK_VPLL, -1 },
		.mux = { CGU_REG_MACCDR, 30, 2 },
		.div = { CGU_REG_MACCDR, 0, 1, 8, 29, 28, 27 },
		.gate = { CGU_REG_CLKGR1, 4 },
	},

	[T31_CLK_LCD] = {
		"lcd", CGU_CLK_MUX | CGU_CLK_DIV | CGU_CLK_GATE,
		.parents = { T31_CLK_SCLKA, T31_CLK_MPLL,
			     T31_CLK_VPLL, -1 },
		.mux = { CGU_REG_LPCDR, 30, 2 },
		.div = { CGU_REG_LPCDR, 0, 1, 8, 29, 28, 27 },
		.gate = { CGU_REG_CLKGR0, 24 },
	},

	[T31_CLK_MSC0] = {
		"msc0", CGU_CLK_MUX | CGU_CLK_DIV | CGU_CLK_GATE,
		.parents = { T31_CLK_SCLKA, T31_CLK_MPLL,
			     T31_CLK_VPLL, T31_CLK_EXCLK },
		.mux = { CGU_REG_MSC0CDR, 30, 2 },
		.div = { CGU_REG_MSC0CDR, 0, 4, 8, 29, 28, 27 },
		.gate = { CGU_REG_CLKGR0, 4 },
	},

	[T31_CLK_MSC1] = {
		"msc1", CGU_CLK_MUX | CGU_CLK_DIV | CGU_CLK_GATE,
		.parents = { T31_CLK_SCLKA, T31_CLK_MPLL,
			     T31_CLK_VPLL, T31_CLK_EXCLK },
		.mux = { CGU_REG_MSC1CDR, 30, 2 },
		.div = { CGU_REG_MSC1CDR, 0, 4, 8, 29, 28, 27 },
		.gate = { CGU_REG_CLKGR0, 5 },
	},

	[T31_CLK_SSI] = {
		"ssi", CGU_CLK_MUX | CGU_CLK_DIV,
		.parents = { T31_CLK_SCLKA, T31_CLK_MPLL,
			     T31_CLK_VPLL, -1 },
		.mux = { CGU_REG_SSICDR, 30, 2 },
		.div = { CGU_REG_SSICDR, 0, 1, 8, 29, 28, 27 },
	},

	[T31_CLK_SFC] = {
		"sfc", CGU_CLK_DIV | CGU_CLK_GATE,
		.parents = { T31_CLK_SSI, -1, -1, -1 },
		.div = { CGU_REG_SSICDR, 0, 1, 8, 29, 28, 27 },
		.gate = { CGU_REG_CLKGR0, 20 },
	},

	[T31_CLK_CIM] = {
		"cim", CGU_CLK_MUX | CGU_CLK_DIV,
		.parents = { T31_CLK_SCLKA, T31_CLK_MPLL,
			     T31_CLK_VPLL, -1 },
		.mux = { CGU_REG_CIMCDR, 30, 2 },
		.div = { CGU_REG_CIMCDR, 0, 1, 8, 29, 28, 27 },
	},

	[T31_CLK_ISP] = {
		"isp", CGU_CLK_MUX | CGU_CLK_DIV | CGU_CLK_GATE,
		.parents = { T31_CLK_SCLKA, T31_CLK_MPLL,
			     T31_CLK_VPLL, -1 },
		.mux = { CGU_REG_ISPCDR, 30, 2 },
		.div = { CGU_REG_ISPCDR, 0, 1, 4, 29, 28, 27 },
		.gate = { CGU_REG_CLKGR0, 23 },
	},

	[T31_CLK_RSA] = {
		"rsa", CGU_CLK_MUX | CGU_CLK_DIV | CGU_CLK_GATE,
		.parents = { T31_CLK_SCLKA, T31_CLK_MPLL,
			     T31_CLK_VPLL, -1 },
		.mux = { CGU_REG_RSACDR, 30, 2 },
		.div = { CGU_REG_RSACDR, 0, 1, 4, 29, 28, 27 },
		.gate = { CGU_REG_CLKGR0, 27 },
	},

	[T31_CLK_I2ST] = {
		"i2st", CGU_CLK_MUX | CGU_CLK_DIV,
		.parents = { T31_CLK_SCLKA, T31_CLK_MPLL,
			     T31_CLK_VPLL, -1 },
		.mux = { CGU_REG_I2STCDR, 30, 2 },
		.div = { CGU_REG_I2STCDR, 0, 1, 20, 29, 28, -1 },
	},

	[T31_CLK_I2SR] = {
		"i2sr", CGU_CLK_MUX | CGU_CLK_DIV,
		.parents = { T31_CLK_SCLKA, T31_CLK_MPLL,
			     T31_CLK_VPLL, -1 },
		.mux = { CGU_REG_I2SRCDR, 30, 2 },
		.div = { CGU_REG_I2SRCDR, 0, 1, 20, 29, 28, -1 },
	},

	[T31_CLK_EL150] = {
		"el150", CGU_CLK_MUX | CGU_CLK_DIV | CGU_CLK_GATE,
		.parents = { T31_CLK_SCLKA, T31_CLK_MPLL,
			     T31_CLK_VPLL, -1 },
		.mux = { CGU_REG_EL150CDR, 30, 2 },
		.div = { CGU_REG_EL150CDR, 0, 1, 4, 29, 28, 27 },
		.gate = { CGU_REG_CLKGR1, 0 },
	},

	/* Gate-only clocks */

	[T31_CLK_GATE_DDR] = {
		"gate_ddr", CGU_CLK_GATE,
		.flags = CLK_IS_CRITICAL,
		.parents = { T31_CLK_DDR, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 31 },
	},

	[T31_CLK_GATE_TCU] = {
		"tcu_gate", CGU_CLK_GATE,
		.flags = CLK_IS_CRITICAL,
		.parents = { T31_CLK_EXCLK, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 30 },
	},

	[T31_CLK_GATE_DES] = {
		"des", CGU_CLK_GATE,
		.parents = { T31_CLK_PCLK, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 28 },
	},

	[T31_CLK_GATE_RSA] = {
		"gate_rsa", CGU_CLK_GATE,
		.parents = { T31_CLK_RSA, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 27 },
	},

	[T31_CLK_GATE_CSI] = {
		"csi", CGU_CLK_GATE,
		.parents = { T31_CLK_AHB0, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 25 },
	},

	[T31_CLK_GATE_LCD] = {
		"gate_lcd", CGU_CLK_GATE,
		.parents = { T31_CLK_LCD, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 24 },
	},

	[T31_CLK_GATE_ISP] = {
		"gate_isp", CGU_CLK_GATE,
		.parents = { T31_CLK_ISP, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 23 },
	},

	[T31_CLK_GATE_PDMA] = {
		"pdma", CGU_CLK_GATE,
		.parents = { T31_CLK_AHB2, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 21 },
	},

	[T31_CLK_GATE_SFC] = {
		"gate_sfc", CGU_CLK_GATE,
		.parents = { T31_CLK_SFC, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 20 },
	},

	[T31_CLK_GATE_SSI1] = {
		"ssi1", CGU_CLK_GATE,
		.parents = { T31_CLK_SSI, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 19 },
	},

	[T31_CLK_GATE_HASH] = {
		"hash", CGU_CLK_GATE,
		.parents = { T31_CLK_AHB2, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 18 },
	},

	[T31_CLK_GATE_SLV] = {
		"slv", CGU_CLK_GATE,
		.parents = { T31_CLK_PCLK, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 17 },
	},

	[T31_CLK_GATE_UART2] = {
		"uart2", CGU_CLK_GATE,
		.parents = { T31_CLK_EXCLK, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 16 },
	},

	[T31_CLK_GATE_UART1] = {
		"uart1", CGU_CLK_GATE,
		.parents = { T31_CLK_EXCLK, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 15 },
	},

	[T31_CLK_GATE_UART0] = {
		"uart0", CGU_CLK_GATE,
		.parents = { T31_CLK_EXCLK, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 14 },
	},

	[T31_CLK_GATE_SADC] = {
		"sadc", CGU_CLK_GATE,
		.parents = { T31_CLK_PCLK, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 13 },
	},

	[T31_CLK_GATE_DMIC] = {
		"dmic", CGU_CLK_GATE,
		.parents = { T31_CLK_PCLK, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 12 },
	},

	[T31_CLK_GATE_AIC] = {
		"aic", CGU_CLK_GATE,
		.parents = { T31_CLK_I2ST, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 11 },
	},

	[T31_CLK_GATE_SMB1] = {
		"smb1", CGU_CLK_GATE,
		.parents = { T31_CLK_PCLK, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 8 },
	},

	[T31_CLK_GATE_SMB0] = {
		"smb0", CGU_CLK_GATE,
		.parents = { T31_CLK_PCLK, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 7 },
	},

	[T31_CLK_GATE_SSI0] = {
		"ssi0", CGU_CLK_GATE,
		.parents = { T31_CLK_SSI, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 6 },
	},

	[T31_CLK_GATE_MSC1] = {
		"gate_msc1", CGU_CLK_GATE,
		.parents = { T31_CLK_MSC1, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 5 },
	},

	[T31_CLK_GATE_MSC0] = {
		"gate_msc0", CGU_CLK_GATE,
		.parents = { T31_CLK_MSC0, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 4 },
	},

	[T31_CLK_GATE_OTG] = {
		"otg", CGU_CLK_GATE,
		.parents = { T31_CLK_AHB2, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 3 },
	},

	[T31_CLK_GATE_EFUSE] = {
		"efuse", CGU_CLK_GATE,
		.parents = { T31_CLK_AHB2, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 1 },
	},

	[T31_CLK_GATE_NEMC] = {
		"nemc", CGU_CLK_GATE,
		.parents = { T31_CLK_AHB2, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR0, 0 },
	},

	[T31_CLK_GATE_CPU] = {
		"gate_cpu", CGU_CLK_GATE,
		.flags = CLK_IS_CRITICAL,
		.parents = { T31_CLK_CPU, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR1, 15 },
	},

	[T31_CLK_GATE_APB0] = {
		"apb0", CGU_CLK_GATE,
		.flags = CLK_IGNORE_UNUSED,
		.parents = { T31_CLK_AHB0, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR1, 14 },
	},

	[T31_CLK_GATE_OST] = {
		"ost_gate", CGU_CLK_GATE,
		.flags = CLK_IS_CRITICAL,
		.parents = { T31_CLK_EXCLK, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR1, 11 },
	},

	[T31_CLK_GATE_AHB0] = {
		"gate_ahb0", CGU_CLK_GATE,
		.flags = CLK_IGNORE_UNUSED,
		.parents = { T31_CLK_AHB0, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR1, 10 },
	},

	[T31_CLK_GATE_AHB1] = {
		"ahb1", CGU_CLK_GATE,
		.parents = { T31_CLK_AHB2, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR1, 6 },
	},

	[T31_CLK_GATE_AES] = {
		"aes", CGU_CLK_GATE,
		.parents = { T31_CLK_AHB2, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR1, 5 },
	},

	[T31_CLK_GATE_GMAC] = {
		"gmac", CGU_CLK_GATE,
		.parents = { T31_CLK_MAC, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR1, 4 },
	},

	[T31_CLK_GATE_IPU] = {
		"ipu", CGU_CLK_GATE,
		.parents = { T31_CLK_AHB0, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR1, 2 },
	},

	[T31_CLK_GATE_DTRNG] = {
		"dtrng", CGU_CLK_GATE,
		.parents = { T31_CLK_PCLK, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR1, 1 },
	},

	[T31_CLK_GATE_EL150] = {
		"gate_el150", CGU_CLK_GATE,
		.parents = { T31_CLK_EL150, -1, -1, -1 },
		.gate = { CGU_REG_CLKGR1, 0 },
	},
};

static void __init t31_cgu_init(struct device_node *np)
{
	int retval;

	cgu = ingenic_cgu_new(t31_cgu_clocks,
			      ARRAY_SIZE(t31_cgu_clocks), np);
	if (!cgu) {
		pr_err("%s: failed to initialise CGU\n", __func__);
		return;
	}

	retval = ingenic_cgu_register_clocks(cgu);
	if (retval) {
		pr_err("%s: failed to register CGU Clocks\n", __func__);
		return;
	}

	ingenic_cgu_register_syscore(cgu);
}
/*
 * CGU has some children devices, this is useful for probing children devices
 * in the case where the device node is compatible with "simple-mfd".
 */
CLK_OF_DECLARE_DRIVER(t31_cgu, "ingenic,t31-cgu", t31_cgu_init);
