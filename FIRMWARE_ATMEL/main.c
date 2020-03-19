// Copyright (C) 2020 IAM Electronic GmbH <info@iamelectronic.com>
// This work is free. You can redistribute it and/or modify it under the
// terms of the Do What The Fuck You Want To Public License, Version 2,
// as published by Sam Hocevar. See http://www.wtfpl.net/ for more details.
// ************************************************************************
// File Name	: 'main.c'
// Title		: Main loop for FMC FRU Programmer firmware
// Company		: IAM Electronic GmbH
// Author		: PFH
// Created		: 19-MARCH-2020
// Target HW	: T0009 FMC FRU EEPROM Programmer, ATMEGA32U4
// Target IDE   : Atmel Studio 7 (Version 7.0.2397)
// ************************************************************************

#include "./includes/fru_programmer.h"
#include "./includes/usb_serial.h"
#include "./includes/i2c.h"

#include <avr/pgmspace.h>  // AVR stuff
#include <stdint.h>        // types
#include <util/delay.h>    // delay timer function

void init(void)
{    	
	init_IOports();
	
	set_led(LED_ALL,LED_ON);            // catch some attention, all LEDs on
	
	usb_init();

	while (!usb_configured())
	{
		set_led(LED_YELLOW,LED_TOGGLE); // yellow LED indicates busy state
		_delay_ms(50);
	}
		
	i2c_init();
	
	set_led(LED_ALL,LED_OFF);
}

