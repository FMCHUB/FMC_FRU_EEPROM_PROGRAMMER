// Copyright (C) 2020 IAM Electronic GmbH <info@iamelectronic.com>
// This work is free. You can redistribute it and/or modify it under the
// terms of the Do What The Fuck You Want To Public License, Version 2,
// as published by Sam Hocevar. See http://www.wtfpl.net/ for more details.
// ************************************************************************
// File Name	  : 'fmc_fru_programmer.c'
// Title		     : Command line tool for FMC FRU Programmer
// Company		  : IAM Electronic GmbH
// Author		  : PFH
// Created		  : 19-MARCH-2020
// Last modified : 19-MARCH-2020
// Target HW	  : T0009 FMC FRU Programmer
// Target OS     : Windows
// ************************************************************************
#include <windows.h>
#include <math.h>
#include <stdio.h>

#define REVISION_MAJOR 1
#define REVISION_MINOR 1
#define BUILD_NUMBER   1

#define WIN_COM_PORT_MAX_NO 255  // this will be the maximum number for scanning COM ports, COM0, ..., COMn, ..., COMmax

#define TX_BUFFER_SIZE 128       // default buffer size for TX bufer (UART communication)
#define RX_BUFFER_SIZE 128       // default buffer size for RX bufer (UART communication)

#define BADCH   (int)'?'
#define BADARG  (int)':'
#define EMSG    ""

int           d_task(HANDLE* hComm, unsigned char i2c_addr, unsigned char* N_addr, unsigned int* N_bytes, unsigned char read_burst, char* filename);  // command line option: -d
int           u_task(HANDLE* hComm, unsigned char i2c_addr, unsigned char* N_addr, unsigned int* N_bytes, unsigned char write_burst, char* filename); // command line option: -u
unsigned char i_task(HANDLE* hComm);                                                                                                                  // command line option: -i
int           m_task(HANDLE* hComm, unsigned char i2c_addr, unsigned char* N_addr, unsigned int* N_bytes);                                            // command line option: -m
unsigned char p_task(HANDLE* hComm);                                                                                                                  // command line option: -p
int           s_task(HANDLE* hComm);                                                                                                                  // command line option: -s
unsigned char r_task(HANDLE* hComm, unsigned char read_burst);                                                                                        // command line option: -r
unsigned char w_task(HANDLE* hComm, unsigned char write_burst);                                                                                       // command line option: -w

int init_serial_port(unsigned char n, HANDLE* hComport);

void read_from_eeprom(unsigned char i2c_addr, unsigned char addr, unsigned char* rxbuffer, int* read_N, HANDLE* hComm); // 1 byte addressing read command
void Read_from_eeprom(unsigned char i2c_addr, unsigned int addr, unsigned char* rxbuffer, int* read_N, HANDLE* hComm);  // 2 byte addressing Read command

void write_to_eeprom(unsigned char i2c_addr, unsigned char addr, unsigned char txbyte, unsigned char* rxbuffer, int* read_N, HANDLE* hComm); // 1 byte addressing write command with rx buffer for ACK/NACK return
void Write_to_eeprom(unsigned char i2c_addr, unsigned int addr, unsigned char txbyte, unsigned char* rxbuffer, int* read_N, HANDLE* hComm);  // 2 byte addressing write command with rx buffer for ACK/NACK return
void write_to_eeprom_burst(unsigned char i2c_addr, unsigned char addr, unsigned char* txbyte, unsigned char N_txbyte, unsigned char* rxbuffer, int* read_N, HANDLE* hComm); // 1 byte addressing burst write command with rx buffer for ACK/NACK return
void Write_to_eeprom_burst(unsigned char i2c_addr, unsigned int addr, unsigned char* txbyte, unsigned char N_txbyte, unsigned char* rxbuffer, int* read_N, HANDLE* hComm); // 2 byte addressing burst write command with rx buffer for ACK/NACK return

int TestIfSizeIs(unsigned int n, unsigned char i2c_addr, unsigned char addressing, HANDLE* hComm); // checks address overflow during write access on i2c device
int readOk(unsigned char* rxbuffer, int read_N);                                                   // parse rxbuffer for read ACK
int writeOk(unsigned char* rxbuffer, int read_N);                                                  // parse rxbuffer for write ACK

int getopt(int nargc, char * const nargv[], const char *ostr);                                     // windows clone getopt from unistd.h

int verbose_on;                                                                                    // enable/disable printf stdout
int opterr;		                                                                                    // if error message should be printed
int optind;		                                                                                    // index into parent argv vector
int optopt;		                                                                                    // character checked for validity
int optreset;                                                                                      // reset getopt
char *optarg;	                                                                                    // argument associated with option

void usage(void)
{
   printf("\nFMC FRU PROGRAMMER %d.%d.%d\n", REVISION_MAJOR, REVISION_MINOR, BUILD_NUMBER);
   printf(" Copyright (C) 2020 IAM Electronic GmbH <info@iamelectronic.com>\n"
          " This work is free. You can redistribute it and/or modify it under the\n"
          " terms of the Do What The Fuck You Want To Public License, Version 2,\n"
          " as published by Sam Hocevar. See http://www.wtfpl.net/ for more details.\n\n\n");
   printf(" File transfer (EEPROM binary images)\n"
          "    -d <filename.bin>\tdownload content from FMC FRU EEPROM and write to file)\n"
          "    -u <filename.bin>\tupload a file to FMC FRU EEPROM)\n\n");
   printf(" EEPROM read/write parameters\n"
          "    -a <1,2> set address width in bytes (1 or 2 bytes are supported)\n"
          "    -l <1024 .. 524288> set EEPROM size in bits (multiples of 1024 allowed)\n"
          "    -L  <128 ..  65536> set EEPROM size in Bytes (multiples of 128 allowed)\n"
          "    -r  <1, 8, 16, 24, .. 64> set read burst size in bytes (8 byte is default)\n"
          "    -w  <1, 8, 16, 32>        set write burst size in bytes (8 byte is default)\n\n");
   printf(" Miscellaneous functions\n"          
          "    -i\t\t\tScan I2C bus for EEPROM devices\n"
          "    -m\t\t\tMemory autodetect\n"
          "    -p\t\t\tScan Present pin of FMC module\n"
          "    -s\t\t\tScan serial ports for FMC FRU Programmer\n\n");
}

