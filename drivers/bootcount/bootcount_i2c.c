// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2013
 * Heiko Schocher, DENX Software Engineering, hs@denx.de.
 */

#include <bootcount.h>
#include <linux/compiler.h>
#include <i2c.h>

#ifndef CONFIG_SYS_BOOTCOUNT_I2C_ADDR
/* compatibility with the previous logic:
 * previous version of driver used RTC device to store bootcount
 */
#define CONFIG_SYS_BOOTCOUNT_I2C_ADDR CONFIG_SYS_I2C_RTC_ADDR
#endif

#define BC_MAGIC	0xbc

#ifdef CONFIG_SYS_BOOTCOUNT_I2C_BUS
static int bootcount_set_bus(void)
{
	unsigned int current_bus = i2c_get_bus_num();

	assert(current_bus <= INT_MAX);

	int res = i2c_set_bus_num(CONFIG_SYS_BOOTCOUNT_I2C_BUS);

	if (res < 0) {
		puts("Error switching I2C bus\n");
		return res;
	}
	return (int)current_bus;
}

static void bootcount_set_bus_back(int prev_bus)
{
	if (i2c_set_bus_num(prev_bus) < 0)
		puts("Can't switch I2C bus back\n");
}
#else
static inline void bootcount_set_bus(void) { return ; }

static inline int bootcount_set_bus_back(int prev_bus __attribute__((unused)))
{
	return 0;
}
#endif

void bootcount_store(ulong a)
{
	int prev_i2c_bus = bootcount_set_bus();

	if (prev_i2c_bus < 0)
		return;

	unsigned char buf[2];
	int ret;

	BUILD_BUG_ON(sizeof(buf) > CONFIG_BOOTCOUNT_I2C_LEN);
	buf[0] = BC_MAGIC;
	buf[1] = (a & 0xff);
	ret = i2c_write(CONFIG_SYS_BOOTCOUNT_I2C_ADDR,
			CONFIG_SYS_BOOTCOUNT_ADDR,
			CONFIG_BOOTCOUNT_ALEN, buf, sizeof(buf));
	if (ret != 0)
		puts("Error writing bootcount\n");

	bootcount_set_bus_back(prev_i2c_bus);
}

ulong bootcount_load(void)
{
	ulong count = 0;

	int prev_i2c_bus = bootcount_set_bus();

	if (prev_i2c_bus < 0)
		return count;

	unsigned char buf[2];
	int ret;

	BUILD_BUG_ON(sizeof(buf) > CONFIG_BOOTCOUNT_I2C_LEN);
	ret = i2c_read(CONFIG_SYS_BOOTCOUNT_I2C_ADDR,
		       CONFIG_SYS_BOOTCOUNT_ADDR,
		       CONFIG_BOOTCOUNT_ALEN, buf, sizeof(buf));
	if (ret != 0) {
		puts("Error loading bootcount\n");
		goto out;
	}
	if (buf[0] == BC_MAGIC)
		count = buf[1];
	else
		bootcount_store(count);

out:
	bootcount_set_bus_back(prev_i2c_bus);
	return count;
}
