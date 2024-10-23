#include "bib.h"
#include "crypto/cryptoapp.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

LOG_MODULE_REGISTER(bib, LOG_LEVEL_DBG);

int8_t bib_parseMainMsg(uint8_t *MainMsg,uint16_t MainMsgLen,uint8_t *Counter_IV,uint8_t *CipherText,uint16_t *CipherLength)
{

   
    // Ensure packet starts with SOH and ends with EOT
    if (MainMsg[0] != SOH || MainMsg[MainMsgLen - 1] != EOT) {
        LOG_ERR("Invalid packet format!\n");
        return -1;
    }

    size_t pos = 1; // Start after SOH

    // Extract the Counter (IV)
    memcpy(Counter_IV, &MainMsg[pos], 16);
    pos += 16;

    // Ensure the next byte is the first EM
    if (MainMsg[pos] != EM) {
        LOG_ERR("Invalid packet format! Missing first EM.\n");
        return -1;
    }
    pos++; // Move past first EM

    if(MainMsg[MainMsgLen - 1] == EOT)
    {
      *CipherLength = (MainMsg[MainMsgLen - 3] << 8) | MainMsg[MainMsgLen - 2]; // 2-byte length
    }
    else
    {
       LOG_ERR("No EOT so data is not right and cant extract cipherlength\n");
        return -1;
    }

    if(*CipherLength >= MainMsgLen )
    {
      LOG_ERR("Cipherlength is bigger than message length\n");
        return -1;
    }

    // Extract CipherText based on CipherLength
    memcpy(CipherText, &MainMsg[pos], *CipherLength);
 

    return 0; // Success

}



int8_t bib_parseInnerMsg(uint8_t *MainMsg,uint16_t MainMsgLen,uint8_t *UniqueId,uint8_t *DataType,uint8_t *MsgId,uint8_t *Data,uint16_t *MsgLength)
{

      
    // Ensure packet starts with STX and ends with EOT
    if (MainMsg[0] != STX || MainMsg[MainMsgLen - 1] != EOT) {
        LOG_ERR("Invalid packet format!\n");
        return -1;
    }

    
    size_t pos = 1; // Start after STX

    // Extract the unique id
    memcpy(UniqueId, &MainMsg[pos], 8);
    pos += 8;

    
    // Ensure the next byte is the first EM
    if (MainMsg[pos] != EM) {
        LOG_ERR("Invalid packet format! Missing first EM.\n");
        return -1;
    }
    pos++; // Move past first EM

     // Extract the datatype
    memcpy(DataType, &MainMsg[pos], 1);
     pos += 1;

      // Ensure the next byte is the first EM
    if (MainMsg[pos] != EM) {
        LOG_ERR("Invalid packet format! Missing first EM.\n");
        return -1;
    }
    pos++; // Move past first EM

      memcpy(MsgId, &MainMsg[pos], 1);
      pos += 1;


          // Ensure the next byte is the first EM
    if (MainMsg[pos] != EM) {
        LOG_ERR("Invalid packet format! Missing first EM.\n");
        return -1;
    }
     pos++; // Move past first EM


     if(MainMsg[MainMsgLen - 1] == EOT)
    {
      *MsgLength = (MainMsg[MainMsgLen - 4] << 8) | MainMsg[MainMsgLen - 3]; // 2-byte length
    }
    else
    {
       LOG_ERR("No EOT so data is not right and cant extract data length\n");
        return -1;
    }

    if(*MsgLength >= MainMsgLen )
    {
      LOG_ERR("data length is bigger than message length\n");
        return -1;
    }


  // Extract CipherText based on CipherLength
    memcpy(Data, &MainMsg[pos], *MsgLength);

 

return 0;
}