int main(int argc, char **argv)
{
   int  ret;                     // default return value
   unsigned char i2c_addr;       // default I2C EEPROM addr
   unsigned char N_addr;         // number of bytes used for addressing memory locations of EEPROM
   unsigned int  N_bytes;        // number of bytes to read from EEPROM
   unsigned char read_burst;     // number of bytes transferred during a read cycle
   unsigned char write_burst;    // number of bytes transferred during a write cycle
   char opt;                     // helper for parsing argument strings
   int opt_num;                  // helper for parsing numerical strings
   char *input_file = NULL;      // string ptr for input filename
   char *output_file = NULL;     // string ptr for output filename
   HANDLE hComm;                 // Serial port handle

   opterr = 1;
   optind = 1; 

   i2c_addr    = 0xFF;
   N_addr      = 0x00;
   N_bytes     = 0x00000000;
   read_burst  = 0x08;
   write_burst = 0x08;

   while ((opt = getopt (argc, argv, "a:l:L:r:w:d:u:imps?h")) != -1)
   {    
      switch (opt)
      {
         case 'a':
            opt_num = atoi(optarg);
            if (opt_num == 1)
               N_addr = 0x01;
            else if (opt_num == 2)
               N_addr = 0x02;
            else
               N_addr = 0x00;

            printf("\nSet EEPROM address width: %d byte\n",N_addr);

            break;

         case 'l':
            opt_num = atoi(optarg);
            if ((opt_num > 0) && (opt_num<=524288) && ((opt_num % 1024)==0))
               N_bytes = opt_num / 8;
            else
               N_bytes = 0;
            
            printf("\nSet EEPROM size: %d bytes (%d bits)\n",N_bytes,N_bytes*8);
            
            break;

         case 'L':
            opt_num = atoi(optarg);
            if ((opt_num > 0) && (opt_num<=65536) && ((opt_num % 128)==0))
               N_bytes = opt_num;
            else
               N_bytes = 0;            

            printf("\nSet EEPROM size: %d bytes (%d bits)\n",N_bytes,N_bytes*8);

            break;           

         case 'r':
            opt_num = atoi(optarg);
            if (((opt_num > 0) && (opt_num<=64) && ((opt_num % 8)==0)) || (opt_num==1)) // valid arguments: 1, 8, 16, 24, 32, 40, 48, 56, 64
               read_burst = opt_num;
            else
               read_burst = 0x08;

            verbose_on = 0;       // hide outputs from s_task
            ret = s_task(&hComm); // run serial port scan
            verbose_on = 1;       // show outputs from r_task            

            if (ret)
            {
               ret = r_task(&hComm,read_burst);
               if (!ret)
                  printf("\nCould not set read burst length!\n");           
            }
            else
            {
               printf("\nNo FMC FRU Programmer connected!\n");           
               break;
            }

            CloseHandle(hComm); // close serial port handle, not needed anymore
            break;

         case 'w':
            opt_num = atoi(optarg);
            if ((opt_num==1) || (opt_num==8) || (opt_num==16) || (opt_num==32)) // 1, 8, 16, 32
               write_burst = opt_num;
            else
               write_burst = 0x08;

            printf("\nSet write burst length: %d\n",write_burst);

            break;              

         case 'd':
            verbose_on = 0;               // hide outputs from s_task and i_task
            ret      = s_task(&hComm);    // run serial port scan
            if (ret)
               i2c_addr = i_task(&hComm); // run i2c scan
            else
            {
               printf("\nNo FMC FRU Programmer connected!\n");           
               break;
            }           

            if (i2c_addr!=0xFF)           // valid EEPROM i2c address found
            {
               verbose_on = 1;            // show outputs from d_task  
               d_task(&hComm, i2c_addr, &N_addr, &N_bytes, read_burst, optarg);
               verbose_on = 0;            // disable show outputs
            }                  
            else
               printf("\nNo I2C EEPROM found!\n");

            CloseHandle(hComm);           // close serial port handle, not needed anymore                  
            break;

         case 'u':
            verbose_on = 0;               // hide outputs from s_task and i_task
            ret      = s_task(&hComm);    // run serial port scan
            if (ret)
               i2c_addr = i_task(&hComm); // run i2c scan
            else
            {
               printf("\nNo FMC FRU Programmer connected!\n");           
               break;
            }

            if (i2c_addr!=0xFF)           // valid EEPROM i2c address found
            {
               verbose_on = 1;            // show outputs from u_task  
               u_task(&hComm, i2c_addr, &N_addr, &N_bytes, write_burst, optarg);
               verbose_on = 0;            // disable show outputs
            }                  
            else
               printf("\nNo I2C EEPROM found!\n");

            CloseHandle(hComm);           // close serial port handle, not needed anymore                  
            break; 

         case 'i':
            verbose_on = 0;               // hide outputs from s_task
            ret = s_task(&hComm);         // run serial port scan
            verbose_on = 1;               // show outputs from i_task  
            
            if (ret)
               i_task(&hComm);            // run i2c scan
            else
            {
               printf("\nNo FMC FRU Programmer connected!\n");           
               break;
            }
            CloseHandle(hComm);           // close serial port handle, not needed anymore            
            break;  

         case 'm':
            verbose_on = 0;               // hide outputs from tasks
            ret = s_task(&hComm);         // run serial port scan
            if (ret)
               i2c_addr = i_task(&hComm); // run i2c scan
            else
            {
               printf("\nNo FMC FRU Programmer connected!\n");
               break;
            }

            if (i2c_addr!=0xFF)                                   // valid EEPROM i2c address found
            {
               verbose_on = 1;                                    // show outputs from m_task
               ret = m_task(&hComm, i2c_addr, &N_addr, &N_bytes); // run automatic memory detect
               verbose_on = 0;                                    // hide outputs               
               if (ret)
               {
                  if (N_addr==0)
                     printf("\nMemory autodetection failed!\nPlease specify number of bytes for addressing EEPROM by using -a option.\n");
               }
               else
               {                     
                     printf("\nMemory autodetection failed!\nPlease check the write protection of the EEPROM.\n");
               }
            }
            else
               printf("\nNo I2C EEPROM found!\n");
            CloseHandle(hComm);   // close serial port handle, not needed anymore             
            break;

         case 'p':
            verbose_on = 0;       // hide outputs from s_task
            ret = s_task(&hComm); // run serial port scan
            verbose_on = 1;       // show outputs from p_task  
            if (ret)
            {
               if(p_task(&hComm)) // run present pin scan
                  printf("\nFMC module is present (pin H2 PRSNT_M2C_L is LOW)\n");
               else
                  printf("\nFMC module is not attached (pin H2 PRSNT_M2C_L is HIGH)\n");
            }               
            else
            {
               printf("\nNo FMC FRU Programmer connected!\n");           
               break;
            }
            CloseHandle(hComm);   // close serial port handle, not needed anymore            
            break;

         case 's':
            verbose_on = 1;
            s_task(&hComm);       // run a serial port scan
            CloseHandle(hComm);   // close serial port handle, not needed anymore
            break;
         case '?':            
         case 'h':            
            usage();
            switch (optopt)
            {
               case 'a': printf("\n\nExample usage:\nfmc_fru_programmer.exe -a 1\n"); break;
               case 'l': printf("\n\nExample usage:\nfmc_fru_programmer.exe -l 2048\n"); break;
               case 'L': printf("\n\nExample usage:\nfmc_fru_programmer.exe -L 256\n"); break;
               case 'r': printf("\n\nExample usage:\nfmc_fru_programmer.exe -r 8\n"); break;
               case 'w': printf("\n\nExample usage:\nfmc_fru_programmer.exe -w 8\n"); break;
               case 'd': printf("\n\nExample usage:\nfmc_fru_programmer.exe -d file_to_upload.bin\n"); break;
               case 'u': printf("\n\nExample usage:\nfmc_fru_programmer.exe -u filename_for_download.bin\n"); break;
            }
            return 1;
            break;
         default:
            printf("Unknown option: %c\n\n", opt);
            usage();
            return 1;
      }
   }
    
   if (argc==1)
      usage();

   return 0;
}

