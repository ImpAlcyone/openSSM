#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <stdint.h>

#define MAX_LENGTH 255
#define MAX_OUTPUTLINE_LENGTH 1024

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
    char infile[1024] = {'\0'};
    char outfile[1024] = {'\0'};
    long map_address = 0x0;
    long xAxis_address = 0x0;
    long yAxis_address = 0x0;
    int length = 0;

    uint8_t xAxis[MAX_LENGTH];
    uint8_t yAxis[MAX_LENGTH];
    uint8_t map[MAX_LENGTH][MAX_LENGTH];
		
    if(argc < 7){
        printf("Usage: mapextract [-i input_file] [-o output_file] [-m map_address] [-x x-Axis_address] [-y y-Axis_address] [-l length in bytes]\n");
    }

	while ((opt = getopt (argc, argv, "i:o:m:x:y:l")) != -1 )
	{
		switch(opt)
		{
			case 'i': *infile=optarg;
				  break;
			case 'o': *outfile=optarg;
				  break;
			case 'm': map_address = get_address(optarg);
				  break;
            case 'l': length = atoi(optarg);
				  break;
			default : printf("Usage: mapextract [-i input_file] [-o output_file] [-m map_address] [-x x-Axis_address] [-y y-Axis_address] [-l length in bytes]\n");
				  exit(0);
				  break;
		}
			
	}

    FILE *inputRom = fopen(infile, "rb");
    if (infile == NULL) {
        printf("Failed to open rom file %s\n", infile);
        return EXIT_FAILURE;
    }

    // set file pointer to x-Axis and read data
    fseek(infile, xAxis_address, SEEK_SET);
    fread(xAxis, 1, length, infile);

    // set file pointer to x-Axis and read data
    fseek(infile, yAxis_address, SEEK_SET);
    fread(yAxis, 1, length, infile);

    fseek(infile, map_address, SEEK_SET);
    for(int row = 0; row < length; row++){
        if(row > 0){
            fseek(infile, map_address + (length * row), SEEK_SET);
        }
        fread(*map[row], 1, length, infile);
                
    }







}