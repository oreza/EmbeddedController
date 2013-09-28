/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/* Emulator board-specific configuration */

#include "extpower.h"
#include "gpio.h"
#include "lid_switch.h"
#include "power_button.h"
#include "temp_sensor.h"
#include "util.h"

#define MOCK_GPIO(x) {#x, 0, 0, 0, 0}
#define MOCK_GPIO_INT(x, i, r) {#x, 0, 0, i, r}

const struct gpio_info gpio_list[] = {
	MOCK_GPIO(EC_INT),
	MOCK_GPIO_INT(LID_OPEN, GPIO_INT_BOTH, lid_interrupt),
	MOCK_GPIO_INT(POWER_BUTTON_L, GPIO_INT_BOTH, power_button_interrupt),
	MOCK_GPIO(WP),
	MOCK_GPIO(ENTERING_RW),
	MOCK_GPIO_INT(AC_PRESENT, GPIO_INT_BOTH, extpower_interrupt),
	MOCK_GPIO(PCH_BKLTEN),
	MOCK_GPIO(ENABLE_BACKLIGHT),
};
BUILD_ASSERT(ARRAY_SIZE(gpio_list) == GPIO_COUNT);

/* Pins with alternate functions; not on simulated host platform */
const struct gpio_alt_func gpio_alt_funcs[] = {
};
const int gpio_alt_funcs_count = ARRAY_SIZE(gpio_alt_funcs);

test_mockable_static int dummy_temp_get_val(int idx, int *temp_ptr)
{
	*temp_ptr = 0;
	return EC_SUCCESS;
}

const struct temp_sensor_t temp_sensors[] = {
	{"CPU", TEMP_SENSOR_TYPE_CPU, dummy_temp_get_val, 0, 3},
	{"Board", TEMP_SENSOR_TYPE_BOARD, dummy_temp_get_val, 1, 3},
	{"Case", TEMP_SENSOR_TYPE_CASE, dummy_temp_get_val, 2, 0},
	{"Battery", TEMP_SENSOR_TYPE_BOARD, dummy_temp_get_val, 3, 0},
};
BUILD_ASSERT(ARRAY_SIZE(temp_sensors) == TEMP_SENSOR_COUNT);