/*
 * -d option
 * Download content from EEPROM
 */
int d_task(HANDLE* hComm, unsigned char i2c_addr, unsigned char* N_addr, unsigned int* N_bytes, unsigned char read_burst, char* filename)
{  
   unsigned char rxbuffer[RX_BUFFER_SIZE]; // buffer for readout
   int           read_N;                   // number of bytes in readout buffer

   FILE*         fp;                       // file pointer to outpput file  

   if (*N_addr==0x00) // addressin width is not valid
   {
      *N_addr = ((i2c_addr & 0x04) >> 2) + 1; // is 2 when bit 2 from i2c_addr[7..0] is set, is 1 when bit 2 from i2c_addr[7..0] is not set
      if (verbose_on)
         printf("\nAddress width not set, using value %d (determined by I2C addr:0x%02X)\n",*N_addr,i2c_addr);
   }
   
   if (*N_bytes==0x00000000)
   {
      if (*N_addr==1)
         *N_bytes = 256;  // recommendation 5.7-2 in ANSI VITA 57.1 (Mezzanine cards should provide only one EEPROM, either 2Kb or 32Kb)
      else if (*N_addr==2)
         *N_bytes = 4096; // recommendation 5.7-2 in ANSI VITA 57.1 (Mezzanine cards should provide only one EEPROM, either 2Kb or 32Kb)                     
      else
         *N_bytes = 0;    //
      if (verbose_on)
         printf("\nNumber of bytes not set, using default value: %d\n",*N_bytes);
   }

   printf("\nDownloading %d bytes (burst length: %d) to file %s\n",*N_bytes, read_burst, filename);

   fp = fopen(filename, "wb");
   for(unsigned int addr=0; addr < *N_bytes; addr=addr+read_burst)//addr++)
   {       

     if (*N_addr == 2)
        Read_from_eeprom(i2c_addr, addr, rxbuffer, &read_N, hComm);             // 2 byte addressing Read command
     else if (*N_addr == 1)
        read_from_eeprom(i2c_addr, addr, rxbuffer, &read_N, hComm);             // 2 byte addressing Read command
     else
     {        
        printf("\nError during readout, address width (%d) is not valid\n",*N_addr);
        break;
     }

     if (readOk(rxbuffer, read_N))   // check return values for ACK
     {
         fwrite(rxbuffer+1,1,read_burst,fp); // write bytes in file, skip first position (its the ACK)
         fflush(fp);
         printf("%3.1f%%\r", (float)(addr+read_burst) / (float)(*N_bytes) * 100.0);
         fflush(stdout);
     }     
     else
     {        
        printf("\nError during readout, EEPROM returns no ACK on Read command!\n");
        break;
     }        
   }
   fclose(fp);
   return 1;
}

/*
 * -u option
 * Upload content from EEPROM
 */
int u_task(HANDLE* hComm, unsigned char i2c_addr, unsigned char* N_addr, unsigned int* N_bytes, unsigned char write_burst, char* filename)
{
   unsigned char readchar;                    // read one byte from file
   unsigned char writebuffer[TX_BUFFER_SIZE]; // writebuffer for EEPROM   
   char rxbuffer[RX_BUFFER_SIZE];             // receive buffer for ACK
   int read_N;                                // received bytes
   int c;
   int i;
   
   unsigned int mem_addr;                     // mem addr counter for EEPROM content
   FILE* fp;                                  // file pointer to outpput file  
   unsigned int filesize;                     // filesize in bytes
   unsigned int cnt_download;                 // byte counter for download
   float n_log2;
   fp = fopen(filename, "rb");
   if (fp == NULL)
   {
      fclose(fp);
      printf("\nCannot open file %s\n",filename);
      return 0;
   }
   else
   {
      fseek(fp, 0, SEEK_END); // seek to end of file
      filesize = ftell(fp);   // get filesize
      fseek(fp, 0, SEEK_SET); // seek back to beginning of file    

      i        = 1;
      mem_addr = 0;
      n_log2   = log((double)filesize) / log(2.0);
      *N_bytes = (unsigned int)pow(2,ceil(n_log2));                // set N_bytes to next power of 2

      if (*N_addr==0x00) // addressin width is not valid
      {
         *N_addr = ((i2c_addr & 0x04) >> 2) + 1; // is 2 when bit 2 from i2c_addr[7..0] is set, is 1 when bit 2 from i2c_addr[7..0] is not set
         if (verbose_on)
            printf("\nAddress width not set, using value %d (determined by I2C addr:0x%02X)\n",*N_addr,i2c_addr);
      }

      printf("\nUploading file %s (%d bytes)\n",filename,filesize);
      cnt_download = 0;
      while(c = fgetc(fp), c != EOF)
      {
         readchar     = (unsigned char)c;
         cnt_download = cnt_download + 1;
         writebuffer[i-1] = readchar;
         printf("%3.1f%%\r", (float)(cnt_download) / (float)(filesize) * 100.0);
         fflush(stdout);
         //printf("0x%02X ",writebuffer[i-1]);  fflush(stdout);      
         if ((i % write_burst)==0)
         {            
            //printf(" (Addr: 0x%04X)\n",mem_addr-write_burst+1);  fflush(stdout);        
            i = 1;
            if (*N_addr == 2)
            {
               Write_to_eeprom_burst(i2c_addr, mem_addr-write_burst+1, writebuffer, write_burst, rxbuffer, &read_N, hComm); // bust write with 2 byte addressing
            }
            else if (*N_addr == 1)
               write_to_eeprom_burst(i2c_addr, mem_addr-write_burst+1, writebuffer, write_burst, rxbuffer, &read_N, hComm); // bust write with 1 byte addressing
            else
            {        
               printf("\nError during readout, address width (%d) is not valid\n",*N_addr);
               break;
            }            
         }
         else
         {
            if (mem_addr >= (filesize - (filesize % write_burst))) // too few remaining bytes for burst write
            {
               if (*N_addr == 2)
               {
                  Write_to_eeprom(i2c_addr, mem_addr, readchar, rxbuffer, &read_N, hComm); // bust write with 2 byte addressing
               }
               else if (*N_addr == 1)
                  write_to_eeprom(i2c_addr, mem_addr, readchar, rxbuffer, &read_N, hComm); // bust write with 1 byte addressing
               else
               {        
                  printf("\nError during readout, address width (%d) is not valid\n",*N_addr);
                  break;
               }
               //printf("mem_addr: 0x%02X (char: 0x%02X)\n",mem_addr, readchar);
               fflush(stdout);
            }
            i = i + 1;
         }
         if (ferror(fp))
         {
            /* error handling here */
             printf("\nError while reading file %s\n",filename);
         }
         mem_addr = mem_addr + 1;
      }
      
      fclose(fp);
      return 1;
   }   
}

