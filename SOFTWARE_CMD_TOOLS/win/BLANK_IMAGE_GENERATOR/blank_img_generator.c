// Copyright (C) 2020 IAM Electronic GmbH <info@iamelectronic.com>
// This work is free. You can redistribute it and/or modify it under the
// terms of the Do What The Fuck You Want To Public License, Version 2,
// as published by Sam Hocevar. See http://www.wtfpl.net/ for more details.
// ************************************************************************
// File Name	  : 'blank_img_generator.c'
// Title		     : Command line tool for generating blank EEPROM iamges
// Company		  : IAM Electronic GmbH
// Author		  : PFH
// Created		  : 19-MARCH-2020
// Last modified : 19-MARCH-2020
// Target HW	  : T0009 FMC FRU Programmer
// Target OS     : Windows
// ************************************************************************
#include <windows.h>
#include <stdio.h>

#define REVISION_MAJOR 1
#define REVISION_MINOR 1
#define BUILD_NUMBER   1

#define DEFAULT_SIZE 256
#define DEFAULT_CHAR 0xAA

#define BADCH   (int)'?'
#define BADARG  (int)':'
#define EMSG    ""

int getopt(int nargc, char * const nargv[], const char *ostr);  // windows clone getopt from unistd.h

int verbose_on;                                                 // enable/disable printf stdout
int opterr;		                                                 // if error message should be printed
int optind;		                                                 // index into parent argv vector
int optopt;		                                                 // character checked for validity
int optreset;                                                   // reset getopt
char *optarg;	                                                 // argument associated with option

void usage(void)
{
   printf("\nBLANK IMAGE GENERATOR %d.%d.%d\n", REVISION_MAJOR, REVISION_MINOR, BUILD_NUMBER);
   printf(" Copyright (C) 2020 IAM Electronic GmbH <info@iamelectronic.com>\n"
          " This work is free. You can redistribute it and/or modify it under the\n"
          " terms of the Do What The Fuck You Want To Public License, Version 2,\n"
          " as published by Sam Hocevar. See http://www.wtfpl.net/ for more details.\n\n\n");
   printf(" Image options:\n"
          "    -c <0 .. 255>\tset default character (1 byte in decimal) for file content\n"          
          "    -l <1024 .. 524288> set image size size in bits (only multiples of 1024 are allowed)\n"
          "    -L  <128 ..  65536> set image size size in Bytes (only multiples of 128 are allowed)\n"
          "    -o <filename.bin>\tset output filename for blank image\n\n");
}

int main(int argc, char **argv)
{
   int  ret;                     // default return value
   unsigned int  N_bytes;        // number of bytes for image
   char opt;                     // helper for parsing argument strings
   int opt_num;                  // helper for parsing numerical strings
   unsigned char file_content;   // default character for blank image
   FILE* fp;                     // file pointer to outpput file   
   
   opterr = 1;
   optind = 1; 
   
   N_bytes      = DEFAULT_SIZE;  // default value
   file_content = DEFAULT_CHAR;  // default value

   while ((opt = getopt (argc, argv, "c:l:L:o:?h")) != -1)
   {    
      switch (opt)
      {
         case 'c':
            opt_num = atoi(optarg);
            if ((opt_num >= 0) && (opt_num<=255))      // only byte values are valid
               file_content = (unsigned char)opt_num;  // cast byte to unsigned char
            else
               file_content = DEFAULT_CHAR;            // invalid range, use default

            printf("\nSet default char to: 0x%02X\n",file_content);

            break;

         case 'l':
            opt_num = atoi(optarg);
            if ((opt_num > 0) && (opt_num<=524288) && ((opt_num % 1024)==0))  // check range (max 524288 bits = 65536 bytes)
               N_bytes = opt_num / 8;                                         // convert bits in bytes
            else
               N_bytes = DEFAULT_SIZE;                                        // invlaid range, use default value
            
            printf("\nSet image size: %d bytes (%d bits)\n",N_bytes,N_bytes*8);
            
            break;

         case 'L':
            opt_num = atoi(optarg);
            if ((opt_num > 0) && (opt_num<=65536) && ((opt_num % 128)==0))    // check range (max. 65536 bytes can be addressed with two-byte addresses)
               N_bytes = opt_num;
            else
               N_bytes = DEFAULT_SIZE;

            printf("\nSet image size: %d bytes (%d bits)\n",N_bytes,N_bytes*8);

            break;                     

         case 'o':
            if (optarg != NULL)
            {
               fp = fopen(optarg, "wb");           // open file for writing binary content
               if (fp!=NULL)
               {
                  for (int i=0;i<N_bytes;i++)               
                     fwrite(&file_content,1,1,fp); // write conent bytewise in file
                  fflush(fp);
                  fclose(fp);
                  printf("\nSuccessfully generated image file %s (%d Bytes)\n",optarg,N_bytes);
               }
               else
                  printf("\nCannot write to file %s\n", optarg);
            }
            else
            {
               printf("\nWrong command line argument %s\n", optarg);
            }     
            break;

         case '?':            
         case 'h':            
            usage();
            switch (optopt)
            {
               case 'c': printf("\n\nExample usage:\nblank_img_generator.exe -c 255\n"); break;
               case 'l': printf("\n\nExample usage:\nblank_img_generator.exe -l 2048\n"); break;
               case 'L': printf("\n\nExample usage:\nblank_img_generator.exe -L 256\n"); break;               
               case 'o': printf("\n\nExample usage:\nblank_img_generator.exe -d blankimage.bin\n"); break;
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