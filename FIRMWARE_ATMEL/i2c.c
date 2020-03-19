// Copyright (C) 2020 IAM Electronic GmbH <info@iamelectronic.com>
// This work is free. You can redistribute it and/or modify it under the
// terms of the Do What The Fuck You Want To Public License, Version 2,
// as published by Sam Hocevar. See http://www.wtfpl.net/ for more details.
// ************************************************************************
// File Name	: 'i2c.c'
// Title		: I²C-Library using the 2-wire Serial Interface (TWI) of the Atmega32u4
// Company		: IAM Electronic GmbH
// Author		: BER
// Created		: 19-MARCH-2020
// Target HW	: T0009 FMC FRU EEPROM Programmer, ATMEGA32U4
// Target IDE   : Atmel Studio 7 (Version 7.0.2397)
// ************************************************************************

#include "./includes/i2c.h"

void i2c_init()
{
	TWCR = 0;
	PORTD &= ~(1 << 0); // Port D0 SCL, Tri-State, has external Pull-Up
	PORTD &= ~(1 << 1); // Port D1 SDA, Tri-State, has external Pull-Up
	TWSR = 0;           // Prescaler Options, keep default
	TWBR = 32;			// 100kHz = SCL clockspeed = f_cpu / (16 + 2*TWBR*1)	 
}

void i2c_writeByte(uint8_t data)
{
	TWDR = data;
	TWCR = (1 << TWINT) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)));
}

void i2c_startCondition()
{
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)));  // Start Condition sent
}

void i2c_stopCondition()
{
	TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
}

void i2c_write(uint8_t address, uint8_t* buffer, uint8_t length)
{
	i2c_startCondition();

	i2c_writeByte(2*address);	// even address => write

	for (uint8_t i = 0; i < length; i++)
	{
		i2c_writeByte(buffer[i]);	// even address => write
	}
    i2c_stopCondition();
}


uint8_t i2c_read(uint8_t address, uint8_t* buffer, uint8_t length)
{
	uint16_t timeout = 0;
	
	i2c_startCondition();
	
	i2c_writeByte(2*address + 1); // odd address => read
	
	for (int i = 0; i < length; i++)
	{	
		if (i == (length - 1))
		{
			TWCR = (1 << TWINT) | (1 << TWEN);
		}
		else
		{
			TWCR = (1 << TWINT) | (1 << TWEA) | (1 << TWEN);
		}
		timeout = 0;
		while (!(TWCR & (1 << TWINT)))
		{
			timeout++;
			if (timeout > 50000)
			{
				return 0;
			}
		}
		
		buffer[i] = TWDR;
	}

	i2c_stopCondition();
	return 1;
}

// check if i2c device exists on this address
uint8_t i2c_scan(uint8_t address)
{
	i2c_startCondition();
	TWDR = 2*address;
	TWCR = (1 << TWINT) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)));
	if((TWSR & 0xF8) != 0x18) // no ACK received from slave => no device on this address
	{
		i2c_stopCondition();
		return 0;
	}
	i2c_stopCondition();
	return 1;
}
