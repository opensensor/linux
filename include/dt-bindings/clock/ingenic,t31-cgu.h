/* SPDX-License-Identifier: GPL-2.0 */
/*
 * This header provides clock numbers for the ingenic,t31-cgu DT binding.
 *
 * They are roughly ordered as:
 *   - external clocks
 *   - PLLs
 *   - muxes/dividers in the order they appear in the T31 programmers manual
 *   - gates in order of their bit in the CLKGR* registers
 */

#ifndef __DT_BINDINGS_CLOCK_T31_CGU_H__
#define __DT_BINDINGS_CLOCK_T31_CGU_H__

#define T31_CLK_EXCLK		0
#define T31_CLK_RTCLK		1
#define T31_CLK_APLL		2
#define T31_CLK_MPLL		3
#define T31_CLK_VPLL		4
#define T31_CLK_OTGPHY		5
#define T31_CLK_SCLKA		6
#define T31_CLK_CPUMUX		7
#define T31_CLK_CPU		8
#define T31_CLK_L2CACHE		9
#define T31_CLK_AHB0		10
#define T31_CLK_AHB2MUX		11
#define T31_CLK_AHB2		12
#define T31_CLK_PCLK		13
#define T31_CLK_DDR		14
#define T31_CLK_MAC		15
#define T31_CLK_LCD		16
#define T31_CLK_MSC0		17
#define T31_CLK_MSC1		18
#define T31_CLK_SSI		19
#define T31_CLK_SFC		20
#define T31_CLK_CIM		21
#define T31_CLK_ISP		22
#define T31_CLK_RSA		23
#define T31_CLK_I2ST		24
#define T31_CLK_I2SR		25
#define T31_CLK_EL150		26
#define T31_CLK_GATE_DDR	27
#define T31_CLK_GATE_TCU	28
#define T31_CLK_GATE_DES	29
#define T31_CLK_GATE_RSA	30
#define T31_CLK_GATE_CSI	31
#define T31_CLK_GATE_LCD	32
#define T31_CLK_GATE_ISP	33
#define T31_CLK_GATE_PDMA	34
#define T31_CLK_GATE_SFC	35
#define T31_CLK_GATE_SSI1	36
#define T31_CLK_GATE_HASH	37
#define T31_CLK_GATE_SLV	38
#define T31_CLK_GATE_UART2	39
#define T31_CLK_GATE_UART1	40
#define T31_CLK_GATE_UART0	41
#define T31_CLK_GATE_SADC	42
#define T31_CLK_GATE_DMIC	43
#define T31_CLK_GATE_AIC	44
#define T31_CLK_GATE_SMB1	45
#define T31_CLK_GATE_SMB0	46
#define T31_CLK_GATE_SSI0	47
#define T31_CLK_GATE_MSC1	48
#define T31_CLK_GATE_MSC0	49
#define T31_CLK_GATE_OTG	50
#define T31_CLK_GATE_EFUSE	51
#define T31_CLK_GATE_NEMC	52
#define T31_CLK_GATE_CPU	53
#define T31_CLK_GATE_APB0	54
#define T31_CLK_GATE_OST	55
#define T31_CLK_GATE_AHB0	56
#define T31_CLK_GATE_AHB1	57
#define T31_CLK_GATE_AES	58
#define T31_CLK_GATE_GMAC	59
#define T31_CLK_GATE_IPU	60
#define T31_CLK_GATE_DTRNG	61
#define T31_CLK_GATE_EL150	62

#endif /* __DT_BINDINGS_CLOCK_T31_CGU_H__ */
