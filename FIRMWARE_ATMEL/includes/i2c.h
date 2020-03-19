// Copyright (C) 2020 IAM Electronic GmbH <info@iamelectronic.com>
// This work is free. You can redistribute it and/or modify it under the
// terms of the Do What The Fuck You Want To Public License, Version 2,
// as published by Sam Hocevar. See http://www.wtfpl.net/ for more details.
// ************************************************************************
// File Name	: 'i2c.h'
// Title		: I²C-Library using the 2-wire Serial Interface (TWI) of the Atmega32u4
// Company		: IAM Electronic GmbH
// Author		: BER
// Created		: 19-MARCH-2020
// Target HW	: T0009 FMC FRU EEPROM Programmer, ATMEGA32U4
// Target IDE   : Atmel Studio 7 (Version 7.0.2397)
// ************************************************************************

#ifndef FRU_I2C_H
#define FRU_I2C_H

#include <avr/pgmspace.h>

//#define SCL_CLOCK 100000L

void i2c_init();
void i2c_writeByte(uint8_t data);
void i2c_startCondition();
void i2c_stopCondition();
void i2c_write(uint8_t address, uint8_t* buffer, uint8_t length);
uint8_t i2c_read(uint8_t address, uint8_t* buffer, uint8_t bytes);
uint8_t i2c_scan(uint8_t address);

#endif