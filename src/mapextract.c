#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#define MAX_LENGTH 1024
#define MAX_OUTPUTLINE_LENGTH 0x100000
#define MAX_FILENAME_LENGTH 1024

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

	char opt;
    char *infile = NULL;
    char *outfile = NULL;
    char printLine[MAX_OUTPUTLINE_LENGTH] = {'\0'};
    long map_address = 0x0;
    long xAxis_address = 0x0;
    long yAxis_address = 0x0;
    int length = 16;
    uint16_t written = 0;

    uint8_t xAxis[MAX_LENGTH];
    uint8_t yAxis[MAX_LENGTH];
    uint8_t map[MAX_LENGTH];
		
    if(argc < 7){
        printf("Usage: mapextract [-i input_file] [-o output_file] [-m map_address] [-x x-Axis_address] [-y y-Axis_address] [-l length in bytes]\n");
    }

	while ((opt = getopt(argc, argv, "i:o:m:x:y:l:")) != -1 )
	{
		switch(opt)
		{
			case 'i': infile=optarg;
				  break;
			case 'o': outfile=optarg;
				  break;
			case 'm': map_address = get_address(optarg);
				  break;
            case 'x': xAxis_address = get_address(optarg);
				  break;
            case 'y': yAxis_address = get_address(optarg);
				  break;
            case 'l': length = atoi(optarg);
				  break;
			default : printf("Usage: mapextract [-i input_file] [-o output_file] [-m map_address] [-x x-Axis_address] [-y y-Axis_address] [-l length in bytes]\n");
				  exit(0);
				  break;
		}
			
	}

    FILE *inputRom = fopen(infile, "rb");
    if (inputRom == NULL) {
        printf("Failed to open rom file %s\n", infile);
        return EXIT_FAILURE;
    }

    FILE *outputMap = fopen(outfile, "w");
    if (outputMap == NULL) {
        printf("Failed to open output file %s\n", outfile);
        return EXIT_FAILURE;
    }

    // set file pointer to x-Axis and read data
    fseek(inputRom, xAxis_address, SEEK_SET);
    fread(xAxis, 1, length, inputRom);

    // set file pointer to y-Axis and read data
    fseek(inputRom, yAxis_address, SEEK_SET);
    fread(yAxis, 1, length, inputRom);

    // set file pointer to map and read data
    fseek(inputRom, map_address, SEEK_SET);
    if((fread(map, 1, length * length, inputRom)) != length*length){
        fprintf(stderr, "Failed to read map from address %lX\n", map_address);
        return EXIT_FAILURE;
    }

    fclose(inputRom);   


    char *writePos = printLine;

    for(int idx = 0; idx < length*length ; idx++){
        
        if(0 == (idx % length)){
            written = snprintf(writePos, MAX_OUTPUTLINE_LENGTH - written, "%02X,,", yAxis[idx / length]);
            writePos += written;
        }
        written = snprintf(writePos, MAX_OUTPUTLINE_LENGTH - written, "%02X", map[idx]);
        writePos += written;
        if(0 == ((idx + 1) % length)){
            written = snprintf(writePos, MAX_OUTPUTLINE_LENGTH - written, "\n");
        }else{
            written = snprintf(writePos, MAX_OUTPUTLINE_LENGTH - written, ",");
        }
        writePos += written;                
    }

    written = snprintf(writePos, MAX_OUTPUTLINE_LENGTH - written, "\n,,");
    writePos += written; 

    for(int idx = 0; idx < length ; idx++){
        written = snprintf(writePos, MAX_OUTPUTLINE_LENGTH - written, "%02X", xAxis[idx]);
        writePos += written; 
        if(idx == (length - 1)){
            written = snprintf(writePos, MAX_OUTPUTLINE_LENGTH - written, "\n");
        }else{
            written = snprintf(writePos, MAX_OUTPUTLINE_LENGTH - written, ",");
        }
        writePos += written;
    }
    printf("\n%s\n", printLine);
    fputs(printLine, outputMap);

    fclose(outputMap);


    return EXIT_SUCCESS;
}