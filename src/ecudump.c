//
//    **                                                **
//   ******************************************************
//  ***  THIS PROGRAM IS THE PROPERTY OF ALCYONE LIMITED ***
//   ******************************************************
//    **                                                **
//
//  ecudump.c by Phil Skuse
//  Copyright (C) Alcyone Limited 2007.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>

#include "ssm.h"

int get_address(char *str)
{
	int hex;
	char *endptr;

	hex=strtol(str,&endptr,0x10);

	if (endptr != str+4) return -1;

	return hex;
}

int main(int argc, char *argv[])
{

	FILE *fh;
	int address;
	char rc, opt;
	unsigned char data;

	int start = 0x0000;
	int end   = 0xFFFF;

	char *comport="/dev/ttyUSB0";
	char *outfile="ecu.dat";

	int romid;
	
	/*----------------------------*/
	/* Check command line options */
	/*----------------------------*/
	
	while ((opt = getopt (argc, argv, "d:f:s:e:")) != -1 )
	{
		switch(opt)
		{
			case 'd': comport=optarg;
				  break;
			case 'f': outfile=optarg;
				  break;
			case 's': start=get_address(optarg);
				  break;
			case 'e': end=get_address(optarg);
				  break;
			default : printf("Usage: ecudump [-d device] [-f output_file] [-s start_address] [-e end_address]\n");
				  exit(0);
				  break;
		}
			
	}

	/*--------------------------------------*/
	/* Sanity check start and end addresses */
	/*--------------------------------------*/

	if ((start < 0x0000) || (start > 0xFFFF))
	{
		printf("start address must be between 0x0000 and 0xFFFF\n");
		exit(1);
	}
	if ((end   < 0x0000) || (end   > 0xFFFF))
	{
		printf("end address must be between 0x0000 and 0xFFFF\n");
		exit(2);
	}
	if (end < start)
	{
		printf("end address must be >= start_address\n");
		exit(3);
	}

	printf("Dumping ECU address space from %4.4x to %4.4x\n",start,end);

	/*------------------*/
	/* Open output file */
	/*------------------*/
	
	fh=fopen(outfile,"wb");

	/*------------------------------*/
	/* Open a connection to the ECU */
	/*------------------------------*/
	
	if ((rc=ssm_open(comport)) != 0)
	{
		printf("ssm_open() returned %d\n",rc);
		perror("Error Details");
		fclose(fh);
		exit(4);
	}

	/*--------------------------------*/
	/* Reset the ECU for good measure */
	/*--------------------------------*/
	
	if ((rc=ssm_reset()) != 0)
	{
		printf("ssm_reset() returned %d\n",rc);
		ssm_close();
		fclose(fh);
		exit(5);
	}

	/*-----------*/
	/* Get ROMID */
	/*-----------*/

	if ((rc=ssm_romid_ecu(&romid)) != 0)
	{
		printf("ssm_romid() returned %d\n",rc);
		ssm_close();
		fclose(fh);
		exit(6);
	}
	
	printf("ECU ROM ID is %6.6x\n",romid);
	
	/*-----------------------------------------------*/
	/* Dump the ECU address space to file            */
	/* Takes over 3 hours if you dump the whole lot  */
	/*-----------------------------------------------*/
	unsigned int retries = 0;
	for (address=start; address<=end; address++)
	{	
		retries = 0;
		while(0 != (rc=ssm_query_ecu(address,&data,1))){
			sleep(5);
			if(retries > 10){
				break;
			}
			retries++;
		}
		if (rc != 0){
			printf("ssm_query_ecu() returned %d\n",rc);
			ssm_close();
			fclose(fh);
			exit(7);
		}
		if ((address % 0x10) == 0) /* Newline every 16 bytes */
		{
			printf("\n%4.4x  ",address);
		}
		printf("%2.2x  ",data);
		fflush(stdout);
		fwrite(&data,1,1,fh);
	}

	printf("\n");

	/*----------*/
	/* Shutdown */
	/*----------*/

	ssm_close();
	fclose(fh);

	return 0;
}

