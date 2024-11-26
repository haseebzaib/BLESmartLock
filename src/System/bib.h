#ifndef _BIB_H_
#define _BIB_H_

#include "app_defines.h"


extern int8_t bib_parseMainMsg(uint8_t *MainMsg,uint16_t MainMsgLen,uint8_t *Counter_IV,uint8_t *CipherText,uint16_t *CipherLength);
extern int8_t bib_parseInnerMsg(uint8_t *MainMsg,uint16_t MainMsgLen,uint8_t *UniqueId,uint8_t *DataType,uint8_t *MsgId,uint8_t *Data,uint16_t *MsgLength);
extern int8_t bib_parseTimeDateAndValidate(char* buffer,uint8_t *hour, uint8_t *minute, uint8_t *second, uint8_t *day, uint8_t *month, uint8_t *year);




#endif