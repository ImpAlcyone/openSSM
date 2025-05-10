#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char **argv)
{
	unsigned char sum=0,write=0;
	unsigned char buffer[0x10000],filemode[4];
	int start,end,i,rc;
	FILE *romfile;

	if ((argc < 2) || (argc > 3))
	{
		printf("Usage: checktcu <filename> [-w]\n");
		exit(1);	
	}

	if (argc == 2)
	{
		write=0;
		strcpy(filemode,"rb");
	}
	else
	{
		if (strcmp(argv[2],"-w")==0)
		{
			write=1;
			strcpy(filemode,"rb+");
		}
		else
		{
			printf("Unknown option: %s\n",argv[2]);
			exit(2);
		}
	}

	if ((romfile=fopen(argv[1],filemode))==0)
	{
		perror("Failed to open input file:");
		exit(1);
	}

	rc=fread(buffer,1,0x10000,romfile);

	if ((rc != 0x4000) && (rc != 0x8000) && (rc != 0x10000))
	{
		perror("Failed to read input file:");
		exit(3);
	}

	start=(rc-0x4000);
	end=rc;

	for (i=start;i<end;i++)
	{
		sum=sum+buffer[i];
	}

	printf("Calculated Checksum: %2.2x (should be 00)\n",sum);

	if (write)
	{
		printf("Writing updated checksums\n");
		buffer[end-0x50]=buffer[end-0x50]-sum;
		fseek(romfile,0,SEEK_SET);
		rc=fwrite(buffer,1,end,romfile);

		if (rc != end)
		{
			perror("Failed to write file:");
			exit(4);
		}
	}

	fclose(romfile);
	return 0;
}

