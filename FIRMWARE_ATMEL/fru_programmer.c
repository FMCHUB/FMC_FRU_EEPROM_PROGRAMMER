// Copyright © 2020 IAM Electronic GmbH <info@iamelectronic.com>
// This work is free. You can redistribute it and/or modify it under the
// terms of the Do What The Fuck You Want To Public License, Version 2,
// as published by Sam Hocevar. See http://www.wtfpl.net/ for more details.
// ************************************************************************
// File Name	: 'fru_programmer.c'
// Title		: Low level hardware functions
// Company		: IAM Electronic GmbH
// Author		: PFH
// Created		: 19-MARCH-2020
// Target HW	: T0009 FMC FRU EEPROM Programmer, ATMEGA32U4
// Target IDE   : Atmel Studio 7 (Version 7.0.2397)
// ************************************************************************

#include "./includes/fru_programmer.h"

/*
 * initializes all data direction registers
 */
void init_IOports(void)
{
   /*
   * HARDWARE PORTS ON FRU PROGRAMMER   
   * LED1 (green) : output port PB6
   * LED2 (yellow): output port PF6
   * WRITE        : output port PD4 (WR pin on FRU programmer)
   * 
   * PRSNT        : input  port PE6 (present pin from FMC module)
   * WR_POL       : input  port PD5 (dip switch SW1 on FRU programmer)
   * GA0          : input  port PD3 (dip switch SW1 on FRU programmer)
   * GA1          : input  port PD2 (dip switch SW1 on FRU programmer)
   */
   DDRB |=  (1 << PB6);  // LED1 (Yellow)
   DDRF |=  (1 << PF7);  // LED2 (Green)
   DDRD |=  (1 << PD4);  // WRITE

   DDRE  &= ~(1 << PE6);  // PRSNT
   PORTE |=  (1 << PE6);  // Use internal Pull-UP
		
   DDRD  &= ~(1 << PD5);  // WR_POL
   PORTD &= ~(1 << PD5);  // Tri-State, has external Pull-Up
   
   DDRD  &= ~(1 << PD3);  // GA0
   PORTD &= ~(1 << PD3);  // Tri-State, has external Pull-Up
   
   DDRD  &= ~(1 << PD2);  // GA1
   PORTD &= ~(1 << PD2);  // Tri-State, has external Pull-Up
}

/*
 * Changes state of LED
 */
void set_led(uint8_t led_idx, uint8_t state)
{	
	switch(state)
	{
		case LED_TOGGLE: if (led_idx==LED_ALL || led_idx==LED_YELLOW)
                            PORTB ^= (1 << PB6);
				         if (led_idx==LED_ALL || led_idx==LED_GREEN)
                            PORTF ^= (1 << PF7);
				         break;				 
		case  LED_OFF:   if (led_idx==LED_ALL || led_idx==LED_YELLOW)
		                    PORTB &= ~(1 << PB6);
				         if (led_idx==LED_ALL || led_idx==LED_GREEN)	
                            PORTF &= ~(1 << PF7);
		                 break;
		case  LED_ON:    if (led_idx==LED_ALL || led_idx==LED_YELLOW)
		                    PORTB |= (1 << PB6);
				         if (led_idx==LED_ALL || led_idx==LED_GREEN)	
					        PORTF |= (1 << PF7);
		                 break;
	}
}

/*
 * Changes state of WR pin
 */
void set_writepin(uint8_t state)
{	
	switch(state)
	{
		case WR_TOGGLE: PORTD ^=  (1 << PD4);
				        break;				 
		case WR_OFF:    PORTD &= ~(1 << PD4);
                        break;
		case WR_ON:     PORTD |=  (1 << PD4);
		                break;
	}
}

/*
 * Read PRSNT flag from FMC connector
 * returns 0 if FMC module is connected (PRSNT pin has GND potential = logical 0)
 * returns 1 if FMC module is not connected (PRSNT pin is floating)
 */
uint8_t get_prsnt_state(void)
{	
   if(PINE & (1 << PE6))
      return INPUT_HIGH;
   else
      return INPUT_LOW;
}

/*
 * Read WR_POL logical state from SW1
 * 0 = active low  write protect (WR pin is high during write cycles)
 * 1 = active high write protect (WR pin is low during write cycles)
 */
uint8_t get_wrpol_state(void)
{	
   if(PIND & (1 << PD5))
      return INPUT_HIGH;
   else
      return INPUT_LOW;
}

/*
 * Read GA1 and GA0 logical state from SW1
 */
uint8_t get_GA_state(void)
{
   uint8_t GA0 = 0x00;
   uint8_t GA1 = 0x00;
   uint8_t ret = 0x00;
   if(PIND & (1 << PD3))
      GA0 = 0x01;
   if(PIND & (1 << PD2))
	  GA1 = 0x01;
   ret |= (GA1 << 1);
   ret |= (GA0 << 0);
   return ret;
}