/*
 * -m option
 * I2C Memory Autodetect, see AN690 from Microchip for details
 * this function determines addressing scheme and memory size of FMC FRU EEPROM
 */
int m_task(HANDLE* hComm, unsigned char i2c_addr, unsigned char* N_addr, unsigned int* N_bytes)
{
   int ret = 0;                        // default return value
   int state = 1;                      // states of algorithm
   int debug = 0;                      // output debug information   
   char rxbuffer[RX_BUFFER_SIZE];      // receive buffer
   int  read_N;                        // number of valid bytes in rx buffer

   unsigned char read_value_1byteaddr_x00; // read value from addr 0x00 with 1 byte addressing
   unsigned char read_value_1byteaddr_x01; // read value from addr 0x01 with 1 byte addressing
   unsigned char read_value_2byteaddr;     // read value from addr 0x00 with 2 byte addressing

   int MODEL;                          // used to determine EEPROM Model 24Cxx
   int L;                              // used to determine Length of EEPROM in Bytes

   unsigned char N_addr_default;
   N_addr_default = ((i2c_addr & 0x04) >> 2) + 1; // is 2 when bit 2 from i2c_addr[7..0] is set, is 1 when bit 2 from i2c_addr[7..0] is not set

   if (state == 1)
   {
      // (1.A) READ FROM ADDR 0x00 WITH 1 BYTE ADDRESSING
      read_from_eeprom(i2c_addr, 0x00, rxbuffer, &read_N, hComm);             // must return 0x06 (0x06 is ACK) in rxbuffer[0]
      if (readOk(rxbuffer, read_N))                                          // check return values for ACK
      {
         read_value_1byteaddr_x00 = rxbuffer[1];
         state = 2;                                // goto next state
         if (debug)
            printf("read: 0x%02X\n", read_value_1byteaddr_x00);
      }
      // (1.B) READ FROM ADDR 0x01 WITH 1 BYTE ADDRESSING
      read_from_eeprom(i2c_addr, 0x01, rxbuffer, &read_N, hComm);             // must return 0x06 (0x06 is ACK) in rxbuffer[0]
      if (readOk(rxbuffer, read_N))                                          // check return values for ACK
      {
         read_value_1byteaddr_x01 = rxbuffer[1];
         state = 2;                                // goto next state
         if (debug)
            printf("read: 0x%02X\n", read_value_1byteaddr_x01);
      }
   }

   if (state == 2)
   {
      // (2) READ FROM ADDR 0x0000 WITH 2 BYTE ADDRESSING
      Read_from_eeprom(i2c_addr, (unsigned int)0x0000, rxbuffer, &read_N, hComm); // must return 0x06 (0x06 is ACK) in rxbuffer[0]
      if (readOk(rxbuffer, read_N))                                              // check return values for ACK
      {
         read_value_2byteaddr = rxbuffer[1];
         state = 3;                                // goto next state
         if (debug)
            printf("Read: 0x%02X\n", read_value_2byteaddr);
      }
   }

   if (state == 3)
   {
      // (3) WRITE VALUE 0x01 TO ADDR 0x0000 WITH 2 BYTE ADDRESSING
      Write_to_eeprom(i2c_addr, (unsigned int)0x0000, 0x01, rxbuffer, &read_N, hComm); // must return 0x06 (0x06 is ACK) in rxbuffer[0]
      if (writeOk(rxbuffer, read_N))                                                   // check return values for ACK
      {
         state = 4;                                // goto next state
         if (debug)
            printf("Write ok (ACK: 0x%02X)\n",rxbuffer[0]);
      }
   }

   if (state == 4)
   {
      // (4) READ FROM ADDR 0x0000 WITH 2 BYTE ADDRESSING
      Read_from_eeprom(i2c_addr, (unsigned int)0x0000, rxbuffer, &read_N, hComm); // must return 0x06 (0x06 is ACK) in rxbuffer[0]
      if (readOk(rxbuffer, read_N))                                              // check return values for ACK
      {         
         state = 5;                                // goto next state
         if (debug)
            printf("Read: 0x%02X\n", rxbuffer[1]); 
      }
   }

   if (state == 5)
   {
      if (rxbuffer[1] == 0x01)
      {
         if (debug)
            printf("2 byte adressing\n", rxbuffer[1]);
         // (5) WRITE BACK INITAL VALUE TO ADDR 0x0000 WITH 2 BYTE ADDRESSING
         Write_to_eeprom(i2c_addr, (unsigned int)0x0000, read_value_2byteaddr, rxbuffer, &read_N, hComm); // must return 0x06 (0x06 is ACK) in rxbuffer[0]
         if (writeOk(rxbuffer, read_N))                                                                  // check return values for ACK
         {
            *N_addr = 2;                              // 2 byte addressing
            state   = 6;                              // goto next state
            if (debug)
               printf("Writeback ok (ACK: 0x%02X)\n",rxbuffer[0]);
         }
      }
      else
      {
         if (debug)
            printf("1 byte adressing\n", rxbuffer[1]);
         // (5.A) WRITE BACK INITAL VALUE TO ADDR 0x00 WITH 1 BYTE ADDRESSING
         write_to_eeprom(i2c_addr, 0x00, read_value_1byteaddr_x00, rxbuffer, &read_N, hComm); // must return 0x06 (0x06 is ACK) in rxbuffer[0]
         if (writeOk(rxbuffer, read_N))                                                      // check return values for ACK
         {
            // (5.B) WRITE BACK INITAL VALUE TO ADDR 0x01 WITH 1 BYTE ADDRESSING
            write_to_eeprom(i2c_addr, 0x01, read_value_1byteaddr_x01, rxbuffer, &read_N, hComm); // must return 0x06 (0x06 is ACK) in rxbuffer[0]
            if (writeOk(rxbuffer, read_N))                                                      // check return values for ACK
            {
               *N_addr = 1;                           // 1 byte addressing
               state   = 6;                           // goto next state
               if (debug)
                  printf("Writeback ok (ACK: 0x%02X)\n",rxbuffer[0]);
            }
         }
      }
   }

   if (state == 6)
   {
      L     = 128;
      MODEL = 1;
      printf("\nRunning memory scan"); fflush(stdout);
      do
      {
         if (verbose_on)
         {
            printf("."); fflush(stdout);
         }

         if (TestIfSizeIs(L, i2c_addr, *N_addr, hComm))
         {            
            state = 7;    // success, state 7 is last state
            *N_bytes = L;
            break;
         }
         else
         {
            L     = L * 2;
            MODEL = MODEL * 2;
         }
      } while (L<=65536);
   }
   
   switch (state)
   {
      case 1: if (debug)
                 printf("Error during I2C memory autodetect.\nNo data was corrupted!\n");
              *N_addr = N_addr_default;
              ret     = 0;                    
              break;
      case 2: if (debug)
                 printf("Error during I2C memory autodetect.\nNo data was corrupted!\n");
              *N_addr = N_addr_default;
              ret     = 0;                    
              break;
      case 3: if (debug)
                 printf("Error during I2C memory autodetect.\nNo data was corrupted!\n");
              *N_addr = N_addr_default;
              ret     = 0;                    
              break;
      case 4: if (debug)
                 printf("Error during I2C memory autodetect.\nAddr location 0x00 maybe corrupted!\n");
               *N_addr = N_addr_default;
               ret     = 0;                    
              break; 
      case 5: if (debug)
                 printf("Error during I2C memory autodetect.\nAddr location 0x00 maybe corrupted!\n");
              *N_addr = N_addr_default;
              ret     = 0;
              break;
      case 6: if (debug)
                 printf("Error during I2C memory autodetect.\nAddr location 0x00 maybe corrupted!\n");              
              *N_addr = N_addr_default;
              ret     = 0;
              break;
      case 7: if (debug)
                 printf("I2C memory autodetect successfully finished!\n");                            
              ret     = 1;
              if (verbose_on)
              {
                 printf("\n");
                 printf("\nMemory information:\n");
                 printf("   Address bytes:\t%d\n",*N_addr);
                 printf("   N bytes      :\t%d\n",L);
                 printf("   MODEL No     :\t%02d\n",MODEL);
              }
              break;                                           
   }
   return ret;
}

