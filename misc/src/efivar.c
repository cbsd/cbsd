// not ready yet
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int readNVRAM(char *);


int readNVRAM(char *vars)
{
	int skipGUID = 1;
	int fd, fsize;
	char* nvr = 0;
	int len = 0;
	int j=0;

	int bpos=0;
	int o1pos=0;
	int o2pos=0;
	int tpos=0;

	// Boot0001

	unsigned char buffer[540672];
	unsigned char buffer2[540672];
	FILE *ptr;

//	ptr = fopen("/root/test/BHYVE_UEFI_VARS.fd-afterstasrt2","rb");  // r for read, b for binary
	ptr = fopen(vars,"rb");  // r for read, b for binary
	fread(buffer,sizeof(buffer),1,ptr); // read 10 bytes to our buffer

	for(int i = 0; i<540672; i++) {

		if(buffer[i]==10) {
                        buffer2[j]=']';
			j++;
                        buffer2[j]='\n';
                        j++;
                }
//                if(buffer[i]==65) {
//                        buffer2[j]=']';
//			j++;
//                        buffer2[j]='\n';
//                        j++;
//                }
                if ((buffer[i]>31)&&(buffer[i]<128)) {
//                        len++;
//                        if (len > 80) {
//                                len = 0;
//                                buffer2[j]='\n';
//                                j++;
//                        }
			buffer2[j]=buffer[i];
			j++;
                }
	}

	for (int i=0; i<j; i++) {
		if(buffer2[i]=='|') {
			printf("\n");
			continue;
		}
		// A\n
		if((buffer2[i]==65)&&(buffer2[i+1]==93)) {
			printf("\n");
			continue;
		}

		// U<
		if((buffer2[i]==85)&&(buffer2[i+1]==60)) {
			printf("\n");
			continue;
		}
		printf("%c",buffer2[i]);
	}

//	printf("%s",buffer2);

//		printf("%u ", buffer[i]); // prints a series of bytes

//	while (*buffer) {
//		printf("OK LOOP\n");
//	}
	return 0;
}

int main(int argc, char *argv[])
{
	if(argc!=2) {
		printf("<cmd> <path_to_vars>\n");
		exit(0);
	}
	readNVRAM(argv[1]);
	return 0;
}
