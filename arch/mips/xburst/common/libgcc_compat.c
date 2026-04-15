// SPDX-License-Identifier: GPL-2.0
/*
 * GCC runtime helpers for 64-bit arithmetic on 32-bit MIPS.
 *
 * Out-of-tree modules (ISP tuning, etc.) use u64 division which causes
 * the compiler to emit __udivdi3/__umoddi3 calls. These aren't in the
 * kernel on 7.0+ since in-tree code uses do_div(). Provide them here.
 */

#include <linux/module.h>
#include <linux/math64.h>

unsigned long long __udivdi3(unsigned long long dividend, unsigned long long divisor)
{
	return div64_u64(dividend, divisor);
}
EXPORT_SYMBOL(__udivdi3);

unsigned long long __umoddi3(unsigned long long dividend, unsigned long long divisor)
{
	unsigned long long rem;
	div64_u64_rem(dividend, divisor, &rem);
	return rem;
}
EXPORT_SYMBOL(__umoddi3);