/*
 * -i option
 *  Scan I2C bus for EEPROM
 *  the task returns a valid I2C address (only 1 EEPROM on bus is supported)
 */
unsigned char i_task(HANDLE* hComm)
{
   int ret = 0;                     // default return value
   char txbuffer[TX_BUFFER_SIZE];   // transmit buffer
   char rxbuffer[RX_BUFFER_SIZE];   // receive buffer
   int  read_N;                     // number of valid bytes in rx buffer
   int  write_N;                    // number of valid bytes in tx buffer
   txbuffer[0] = 's';               // send 'Scan' command
   WriteFile(*hComm, txbuffer, 1, &write_N, NULL);
   ReadFile(*hComm, rxbuffer, RX_BUFFER_SIZE, &read_N, NULL);                // must return 0xNN 0xNN 0xNN 0xFF (0xNN are addresses)
   if ((read_N>1) && ((unsigned char)rxbuffer[read_N-1]==(unsigned char)0xFF)) // at least one address was detected
   {
      if (verbose_on)
      {
         printf("\n");
         printf("Found EEPROM on I2C bus:\n");
         printf("   I2C address (7 bit):\t 0x%02X\n",rxbuffer[read_N-2]);
      }
      return rxbuffer[read_N-2];
   }
   else
   {
      if (verbose_on)
         printf("\nNo I2C EEPROM found!\n");
      return 0xFF;
   }
}

/*
 * -p option
 *  Scan Present pin of FMC Moduke
 *  the task returns 1 if FMC module is attached
 */
unsigned char p_task(HANDLE* hComm)
{
   int ret = 0;                     // default return value
   char txbuffer[TX_BUFFER_SIZE];   // transmit buffer
   char rxbuffer[RX_BUFFER_SIZE];   // receive buffer
   int  read_N;                     // number of valid bytes in rx buffer
   int  write_N;                    // number of valid bytes in tx buffer
   txbuffer[0] = 'p';               // send 'p' command for reading present pin
   WriteFile(*hComm, txbuffer, 1, &write_N, NULL);
   ReadFile(*hComm, rxbuffer, RX_BUFFER_SIZE, &read_N, NULL); // one byte must be returned
   if (read_N==1)                                               // one byte must be returned
   {
      return rxbuffer[0];
   }
   else
   {
      if (verbose_on)
         printf("\nError while reading present pin!\n");
      return 0;
   }
}

/*
 * -s option
 *  Scan serial ports for FMC FRU Programmer
 *  the task returns a valid handle for the serial port
 */
int s_task(HANDLE* hComm)
{
   int ret = 0;    // default return value
   // FOR LOOP TO SCAN ALL WINDOWS COM PORTS
   for (unsigned char n=1; n <= WIN_COM_PORT_MAX_NO; n++)
   {
      ret = init_serial_port(n, hComm);
      if (ret)     // SUCCESS
         break;    // exit for loop

      if (n==WIN_COM_PORT_MAX_NO)                   
         break;    // end of scan
   }

   if (!ret)
   {
      // NO SUCCESS
      if (verbose_on)
         printf("\nNo FMC FRU Programmer connected!\n");

      CloseHandle(*hComm);
      return 0;
   }
   else
   {
      // COM PORT FOUND AND INITIALIZED
      return 1;
   }   
}

