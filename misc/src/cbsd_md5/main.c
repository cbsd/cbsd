#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "md5.h"

void print_hash(uint8_t *p){
    for(unsigned int i = 0; i < 16; ++i){
        printf("%02x", p[i]);
    }
    printf("\n");
}

int main(int argc, char *argv[]){
    uint8_t result[16];
    if(argc > 1){
        for(int i = 1; i < argc; ++i){
            md5String(argv[i], result);
            print_hash(result);
        }
    }
    else{
        md5File(stdin, result);
        print_hash(result);
    }
}
