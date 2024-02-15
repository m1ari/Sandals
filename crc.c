#include <string.h>
#include "crc.h"

unsigned short int crc16(char *data){
        int firstchar=0;

        // Get first non $
        while(data[firstchar]=='$'){
                firstchar++;
        }

        unsigned short int crc=0xFFFF;
        unsigned int i;
        for (i=firstchar; i<strlen(data); i++){
                crc=crc16_update(crc, data[i]);
        }
        return(crc);
}

unsigned short int crc16_update(unsigned short int crc, char c){
        int i;
        crc = crc ^ ((unsigned short int)c << 8);
        for (i=0; i<8; i++) {
                if (crc & 0x8000)
                        crc = (crc << 1) ^ 0x1021;
                else
                        crc <<= 1;
        }
        return crc;
}