/*
 * -r option
 *  set read burst size for I2C bus
 */
unsigned char r_task(HANDLE* hComm, unsigned char read_burst)
{
   char rxbuffer[RX_BUFFER_SIZE];                              // receive buffer for b command
   char txbuffer[TX_BUFFER_SIZE];                              // transmit buffer for b command
   int  read_N;                                                // number of valid bytes in rx buffer  
   int  write_N;                                               // number of valid bytes in tx buffer
   txbuffer[0] = 'b';                                          // send  0x62 b = bytes to read in a burst command
   txbuffer[1] = read_burst;                                   // append read burst size
   WriteFile(*hComm, txbuffer, 2, &write_N, NULL);             // execute command on I2C bus
   ReadFile(*hComm, rxbuffer, RX_BUFFER_SIZE, &read_N, NULL);  // must return 0x06 (0x06 is ACK)
   if (readOk(rxbuffer,read_N))                                // check
   {
      if (verbose_on)
      {
         printf("\nSet read burst length: %d\n",read_burst);
      }
      return read_burst;
   }
   else
      return 0;
}

/*
 *  Initialize the serial port
 *  Default parameters are 115200 Baud, 8 bit data, 1 Startbit, 1 Stopbit, No parity
 */
int init_serial_port(unsigned char n, HANDLE* hComport)
{
   HANDLE hTest;
   char portname[16];                                      // stores string "COMx"
   DCB dcbSerialParams       = { 0 };                      // Initializing DCB structure
   COMMTIMEOUTS timeouts     = { 0 };                      // Initializing Timeout structure
   BOOL Status;                                            // Status of the various operations 
   char txbuffer[TX_BUFFER_SIZE];                          // transmit buffer
   char rxbuffer[RX_BUFFER_SIZE];                          // receive buffer
   int  read_N;                                            // number of valid bytes in rx buffer      
   int  write_N;                                           // number of valid bytes in tx buffer      

   if (n<10)
      snprintf(portname,3+1+1,"COM%d",n);                  // COM0 .. COM9
   else if (n < 100)
      snprintf(portname,4+3+2+1,"\\\\.\\COM%d",n);         // \\.\COM10 .. \\.\COM99
   else
      snprintf(portname,4+3+3+1,"\\\\.\\COM%d",n);         // \\.\COM100 .. \\.\COM255
      
   hTest     = CreateFile(portname,                        // port name
                      GENERIC_READ | GENERIC_WRITE,        // Read/Write
                      0,                                   // No Sharing
                      NULL,                                // No Security
                      OPEN_EXISTING,                       // Open existing port only
                      0,                                   // Non Overlapped I/O
                      NULL);                               // Null for Comm Devices

   if (hTest == INVALID_HANDLE_VALUE)
   {   
       // NO SUCCESS  
       CloseHandle(hTest); // Closing the Serial Port
   }
   else
   {    
      // SUCCESS, portname exists
      dcbSerialParams.DCBlength = sizeof(dcbSerialParams);  
      GetCommState(hTest, &dcbSerialParams);
      dcbSerialParams.BaudRate             = CBR_115200;         // Setting BaudRate = 115200
      dcbSerialParams.ByteSize             = 8;                  // Setting ByteSize = 8
      dcbSerialParams.StopBits             = ONESTOPBIT;         // Setting StopBits = 1
      dcbSerialParams.Parity               = NOPARITY;           // Setting Parity = None
      //dcbSerialParams.fRtsControl          = RTS_CONTROL_ENABLE; // Enables the RTS line when the device is opened and leaves it on
      //dcbSerialParams.fDtrControl          = DTR_CONTROL_ENABLE; // Enables the DTR line when the device is opened and leaves it on
      SetCommState(hTest, &dcbSerialParams);                     // Configuring the port according to settings in DCB 
      
      timeouts.ReadIntervalTimeout         = 80;          // in milliseconds
      timeouts.ReadTotalTimeoutConstant    = 10;          // in milliseconds
      timeouts.ReadTotalTimeoutMultiplier  = 50;          // in milliseconds
      timeouts.WriteTotalTimeoutConstant   = 10;          // in milliseconds
      timeouts.WriteTotalTimeoutMultiplier = 10;          // in milliseconds
      SetCommTimeouts(hTest, &timeouts);                  // Configuring the timeouts

      txbuffer[0] = 'v';                                  // send 'Version' command
      WriteFile(hTest, txbuffer, 1, &write_N, NULL);
      ReadFile(hTest, rxbuffer, RX_BUFFER_SIZE, &read_N, NULL); // must return 0xNN 0xNN 0xNN 0xFF (0xNN are version numbers)   
      if ((read_N == 4) && ((unsigned char)rxbuffer[read_N-1]==(unsigned char)0xFF))
      {         
         if (verbose_on)
         {
            printf("\n");
            printf("Found FMC FRU PROGRAMMER:\n");
            printf("   Serial port:\t\t%s\n",portname);
            printf("   Firmware version:\t%02X.%02X.%02X\n",rxbuffer[0],rxbuffer[1],rxbuffer[2]);
         }
         *hComport = hTest;                               // copy serial port handle
         return 1;                                        // return success
      } 
      CloseHandle(hTest);                                 // Closing the serial port, no FMC FRU Programmer detected
   }
   return 0;
}

/*
 *  read one byte from FMC FRU Programmer
 *  Using 1 byte addresses
 *  On successful read, rx buffer contains 0x06 (ACK) and data
 */
void read_from_eeprom(unsigned char i2c_addr, unsigned char addr, unsigned char* rxbuffer, int* read_N, HANDLE* hComm)
{
   char txbuffer[TX_BUFFER_SIZE];                               // transmit buffer for read command
   txbuffer[0] = 'r';                                           // send 'read (1 byte addressing)' command
   txbuffer[1] = i2c_addr;                                      // append i2s address of eeprom
   txbuffer[2] = addr;                                          // append addr to read
   WriteFile(*hComm, txbuffer, 3, read_N, NULL);                  // execute command on I2C bus
   ReadFile(*hComm, rxbuffer, RX_BUFFER_SIZE, read_N, NULL);   // must return 0x06 0xNN (0x06 is ACK 0xNN is data)
}

/*
 *  Read one byte from FMC FRU Programmer
 *  Using 2 byte addresses
 *  On successful read, rx buffer contains 0x06 (ACK) and data
 */
