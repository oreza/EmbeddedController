/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "adc.h"
#include "board.h"
#include "common.h"
#include "console.h"
#include "debug.h"
#include "hooks.h"
#include "registers.h"
#include "timer.h"
#include "util.h"
#include "usb_pd.h"

/* ------------------------- Power supply control ------------------------ */

/* GPIO level setting helpers through BSRR register */
#define GPIO_SET(n)   (1 << (n))
#define GPIO_RESET(n) (1 << ((n) + 16))

/* Output voltage selection */
enum volt {
	VO_5V  = GPIO_RESET(13) | GPIO_RESET(14),
	VO_12V = GPIO_SET(13)   | GPIO_RESET(14),
	VO_13V = GPIO_RESET(13) | GPIO_SET(14),
	VO_20V = GPIO_SET(13)   | GPIO_SET(14),
};

static inline void set_output_voltage(enum volt v)
{
	/* set voltage_select on PA13/PA14 */
	STM32_GPIO_BSRR(GPIO_A) = v;
}

static inline void output_enable(void)
{
	/* GPF0 (enable OR'ing FETs) = 1 */
	STM32_GPIO_BSRR(GPIO_F) = GPIO_SET(0);
}

static inline void output_disable(void)
{
	/* GPF0 (disable OR'ing FETs) = 0 */
	STM32_GPIO_BSRR(GPIO_F) = GPIO_RESET(0);
}

static inline int output_is_enabled(void)
{
	/* GPF0 = enable output FET */
	return STM32_GPIO_IDR(GPIO_F) & 1;
}

/* ----- fault conditions ----- */

enum faults {
	FAULT_OK = 0,
	FAULT_OCP, /* Over-Current Protection */
	FAULT_OVP, /* Under or Over-Voltage Protection */
};

/* current fault condition */
static enum faults fault;
/* expiration date of the last fault condition */
static timestamp_t fault_deadline;

/* ADC in 12-bit mode */
#define ADC_SCALE (1 << 12)
/* ADC power supply : VDDA = 3.3V */
#define VDDA_MV   3300
/* Current sense resistor : 5 milliOhm */
#define R_SENSE   5
/* VBUS voltage is measured through 10k / 100k voltage divider = /11 */
#define VOLT_DIV  ((10+110)/10)
/* The current sensing op-amp has a x101 gain */
#define CURR_GAIN 101
/* convert VBUS voltage in raw ADC value */
#define VBUS_MV(mv) ((mv)*ADC_SCALE/VOLT_DIV/VDDA_MV)
/* convert VBUS current in raw ADC value */
#define VBUS_MA(ma) ((ma)*ADC_SCALE*R_SENSE/1000*CURR_GAIN/VDDA_MV)

/* Max current : 10% over 3A = 3.3A */
#define MAX_CURRENT VBUS_MA(3300)
/* reset over-current after 1 second */
#define OCP_TIMEOUT SECOND

/* Under-voltage limit is 0.8x Vnom */
#define UVP_MV(mv)  VBUS_MV((mv) * 8 / 10)
/* Over-voltage limit is 1.2x Vnom */
#define OVP_MV(mv)  VBUS_MV((mv) * 12 / 10)

/* ----------------------- USB Power delivery policy ---------------------- */

/* Power Delivery Objects */
const uint32_t pd_src_pdo[] = {
		PDO_FIXED(5000,   500, PDO_FIXED_EXTERNAL),
		PDO_FIXED(5000,  3000, 0),
		PDO_FIXED(12000, 3000, 0),
		PDO_FIXED(20000, 2000, 0),
};
const int pd_src_pdo_cnt = ARRAY_SIZE(pd_src_pdo);

/* PDO voltages (should match the table above) */
static const struct {
	enum volt select; /* GPIO configuration to select the voltage */
	int       uvp;    /* under-voltage limit in mV */
	int       ovp;    /* over-voltage limit in mV */
} voltages[ARRAY_SIZE(pd_src_pdo)] = {
	{VO_5V,  UVP_MV(5000),  OVP_MV(5000)},
	{VO_5V,  UVP_MV(5000),  OVP_MV(5000)},
	{VO_12V, UVP_MV(12000), OVP_MV(12000)},
	{VO_20V, UVP_MV(20000), OVP_MV(20000)},
};

/* currently selected PDO entry */
static int volt_idx;

int pd_request_voltage(uint32_t rdo)
{
	int op_ma = rdo & 0x3FF;
	int max_ma = (rdo >> 10) & 0x3FF;
	int idx = rdo >> 28;
	uint32_t pdo;
	uint32_t pdo_ma;


	/* fault condition not cleared : reject transitions */
	if (fault != FAULT_OK)
		return EC_ERROR_INVAL;

	if (!idx || idx > pd_src_pdo_cnt)
		return EC_ERROR_INVAL; /* Invalid index */

	/* check current ... */
	pdo = pd_src_pdo[idx - 1];
	pdo_ma = (pdo & 0x3ff);
	if (op_ma > pdo_ma)
		return EC_ERROR_INVAL; /* too much op current */
	if (max_ma > pdo_ma)
		return EC_ERROR_INVAL; /* too much max current */

	debug_printf("Switch to %d V %d mA (for %d/%d mA)\n",
		     ((pdo >> 10) & 0x3ff) * 50, (pdo & 0x3ff) * 10,
		     ((rdo >> 10) & 0x3ff) * 10, (rdo & 0x3ff) * 10);

	output_disable();
	/* TODO discharge ? */
	volt_idx = idx - 1;
	set_output_voltage(voltages[volt_idx].select);

	return EC_SUCCESS;
}

int pd_set_power_supply_ready(void)
{
	/* fault condition not cleared : do not turn on power */
	if (fault != FAULT_OK)
		return EC_ERROR_INVAL;

	output_enable();
	return EC_SUCCESS; /* we are ready */
}

void pd_power_supply_reset(void)
{
	output_disable();
	/* TODO discharge ? */
	volt_idx = 0;
	set_output_voltage(VO_5V);
	/* TODO transition delay */
}

int pd_board_checks(void)
{
	int vbus_volt, vbus_amp;

	vbus_volt = adc_read_channel(ADC_CH_V_SENSE);
	vbus_amp = adc_read_channel(ADC_CH_A_SENSE);

	if (vbus_amp > MAX_CURRENT) {
		debug_printf("OverCurrent : %d mA\n",
		  vbus_amp * VDDA_MV / CURR_GAIN * 1000 / R_SENSE / ADC_SCALE);
		fault = FAULT_OCP;
		/* reset over-current after 1 second */
		fault_deadline.val = get_time().val + OCP_TIMEOUT;
		return EC_ERROR_INVAL;
	}
	if (output_is_enabled() && (vbus_volt > voltages[volt_idx].ovp)) {
		debug_printf("OverVoltage : %d mV\n",
			     vbus_volt * VDDA_MV * VOLT_DIV / ADC_SCALE);
		/* TODO(crosbug.com/p/28331) discharge */
		fault = FAULT_OVP;
		/* no timeout */
		fault_deadline.val = get_time().val;
		return EC_ERROR_INVAL;
	}

	/* everything is good *and* the error condition has expired */
	if ((fault != FAULT_OK) && (get_time().val > fault_deadline.val)) {
		fault = FAULT_OK;
		debug_printf("Reset fault\n");
	}

	return EC_SUCCESS;

}