int main(void)
{
   uint8_t i;                          // default loop variable   
   uint8_t task;                       // task code received from usb uart   
   uint8_t i2c_addr;                   // EEPROM's I2C address
   uint8_t bytes_to_read;              // number of bytes to read with a I2C read burst
   uint8_t bytes_to_write;             // number of bytes to write with a I2C write burst
   uint8_t fru_addr_msb;               // EEPROM target address for read/write access
   uint8_t fru_addr_lsb;               // EEPROM target address for read/write access, LSB is not used in case of 1 byte addressing
   uint8_t fru_data;                   // EEPROM target data
   uint8_t i2c_buf[I2C_BUFFERSIZE];    // buffer for I2C bus data
   
   init();                             // initializes hardware 
       
   usb_serial_flush_input();           // discard anything that was received prior
   
   bytes_to_read  = I2C_DEFAULT_READ;  // default value supported by all EEPROMs
   bytes_to_write = I2C_DEFAULT_WRITE; // default value supported by all EEPROMs
   
   while (1)                           // main loop
   {	
	  
	  // GREEN LED SHOULD BE TURNED ON WHEN FMC MODULE IS PLUGGED IN
      if (get_prsnt_state()==INPUT_LOW)
         set_led(LED_GREEN,LED_ON);                                  // FMC Module is plugged in
      else
         set_led(LED_GREEN,LED_OFF);                                 // FMC Module is not plugged in
	
      // SET WRITE PIN, STATE DEPENDS ON WR_POL PIN (DIP SWITCH SW1)
      if (get_wrpol_state()==INPUT_LOW)                              // 0 = active low write enable (WR pin will be low during write cycles, high in idle)
         set_writepin(WR_ON);                                        // enable write protection (write enable is active low, idle is high)
      else
		 set_writepin(WR_OFF);                                       // enable write protection (write enable is active high, idle is low)
		 
	  // READ NEW TASK FROM USB UART
      if (usb_serial_available()>0)                                  // valid data is in rx buffer
      {
         set_led(LED_YELLOW,LED_ON);                                 // indicate busy state
         task = usb_serial_getchar();                                // get first byte from buffer, that code determines next task
         switch(task)                                                // decode task
         {
			case 'b': // 0x62 b = bytes to read in a burst
			          if (usb_serial_available()==1)                 // command has one argument
			          {  
				         i = usb_serial_getchar();                   // get next byte from recv buffer
						 if ((i>=1) && (i<=I2C_MAX_READ))
						 {
                            bytes_to_read = i;
						    usb_serial_putchar(UART_ACK);            // send ACK
							usb_serial_putchar(bytes_to_read);       // send current value	 
						 }
						 else
						 {
                            bytes_to_read = I2C_DEFAULT_READ;        // restore defaults
							usb_serial_putchar(UART_NACK);           // wrong range of parameter
						 }
			          }
					  else if (usb_serial_available()==0)            // command has one argument
					  {
						  usb_serial_putchar(bytes_to_read);         // print current settings
					  }
			          else
			          {
				         usb_serial_putchar(UART_NACK);              // not enough data, end of transmission
			          }
			          break;
					  			 
            case 'f': // 0x66 f = printf 0xff
			          usb_serial_putchar(0xFF);
                      break;
					  
            case 'g': // 0x67 g = global address (GA) pins of dip switch SW1
			          i2c_addr = get_GA_state();                     // read GA1 and GA0 pins 
                      usb_serial_putchar(i2c_addr);                  // print values
                      break;
					  
            case 'p': // 0x70 p = presence of the FMC module
                      if (get_prsnt_state()==INPUT_LOW)
                         usb_serial_putchar(0x01);                   // FMC Module is attached, send 0x01
                      else
                         usb_serial_putchar(0x00);                   // FMC Module is not attached, send 0x00
                      break;
					  
            case 'P': // 0x50 P = Protection polarity (possible write protect polarity of EEPROM)
                      if (get_wrpol_state()==INPUT_LOW)
                         usb_serial_putchar(0x00);                   // WR_POL is low (dip switch SW1)
                      else
                         usb_serial_putchar(0x01);                   // WR_POL is high (dip switch SW1)                      
                      break;
					  								  					  
            case 'r': // 0x72 r = read with 1 byte addressing
			          if (usb_serial_available()==2)                 // command has two arguments
			          {
				          usb_serial_putchar(UART_ACK);              // send ACK
						  i2c_addr     = usb_serial_getchar();       // get next byte from recv buffer
				          fru_addr_msb = usb_serial_getchar();       // get next byte from recv buffer
						  i2c_buf[0]   = fru_addr_msb;               // generate I2C data buffer
						  set_writepin(WR_TOGGLE);                               // mask read access with WR pin
						  i2c_write(i2c_addr, (uint8_t*)i2c_buf, 1);             // transmit adddr to read				  
						  i2c_read(i2c_addr,  (uint8_t*)i2c_buf, bytes_to_read); // read data into same buffer
						  set_writepin(WR_TOGGLE);                               // unmask read access with WR pin
						  for (i=0;i<bytes_to_read;i++)
						     usb_serial_putchar(i2c_buf[i]);                     // send I2C readoutdata
			          }
			          else
			          {
				          usb_serial_putchar(UART_NACK);             // not enough data, end of transmission
			          }
                      break;
					  
            case 'R': // 0x52 R = Read with 2 byte addressing
			          if (usb_serial_available()==3)                 // command has three arguments
			          {
				          usb_serial_putchar(UART_ACK);              // send ACK
				          i2c_addr     = usb_serial_getchar();       // get next byte from recv buffer
				          fru_addr_msb = usb_serial_getchar();       // get next byte from recv buffer
				          fru_addr_lsb = usb_serial_getchar();       // get next byte from recv buffer
						  i2c_buf[0]   = fru_addr_msb;               // generate I2C data buffer
						  i2c_buf[1]   = fru_addr_lsb;               // generate I2C data buffer
						  set_writepin(WR_TOGGLE);                               // mask read access with WR pin
						  i2c_write(i2c_addr, (uint8_t*)i2c_buf, 2);             // transmit addr to read						  
						  i2c_read(i2c_addr,  (uint8_t*)i2c_buf, bytes_to_read); // read data into same buffer
						  set_writepin(WR_TOGGLE);                               // unmask read access with WR pin
						  for (i=0;i<bytes_to_read;i++)
						     usb_serial_putchar(i2c_buf[i]);                     // send I2C readoutdata
			          }
			          else
			          {
				          usb_serial_putchar(UART_NACK);             // not enough data, end of transmission
			          }
                      break;
					  								  
            case 's': // 0x73 s = scan for EEPROM addresses
				      for (i = 0; i < 8; i++)
				      {
					     i2c_addr = I2C_EEPROM_ADDR_7BIT | i;        // scan on fixed EEPROM address and increment lowest 3 bits
					     if(i2c_scan(i2c_addr) == 1)                
					        usb_serial_putchar(i2c_addr);            // print valid I2C address
				      }
				      usb_serial_putchar(UART_END);                  // end of transmission
                      break;
					  
            case 'v': // 0x76 v = version of firmware
					  usb_serial_putchar(FRU_PROGRAMMER_FW_REL_MAJ);
					  usb_serial_putchar(FRU_PROGRAMMER_FW_REL_MIN);
					  usb_serial_putchar(FRU_PROGRAMMER_FW_BUILD);
					  usb_serial_putchar(UART_END);                  // end of transmission
                      break;
					  
            case 'w': // 0x77 w = write with 1 byte addressing
			          if (usb_serial_available()>=3)                 // command has at least three arguments
			          {
						  bytes_to_write = usb_serial_available() - 2; // get number of bytes in commando string (offset 2: 1 byte I2C addr, 1 byte mem addr)
				          usb_serial_putchar(UART_ACK);                // send ACK
				          i2c_addr     = usb_serial_getchar();         // get next byte from recv buffer
				          fru_addr_msb = usb_serial_getchar();         // get next byte from recv buffer				          				  
						  i2c_buf[0]   = fru_addr_msb;                 // generate I2C data buffer
						  for (i=0;i<bytes_to_write;i++)
						  {
							  fru_data     = usb_serial_getchar();     // get next byte from recv buffer		
							  i2c_buf[1+i] = fru_data;                 // generate I2C data buffer
						  }
						  set_writepin(WR_TOGGLE);                                  // toggle WR pin
						  i2c_write(i2c_addr, (uint8_t*)i2c_buf, 1+bytes_to_write); // transmit data and write to EEPROM
						  set_writepin(WR_TOGGLE);                                  // toggle WR pin
			          }
			          else
			          {
				          usb_serial_putchar(UART_NACK);             // not enough data, end of transmission
			          }
                      break;
					  
            case 'W': // 0x57 w = write with 2 byte addressing
			          if (usb_serial_available()>=4)                 // command has at least four arguments
					  {
						 bytes_to_write = usb_serial_available() - 3; // get number of bytes in commando string (offset 3: 1 byte I2C addr, 2 byte mem addr)
                         usb_serial_putchar(UART_ACK);                // send ACK
			             i2c_addr     = usb_serial_getchar();         // get next byte from recv buffer
			             fru_addr_msb = usb_serial_getchar();         // get next byte from recv buffer
					     fru_addr_lsb = usb_serial_getchar();         // get next byte from recv buffer			             
						 i2c_buf[0]   = fru_addr_msb;                 // generate I2C data buffer
						 i2c_buf[1]   = fru_addr_lsb;                 // generate I2C data buffer
						 for (i=0;i<bytes_to_write;i++)
						 {
							 fru_data     = usb_serial_getchar();     // get next byte from recv buffer
							 i2c_buf[2+i] = fru_data;                 // generate I2C data buffer
						 }						 
						 set_writepin(WR_TOGGLE);                                   // toggle WR pin
						 i2c_write(i2c_addr, (uint8_t*)i2c_buf, 2+bytes_to_write);  // transmit data and write to EEPROM
						 set_writepin(WR_TOGGLE);                                   // toggle WR pin
					  }
					  else
					  {
                         usb_serial_putchar(UART_NACK);              // not enough data, end of transmission
					  }
                      break;					  
         } // end switch
         usb_serial_flush_input();
         set_led(LED_YELLOW,LED_OFF);
      } // end if
   } // end while(1) main loop
}