void Read_from_eeprom(unsigned char i2c_addr, unsigned int addr, unsigned char* rxbuffer, int* read_N, HANDLE* hComm)
{
   char txbuffer[TX_BUFFER_SIZE];                              // transmit buffer for read command
   txbuffer[0] = 'R';                                          // send 'Read (2 byte addressing)' command
   txbuffer[1] = i2c_addr;                                     // append i2c address of eeprom
   txbuffer[2] = (unsigned char)0x000000FF & (addr >> 8);      // append addr (MSB) to read (0x0000)
   txbuffer[3] = (unsigned char)0x000000FF & (addr >> 0);      // append addr (LSB) to read (0x0000)
   WriteFile(*hComm, txbuffer, 4, read_N, NULL);                 // execute command on I2C bus   
   ReadFile(*hComm, rxbuffer, RX_BUFFER_SIZE, read_N, NULL);  // must return 0x06 0xNN (0x06 is ACK 0xNN is data)
}

/*
 *  write one byte to FMC FRU Programmer
 *  Using 1 byte addresses
 *  On successful write, rx buffer contains 0x06 (ACK)
 */
void write_to_eeprom(unsigned char i2c_addr, unsigned char addr, unsigned char txbyte, unsigned char* rxbuffer, int* read_N, HANDLE* hComm)
{
   char txbuffer[TX_BUFFER_SIZE];                              // transmit buffer for read command
   txbuffer[0] = 'w';                                          // send 'write (1 byte addressing)' command
   txbuffer[1] = i2c_addr;                                     // append i2c address of eeprom
   txbuffer[2] = addr;                                         // append addr to write (0x00)
   txbuffer[3] = txbyte;                                       // append value to write
   WriteFile(*hComm, txbuffer, 4, read_N, NULL);                 // execute command on I2C bus
   ReadFile(*hComm, rxbuffer, 1, read_N, NULL);                // must return 0x06 (0x06 is ACK)
}

/*
 *  Write one byte to FMC FRU Programmer
 *  Using 2 byte addresses
 *  On successful write, rx buffer contains 0x06 (ACK)
 */
void Write_to_eeprom(unsigned char i2c_addr, unsigned int addr, unsigned char txbyte, unsigned char* rxbuffer, int* read_N, HANDLE* hComm)
{
   char txbuffer[TX_BUFFER_SIZE];                              // transmit buffer for read command
   txbuffer[0] = 'W';                                          // send 'Write (2 byte addressing)' command
   txbuffer[1] = i2c_addr;                                     // append i2c address of eeprom
   txbuffer[2] = (unsigned char)0x000000FF & (addr >> 8);      // append addr (MSB) to read (0x0000)
   txbuffer[3] = (unsigned char)0x000000FF & (addr >> 0);      // append addr (LSB) to read (0x0000)
   txbuffer[4] = txbyte;                                       // append value to write
   WriteFile(*hComm, txbuffer, 5, read_N, NULL);                 // execute command on I2C bus
   ReadFile(*hComm, rxbuffer, 1, read_N, NULL);                // must return 0x06 (0x06 is ACK)
}

/*
 *  Write N byte to FMC FRU Programmer
 *  Using 1 byte addresses
 *  On successful write, rx buffer contains 0x06 (ACK)
 */
void write_to_eeprom_burst(unsigned char i2c_addr, unsigned char addr, unsigned char* txbyte, unsigned char N_txbyte, unsigned char* rxbuffer, int* read_N, HANDLE* hComm)
{
   char txbuffer[TX_BUFFER_SIZE];                              // transmit buffer for read command
   txbuffer[0] = 'w';                                          // send 'write (1 byte addressing)' command
   txbuffer[1] = i2c_addr;                                     // append i2c address of eeprom
   txbuffer[2] = addr;                                         // append addr to write
   for (int i=0; i<N_txbyte; i++)
   {
      txbuffer[3+i] = txbyte[i];                               // append value to write
   }
   WriteFile(*hComm, txbuffer, 3+N_txbyte, read_N, NULL);        // execute command on I2C bus
   Sleep(5);
   ReadFile(*hComm, rxbuffer, 1, read_N, NULL);                // must return 0x06 (0x06 is ACK)
}

/*
 *  Write N byte to FMC FRU Programmer
 *  Using 2 byte addresses
 *  On successful write, rx buffer contains 0x06 (ACK)
 */
void Write_to_eeprom_burst(unsigned char i2c_addr, unsigned int addr, unsigned char* txbyte, unsigned char N_txbyte, unsigned char* rxbuffer, int* read_N, HANDLE* hComm)
{
   char txbuffer[TX_BUFFER_SIZE];                              // transmit buffer for Write command
   txbuffer[0] = 'W';                                          // send 'Write (2 byte addressing)' command
   txbuffer[1] = i2c_addr;                                     // append i2c address of eeprom
   txbuffer[2] = (unsigned char)0x000000FF & (addr >> 8);      // append addr (MSB) to Write (0x0000)
   txbuffer[3] = (unsigned char)0x000000FF & (addr >> 0);      // append addr (LSB) to Write (0x0000)   
   for (int i=0; i<N_txbyte; i++)
   {
      txbuffer[4+i] = txbyte[i];                               // append value to write
   }

   WriteFile(*hComm, txbuffer, 4+N_txbyte, read_N, NULL);        // execute command on I2C bus
   if     (N_txbyte>=32)
      Sleep(20);
   else if(N_txbyte>=16)
      Sleep(10);
   else if(N_txbyte>=8)
      Sleep(5);
   else
      Sleep(1);
   ReadFile(*hComm, rxbuffer, 1, read_N, NULL);                // must return 0x06 (0x06 is ACK)
}

/*
 * EXAMPLE 1 FROM MICROCHIP AN690 I2C MEMORY AUTODETECT
 * n          = Address to test with Algorithm
 * addressing = Number of bytes for addressing (1 or 2)
 * hComport   = Handle for Comport
 */
