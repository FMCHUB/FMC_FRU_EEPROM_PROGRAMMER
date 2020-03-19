// Copyright (C) 2020 IAM Electronic GmbH <info@iamelectronic.com>
// This work is free. You can redistribute it and/or modify it under the
// terms of the Do What The Fuck You Want To Public License, Version 2,
// as published by Sam Hocevar. See http://www.wtfpl.net/ for more details.
// ************************************************************************
// File Name	: 'fru_programmer.h'
// Title		: Low level hardware functions
// Company		: IAM Electronic GmbH
// Author		: PFH
// Created		: 19-MARCH-2020
// Target HW	: T0009 FMC FRU EEPROM Programmer, ATMEGA32U4
// Target IDE   : Atmel Studio 7 (Version 7.0.2397)
// ************************************************************************

#ifndef FRU_PROGRAMMER_H
#define FRU_PROGRAMMER_H

#include <avr/io.h>

/*
 * Firmware release definitions
 */   
#define FRU_PROGRAMMER_FW_REL_MAJ   0x01	/* major release, 8 bit */
#define FRU_PROGRAMMER_FW_REL_MIN   0x01	/* minor release, 8 bit */
#define FRU_PROGRAMMER_FW_BUILD     0x01	/* build number   8 bit */

/*
 * EEPROM definitions
 */
#define I2C_EEPROM_ADDR_7BIT 0x50 /* 0b1010000 (0bnnnnXXX, XXX is determined by EEPROM size and GA[0 1] pins) */

/*
 * Clocking definitions
 */
#define F_CPU           8000000UL   // 8 MHz

/*
 * LED numbers and states
 */
#define LED_YELLOW  0x01
#define LED_GREEN   0x02
#define LED_ALL     0x00

#define LED_ON      0x01
#define LED_OFF     0x00
#define LED_TOGGLE  0x02

#define INPUT_LOW   0x00
#define INPUT_HIGH  0x01

/*
 * WRITE pin states
 */
#define WR_ON      0x01
#define WR_OFF     0x00
#define WR_TOGGLE  0x02

/*
 * helper definitions
 */
#define UART_ACK  0x06
#define UART_NACK '?'
#define UART_END  0xFF

#define I2C_BUFFERSIZE    67 // 1 BYTE I2C ADDR + 2 BYTES MEM ADDR + 64 BYTES
#define I2C_DEFAULT_READ  8  // READ 1 BYTE BY DEFAULT
#define I2C_MAX_READ      64 // READ 32 BYTE MAX IN A BURST
#define I2C_DEFAULT_WRITE 1  // WRITE 1 BYTE BY DEFAULT

/*
 * helper functions
 */
void    init_IOports(void);
void    set_led(uint8_t led_idx, uint8_t state);
void    set_writepin(uint8_t state);

uint8_t get_prsnt_state(void);
uint8_t get_wrpol_state(void);
uint8_t get_GA_state(void);

#endif