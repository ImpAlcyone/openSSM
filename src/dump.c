#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <unistd.h>

#include "ssm.h"

int rc = 0;
char *comport = "/dev/ttyUSB0";
char *romfileName = "rom.bin";
FILE *romfile = NULL;

/*---------------------------------*/
/* convert string to address value */
/*---------------------------------*/
static int get_address(char *str)
{
    char *endptr;
    long value = strtol(str, &endptr, 0); // 0 autodetects base
    if (*endptr != '\0' || value < 0 || value > 0xFFFF)
        return -1;
    return (int)value;
}

static int open_output_file(char *outfile)
{	
	romfile = fopen(outfile,"wb");
    if(romfile != NULL){
        return 0;
    }else{
        return 1;
    }
}

/*-----------------------------*/
/* Prepare for dumping ECU ROM */
/*-----------------------------*/
static int prepare(int *romid)
{
	/*--------------------------------*/
	/* Reset the ECU for good measure */
	/*--------------------------------*/
	
	if ((rc=ssm_reset()) != 0)
	{
		printf("ssm_reset() returned %d\n",rc);
		ssm_close();
		fclose(romfile);
		exit(5);
	}

	/*-----------*/
	/* Get ROMID */
	/*-----------*/

	if ((rc=ssm_romid_ecu(romid)) != 0)
	{
		printf("ssm_romid() returned %d\n",rc);
		ssm_close();
		fclose(romfile);
		exit(6);
	}
	
	printf("ECU ROM ID is %6.6x\n", *romid);	
}

static int sanity_check(int start, int end){
	if ((start < 0x0000) || (start > 0xFFFF))
	{
		printf("start address must be between 0x0000 and 0xFFFF\n");
		return 1;
	}
	if ((end   < 0x0000) || (end   > 0xFFFF))
	{
		printf("end address must be between 0x0000 and 0xFFFF\n");
		return 2;
	}
	if (end < start)
	{
		printf("end address must be >= start_address\n");
		return 3;
	}

    return 0;
}

/*----------*/
/* Shutdown */
/*----------*/
static void end(void)
{
	ssm_close();
	fclose(romfile);
}

void set_romfile_name(char *ext_romfileName)
{
    romfileName = ext_romfileName;
}

/*---------------------------------------------------*/
/* Open a connection to the ECU and open output file */
/*---------------------------------------------------*/
int init(void)
{
    if ((rc=ssm_open(comport)) != 0)
	{
		printf("ssm_open() returned %d\n",rc);
		perror("Error Details");
		return -1;
	}

    if((open_output_file(romfileName)) != 0){
        return -1;
    }

    return 0;
}

/*-----------------------------------------------*/
/* Dump the ECU address space to file            */
/* Takes over 3 hours if you dump the whole lot  */
/*-----------------------------------------------*/
int poll_and_write(uint8_t *data, int start, int end, int *romid)
{
	unsigned int retries = 0;
    int address = 0x0;

    if((sanity_check(start, end)) != 0){
        return 1;
    } 

    prepare(romid);

    printf("Dumping ECU address space from %4.4x to %4.4x\n",start,end);
	for (address=start; address<=end; address++)
	{	
		retries = 0;
		while(0 != (rc=ssm_query_ecu(address,data,1))){
			sleep(5);
			if(retries > 10){
				break;
			}
			retries++;
		}
		if (rc != 0){
			printf("ssm_query_ecu() returned %d\n",rc);
			ssm_close();
			fclose(romfile);
			return 2;
		}
		if ((address % 0x10) == 0) /* Newline every 16 bytes */
		{
			printf("\n%4.4x  ",address);
		}
		printf("%2.2x  ", *data);
		fflush(stdout);
		fwrite(data, 1, 1,romfile);
	}
	printf("\n");

    end;
    
    return 0;
}