int TestIfSizeIs(unsigned int n, unsigned char i2c_addr, unsigned char addressing, HANDLE* hComm)
{
   unsigned char TEMP0;           // data value in EEPROM ADDR 0x00   
   unsigned char TEMPN;           // data value in EEPROM ADDR 0x00   
   char rxbuffer[RX_BUFFER_SIZE]; // receive buffer
   int  read_N;                   // number of valid bytes in rx buffer   
   if (addressing == 0x01)
   {
      read_from_eeprom(i2c_addr, (unsigned char)0x00, rxbuffer, &read_N, hComm); // must return 0x06 (0x06 is ACK) in rxbuffer[0]           
      if (readOk(rxbuffer, read_N))      // check return values for ACK        
         TEMP0 = (unsigned char)rxbuffer[1];
      else
         return 0;
                  
      read_from_eeprom(i2c_addr, (unsigned char)n, rxbuffer, &read_N, hComm);    // must return 0x06 (0x06 is ACK) in rxbuffer[0]           
      if (readOk(rxbuffer, read_N))                                             // check return values for ACK        
         TEMPN = (unsigned char)rxbuffer[1];
      else
         return 0;

      if (TEMPN == TEMP0) // read from different location has same result, check if memory location overflow occurs
      {
         // write new value to mem lcoation 0x00
         write_to_eeprom(i2c_addr, (unsigned char)0x00, TEMP0+1, rxbuffer, &read_N, hComm); // must return 0x06 (0x06 is ACK) in rxbuffer[0]
         if (writeOk(rxbuffer, read_N))                                                     // check return values for ACK
         {
            // read from max. memory location to check overflow
            read_from_eeprom(i2c_addr, (unsigned int)n, rxbuffer, &read_N, hComm);      // must return 0x06 (0x06 is ACK) in rxbuffer[0]           
            if (readOk(rxbuffer, read_N))                                              // check return values for ACK
            {         
               TEMPN = (unsigned char)rxbuffer[1];
               // writeback to mem location 0x00
               write_to_eeprom(i2c_addr, (unsigned char)0x00, TEMP0, rxbuffer, &read_N, hComm); // must return 0x06 (0x06 is ACK) in rxbuffer[0]
               if (writeOk(rxbuffer, read_N))                                                   // check return values for ACK
               {
                  if ((unsigned char)TEMPN == (unsigned char)(TEMP0+1))
                     return 1; // overflow on memory address occurs
                  else
                     return 0;
               }
            }
         }
         return 0;
      }
      else
      {
         return 0;
      }
   }
   else if (addressing == 0x02)
   {
      Read_from_eeprom(i2c_addr, (unsigned int)0x0000, rxbuffer, &read_N, hComm); // must return 0x06 (0x06 is ACK) in rxbuffer[0]           
      if (readOk(rxbuffer, read_N))                                              // check return values for ACK
         TEMP0 = (unsigned char)rxbuffer[1];
      else
         return 0;

      Read_from_eeprom(i2c_addr, (unsigned int)(n & 0x0000FFFF), rxbuffer, &read_N, hComm);      // must return 0x06 (0x06 is ACK) in rxbuffer[0]           
      if (readOk(rxbuffer, read_N))                                                             // check return values for ACK             
         TEMPN = (unsigned char)rxbuffer[1];
      else
         return 0;

      if (TEMPN == TEMP0)
      {
         Write_to_eeprom(i2c_addr, (unsigned int)0x0000, (unsigned char)TEMP0+1, rxbuffer, &read_N, hComm); // must return 0x06 (0x06 is ACK) in rxbuffer[0]
         if (writeOk(rxbuffer, read_N))                                                                    // check return values for ACK
         {
            Read_from_eeprom(i2c_addr, (unsigned int)(n & 0x0000FFFF), rxbuffer, &read_N, hComm);      // must return 0x06 (0x06 is ACK) in rxbuffer[0]           
            if (readOk(rxbuffer, read_N))                                                             // check return values for ACK
            {         
               TEMPN = (unsigned char)rxbuffer[1];
               // writeback to mem location 0x0000 
               Write_to_eeprom(i2c_addr, (unsigned int)0x0000, (unsigned char)TEMP0, rxbuffer, &read_N, hComm); // must return 0x06 (0x06 is ACK) in rxbuffer[0]
               if (writeOk(rxbuffer, read_N))                                                                  // check return values for ACK            
               {
                  if ((unsigned char)TEMPN == (unsigned char)(TEMP0+1))
                     return 1;
                  else
                     return 0;
               }                                 
            }
         }
         return 0;
      }
      else
      {
         return 0;
      }
   }
   else
   {
      return 0;
   }
}

/*
 * parse rxbuffer for read ACK
 * a successful read command returns an ACK character in first positionb followed by the readout bytes
 */
int readOk(unsigned char* rxbuffer, int read_N)
{
   if ((read_N>=2) && ((unsigned char)rxbuffer[0]==(unsigned char)0x06))       // check return values for ACK
      return read_N;
   else
      return 0;
}

/*
 * parse rxbuffer for write ACK
 * a successful write command returns only one ACK character
 */
int writeOk(unsigned char* rxbuffer, int read_N)
{
   if ((read_N==1) && ((unsigned char)rxbuffer[0]==(unsigned char)0x06))       // check return values for ACK
      return read_N;
   else
      return 0;
}

/*
* code from: https://stackoverflow.com/questions/10404448/getopt-h-compiling-linux-c-code-in-windows
* https://gist.github.com/superwills/5815344
* getopt --
*      Parse argc/argv argument vector.
*/
int getopt(int nargc, char * const nargv[], const char *ostr)
{
  static char *place = EMSG;              /* option letter processing */
  const char *oli;                        /* option letter list index */

  if (optreset || !*place) {              /* update scanning pointer */
    optreset = 0;
    if (optind >= nargc || *(place = nargv[optind]) != '-') {
      place = EMSG;
      return (-1);
    }
    if (place[1] && *++place == '-') {      /* found "--" */
      ++optind;
      place = EMSG;
      return (-1);
    }
  }                                       /* option letter okay? */
  if ((optopt = (int)*place++) == (int)':' ||
    !(oli = strchr(ostr, optopt))) {
      /*
      * if the user didn't specify '-' as an option,
      * assume it means -1.
      */
      if (optopt == (int)'-')
        return (-1);
      if (!*place)
        ++optind;
      if (opterr && *ostr != ':')
        (void)printf("illegal option -- %c\n", optopt);
      return (BADCH);
  }
  if (*++oli != ':') {                    /* don't need argument */
    optarg = NULL;
    if (!*place)
      ++optind;
  }
  else {                                  /* need an argument */
    if (*place)                     /* no white space */
      optarg = place;
    else if (nargc <= ++optind) {   /* no arg */
      place = EMSG;
      if (*ostr == ':')
        return (BADARG);
      if (opterr)
        (void)printf("option requires an argument -- %c\n", optopt);
      return (BADCH);
    }
    else                            /* white space */
      optarg = nargv[optind];
    place = EMSG;
    ++optind;
  }
  return (optopt);                        /* dump back option letter */
}