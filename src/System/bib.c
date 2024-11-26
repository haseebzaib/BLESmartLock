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

int8_t bib_parseTimeDateAndValidate(char* buffer,uint8_t *hour, uint8_t *minute, uint8_t *second, uint8_t *day, uint8_t *month, uint8_t *year)
{

   if (strlen(buffer) > 17) {
       LOG_ERR("data length is bigger than it needs to be\n");
        return -1; // Invalid length
    }

      // Validate the format
    if (buffer[2] != ':' || buffer[5] != ':' || buffer[8] != '|' || buffer[11] != '/' || buffer[14] != '/') {
           LOG_ERR("Invalid format of Data discarding it\n");
        return -1; // Delimiters are not in the correct positions
    }

        // Check that all numeric positions contain valid digits
    for (size_t i = 0; i < 17; ++i) {
        if ((i == 2 || i == 5 || i == 8 || i == 11 || i == 14) && buffer[i] != ':' && buffer[i] != '|' && buffer[i] != '/') {
            LOG_ERR("Invalid format of Data discarding it\n");
            return -1; // Ensure delimiters are in the correct positions
        } else if (i != 2 && i != 5 && i != 8 && i != 11 && i != 14) {
            if (buffer[i] < '0' || buffer[i] > '9') {
                LOG_ERR("Invalid digits in data\n");
                return -1; // Invalid digit
            }
        }
    }

        // Parse the values into individual bytes
    *hour = (buffer[0] - '0') * 10 + (buffer[1] - '0');
    *minute = (buffer[3] - '0') * 10 + (buffer[4] - '0');
    *second = (buffer[6] - '0') * 10 + (buffer[7] - '0');
    *day = (buffer[9] - '0') * 10 + (buffer[10] - '0');
    *month = (buffer[12] - '0') * 10 + (buffer[13] - '0');
    *year = (buffer[15] - '0') * 10 + (buffer[16] - '0');

    // Validate the ranges of the extracted values
    if (*hour > 23 || *minute > 59 || *second > 59 || *day < 1 || *day > 31 || *month < 1 || *month > 12) {
           LOG_ERR("Values out of range discarding it\n");
        return -1; // Values out of range
    }



return 0;

}