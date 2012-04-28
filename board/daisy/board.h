/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Daisy board configuration */

#ifndef __BOARD_H
#define __BOARD_H

/* 16 MHz SYSCLK clock frequency */
#define CPU_CLOCK 16000000

/* Use USART1 as console serial port */
#define CONFIG_CONSOLE_UART 1

#define USB_CHARGE_PORT_COUNT 0

/* GPIO signal list */
enum gpio_signal {
	/* Inputs with interrupt handlers are first for efficiency */
	GPIO_KB_PWR_ON = 0,    /* Keyboard power button */
	GPIO_PP1800_LDO2,      /* LDO2 is ON (end of PMIC sequence) */
	GPIO_SOC1V8_XPSHOLD,   /* App Processor ON  */
	GPIO_CHARGER_INT,
	GPIO_LID_OPEN,         /* LID switch detection */
	/* Keyboard inputs */
	GPIO_KB_IN00,
	GPIO_KB_IN01,
	GPIO_KB_IN02,
	GPIO_KB_IN03,
	GPIO_KB_IN04,
	GPIO_KB_IN05,
	GPIO_KB_IN06,
	GPIO_KB_IN07,
	/* Other inputs */
	GPIO_SPI1_NSS,
	/* Outputs */
	GPIO_EN_PP1350,        /* DDR 1.35v rail enable */
	GPIO_EN_PP5000,        /* 5.0v rail enable */
	GPIO_EN_PP3300,        /* 3.3v rail enable */
	GPIO_PMIC_PWRON,       /* 5v rail ready */
	GPIO_EC_ENTERING_RW,   /* EC is R/W mode for the kbc mux */
	GPIO_CHARGER_EN,
	GPIO_EC_INT,
	GPIO_CODEC_INT,        /* To audio codec (KB noise cancellation) */
	GPIO_KB_OUT00,
	GPIO_KB_OUT01,
	GPIO_KB_OUT02,
	GPIO_KB_OUT03,
	GPIO_KB_OUT04,
	GPIO_KB_OUT05,
	GPIO_KB_OUT06,
	GPIO_KB_OUT07,
	GPIO_KB_OUT08,
	GPIO_KB_OUT09,
	GPIO_KB_OUT10,
	GPIO_KB_OUT11,
	GPIO_KB_OUT12,
	GPIO_I2C_SCL,
	GPIO_I2C_SDA,
	/* Number of GPIOs; not an actual GPIO */
	GPIO_COUNT
};

void configure_board(void);

void matrix_interrupt(enum gpio_signal signal);

/* Signal to the AP that keyboard scan data is available */
void board_keyboard_scan_ready(void);

#endif /* __BOARD_H */
