// PN532 library by adafruit/ladyada
// MIT license

// authenticateBlock, readMemoryBlock, writeMemoryBlock contributed
// by Seeed Technology Inc (www.seeedstudio.com)

#include "PN532.h"
#include <Wire.h>

//#define PN532DEBUG

uint8_t pn532ack[] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
uint8_t pn532error[] = {0x00, 0x00, 0xFF, 0x01, 0xFF, 0x7F, 0x81, 0x00};
uint8_t pn532response_firmwarevers[] = {0x00, 0xFF, 0x06, 0xFA, 0xD5, 0x03};

#define COMMAND_RESPONSE_SIZE 3 
#define TS_GET_DATA_IN_MAX_SIZE  262 + 3
#define CHUNK_OF_DATA 64

//Buffern som allt l�ggs p�, rymmer ett standard PN532 commando
uint8_t pn532_packetbuffer[TS_GET_DATA_IN_MAX_SIZE];

//Creating a PN532 
PN532::PN532(uint8_t irq, uint8_t reset) 
{
    _irq = irq;
    _reset = reset;

    pinMode(_irq, INPUT);
    pinMode(_reset, OUTPUT);
}

/************** high level functions ************/

//Initializes the PN532
void PN532::initializeReader() 
{
  Wire.begin(); // Joining I2C buss as master
}

uint32_t PN532::SAMConfig() 
{
    #ifdef PN532DEBUG
      Serial.println("<SAMconfig>: Creating SAMconfig message");
    #endif
    
    pn532_packetbuffer[0] = PN532_SAMCONFIGURATION;
    pn532_packetbuffer[1] = 0x01; // normal mode;
    pn532_packetbuffer[2] = 0x14; // timeout 50ms * 20 = 1 second
    pn532_packetbuffer[3] = 0x01; // use IRQ pin!
    
    uint32_t result = sendCommandCheckAck(pn532_packetbuffer, 4, 1000);

    if (IS_ERROR(result)) 
    {
        Serial.println("SAMConfig: SEND_COMMAND_TX_TIMEOUT_ERROR");
        return result;
    }

    // read data packet
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    return fetchResponse(PN532_SAMCONFIGURATION, response);
}

uint32_t PN532::getFirmwareVersion() 
{
    uint32_t versiondata;

    pn532_packetbuffer[0] = PN532_FIRMWAREVERSION;

    if (IS_ERROR(sendCommandCheckAck(pn532_packetbuffer, 1,1000))) 
    {
        return CONNECTION_ERROR;
    }

    // read response Packet
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    if (IS_ERROR(fetchResponse(PN532_FIRMWAREVERSION, response))) // bytt till fetchResponse, fungerar det?
    {
       return CONNECTION_ERROR;
    }
    
    //response->printResponse();
    
    versiondata = response->data[0];
    versiondata <<= 8;
    versiondata |= response->data[1];
    versiondata <<= 8;
    versiondata |= response->data[2];
    versiondata <<= 8;
    versiondata |= response->data[3];
    
        if (! versiondata) {
        Serial.print("Didn't find PN53x board");
        while (1); // halt
    }
    /*
    // Got ok data, print it out!
    Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
    Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
    Serial.print('.'); 
    Serial.println((versiondata>>8) & 0xFF, DEC);
    Serial.print("Supports "); Serial.println(versiondata & 0xFF, HEX); 
 */
     return versiondata;   
  }
  
uint32_t PN532::getGeneralStatus(){
  
    pn532_packetbuffer[0] = PN532_GETGENERALSTATUS;
    
    uint32_t result = sendCommandCheckAck(pn532_packetbuffer, 1, 1000);

    if (IS_ERROR(result)) 
    {
        Serial.println("getGeneralStatus: SEND_COMMAND_TX_TIMEOUT_ERROR");
        return result;
    }

    // read data packet
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    return fetchResponse(PN532_SAMCONFIGURATION, response);
}

uint32_t PN532::configurePeerAsTarget(uint8_t type)
{
    static const uint8_t npp_client[44] = { PN532_TGINITASTARGET,
                             0x00, 
                             0x00, 0x00, //SENS_RES
                             0x00, 0x00, 0x00, //NFCID1
                             0x00, //SEL_RES

                             0x01, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // POL_RES
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           
                             0x00, 0x00,
                            
                             0x01, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //NFCID3t: Change this to desired value

                             0x06, 0x46, 0x66, 0x6D, 0x01, 0x01, 0x10, 0x00
                             };
    
    static const uint8_t npp_server[44] = { PN532_TGINITASTARGET,
                             0x02, // Mode: DEP only, yes
                             
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x40, // MIFARE

                             0x01, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // FELICA
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // FELICA
                             0xFF, 0xFF, // FELICA
                            
                             0x01, 0xFE, 0x0F, 0xBB, 0xBA, 0xA6, 0xC9, 0x89, 0x00, 0x00, //NFCID3t: Change this to desired value

                             0x06, // Gt length: 6 Bytes is MAX
                             0x46, 0x66, 0x6D, 0x01, 0x01, 0x10, // ATR_RES
                             0x00 // Tk length
                             
                             };

                             
    if (type == NPP_CLIENT)
    {
       for(uint8_t iter = 0;iter < 44;iter++)
       {
          pn532_packetbuffer[iter] = npp_client[iter];
       }
    }
    else if (type == NPP_SERVER)
    {
       for(uint8_t iter = 0;iter < 44;iter++)
       {
          pn532_packetbuffer[iter] = npp_server[iter];
       }
    }
    
    uint32_t result;
    result = sendCommandCheckAck(pn532_packetbuffer, 44, 2000);
   
    if (IS_ERROR(result))
    {
        return result;
    }
    
    sleepArduino(); // Sleep until phone wakes up the PN532
        
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    return fetchResponse(PN532_TGINITASTARGET, response);
}

uint32_t PN532::targetRxData(uint8_t *DataIn)
{
    ///////////////////////////////////// Receiving from Initiator ///////////////////////////
    pn532_packetbuffer[0] = PN532_TGGETDATA;
    uint32_t result = sendCommandCheckAck(pn532_packetbuffer, 1, 1000);
    if (IS_ERROR(result)) {
        //Serial.println(F("SendCommandCheck Ack Failed"));
        return NFC_READER_COMMAND_FAILURE;
    }
    
    // read data packet
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    
    result = fetchResponse(PN532_TGGETDATA, response);
    
    if (IS_ERROR(result))
    {
       return NFC_READER_RESPONSE_FAILURE;
    }
 
    if (response->data[0] == 0x00) // börjar NDEF message med 0x00?
    {
       uint32_t ret_len = response->len - 2; // ändrat
       memcpy(DataIn, &(response->data[1]), ret_len);
       return ret_len;
    }
    
    return (GEN_ERROR | response->data[0]);
}



uint32_t PN532::targetTxData(uint8_t *DataOut, uint32_t dataSize)
{
    ///////////////////////////////////// Sending to Initiator ///////////////////////////
    pn532_packetbuffer[0] = PN532_TGSETDATA;
    uint8_t commandBufferSize = (1 + dataSize);
    for(uint8_t iter=(1+0);iter < commandBufferSize; ++iter)
    {
        pn532_packetbuffer[iter] = DataOut[iter-1]; //pack the data to send to target
    }
    
    uint32_t result = sendCommandCheckAck(pn532_packetbuffer, commandBufferSize, 1000);
    if (IS_ERROR(result)) {
        Serial.println(F("TX_Target Command Failed."));
        return result;
    }
    
    
    // read data packet
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    
    result = fetchResponse(PN532_TGSETDATA, response);
    if (IS_ERROR(result))
    {
       return result;
    }
    
    #ifdef PN532DEBUG
    
           response->printResponse();
    #endif
    
    if (response->data[0] != 0x00)
    {
       return (GEN_ERROR | response->data[0]);
    }
    return RESULT_SUCCESS; //No error
}

/********** LARGE CHUNK CUT OUT, placed at file bottom  **********/

/************** mid level functions ***********/

// default timeout of one second
uint32_t PN532::sendCommandCheckAck(uint8_t *cmd, 
                                    uint8_t cmdlen, 
                                    uint16_t timeout,
                                    boolean debug) 
{
    uint16_t timer = 0;
    if(debug)
      Serial.println("<sendCommandCheckAck> ");
    
     // send the command frame
    sendFrame(cmd, cmdlen);
    
    // Wait for chip to say it has data to send
   
    // read acknowledgement
    if (!fetchCheckAck(timeout)) {
        return SEND_COMMAND_RX_ACK_ERROR;
        Serial.println("<SendcommandCheckAck>: ACK error"); 
    }
    
    #ifdef PN532DEBUG       
      Serial.println("<sendCommandCheckAck>: PN532 Successfully recieved host command");
    #endif  
    return RESULT_SUCCESS; // ack'd command
}

void PN532::sendFrame(uint8_t* cmd, uint8_t cmdlen)
{
  uint8_t checksum;
  
  //Increasing the cmdlen to include the TFI (Direction)
  cmdlen++;

  delay(2);                               // or whatever the delay is for waking up the board    
  Wire.beginTransmission(PN532_I2C_ADDRESS);
  
  // Make whole frame  
  Wire.write(PN532_PREAMBLE);
  Wire.write(PN532_STARTCODE1);
  Wire.write(PN532_STARTCODE2);

  Wire.write(cmdlen);                   //LEN
  uint8_t cmdlen_1 = ~cmdlen + 1;       //calculating the two's complement of cmdlen is used as the LCS
  Wire.write(cmdlen_1);		        //LCS        
  Wire.write(PN532_HOSTTOPN532);        //TFI 

  checksum += PN532_HOSTTOPN532;         

  #ifdef PN532DEBUG 
    Serial.print("<sendFrame>: Sending: ");
    Serial.print(F(" 0x")); 
    Serial.print(PN532_PREAMBLE, HEX);
    Serial.print(F(" 0x")); 
    Serial.print(PN532_PREAMBLE, HEX);
    Serial.print(F(" 0x")); 
    Serial.print(PN532_STARTCODE2, HEX);
    Serial.print(F(" 0x")); 
    Serial.print(cmdlen, HEX);
    Serial.print(F(" 0x")); 
    Serial.print(cmdlen_1, HEX);
    Serial.print(F(" 0x")); 
    Serial.print(PN532_HOSTTOPN532, HEX);
  #endif
 
     
  for (uint8_t i=0; i<cmdlen-1; i++) {
    Wire.write(cmd[i]);                //PD0...PDN
    checksum += cmd[i];  

    #ifdef PN532DEBUG
      Serial.print(F(" 0x")); 
      Serial.print(cmd[i], HEX);
    #endif
  }

  uint8_t checksum_1= ~checksum + 1;      
  
  Wire.write(checksum_1);             //DCS
  Wire.write(PN532_POSTAMBLE); 


  #ifdef PN532DEBUG 
  
    Serial.print(F(" 0x")); 
    Serial.print(checksum_1, HEX);
    Serial.print(F(" 0x")); 
    Serial.print(PN532_POSTAMBLE, HEX);
    Serial.println();
  #endif
  
   Wire.endTransmission(); // Nu datan faktiskt sänds iväg
  
  Serial.println("<sendFrame>: Command sent");
}
boolean PN532::fetchCheckAck(uint16_t timeout)
{
    //Create buffer of size 6 to recieve an ack
    uint8_t ackbuff[6];

    
    uint16_t timer = 0;
    //Wait timeout for PN532 to send the ack, else timeout
    while ((checkDataAvailable()) == PN532_I2C_NO_DATA){
      delay(10);
      timer+=10;
      
      if (timer > timeout){
        Serial.println("<fetchCheckAck>: Timeout");
        return 0;
     }
    }
    
    Wire.requestFrom((uint8_t)PN532_I2C_ADDRESS, (uint8_t)7);
    Wire.read();//take away rubbish due to I2C delay
            
    #ifdef PN532DEBUG
      Serial.println("<fetchCheckAck>: Data recieved from PN532:  ");
    #endif  
    for (uint16_t i=0; i<7; i++){
        delay(1);
        ackbuff[i] = Wire.read();
        
        #ifdef PN532DEBUG    
        
            Serial.print(F(" 0x"));
            Serial.print(ackbuff[i], HEX);            
        #endif
    }
    
    #ifdef PN532DEBUG
      Serial.println();   
      if(0 == strncmp((char *)ackbuff, (char *)pn532ack, 6))
      Serial.println("<fetchCheckAck>: Ack recieved");
      Serial.println();
    #endif
      
    return (0 == strncmp((char *)ackbuff, (char *)pn532ack, 6));
}


//Bryter isär svaret fr�n PN532 
uint32_t PN532::fetchResponse(uint8_t cmdCode, PN532_CMD_RESPONSE *response) 
{
  
    #ifdef PN532DEBUG
      Serial.println("<fetchResponse>");
    #endif
    
    uint8_t calc_checksum = 0;
    uint8_t ret_checksum;
        
    //response->header[0] = response->header[1] = 0xAA;  Useless?
    
    uint32_t retVal = RESULT_SUCCESS;
      
    
    //Fetch the data
    fetchData((uint8_t *)response, 1000);
    
    /* Won't work
    // Check if an error frame was fetched
    if(0 == strncmp((char *)response, (char *)pn532error, 8)){
      retVal = SEND_COMMAND_RX_ERROR_FRAME;
      
        if(debug){
          Serial.println("<fetchResponse>: Error frame recieved");
        }
      return retVal;
    }*/
    
    retVal = response->verifyResponse(cmdCode) ? RESULT_SUCCESS : INVALID_RESPONSE;      
    
    //Dont get this part
    if (RESULT_OK(retVal)){  
      
      calc_checksum += response->direction;
      calc_checksum += response->responseCode;
      
      // Add data fields to checksum
      // 2 is removed since direction (TFI) and responsCode is included in LEN
      //Never run for frames with len < 3, e.g response to a SAMconfig()
        uint8_t i = 0;
        for ( i; i < response -> len -2 ; i++){
            calc_checksum +=  response->data[i];
         }
                  
       // Find Packet Data Checksum. Will be pointed to after the above loop
       uint8_t ret_checksum = response->data[i++];
         
        
        // Check if the checksums are correct
         if (((uint8_t)(calc_checksum + ret_checksum)) != 0x00) 
         {
            Serial.println(F("<fetchResponse>: Invalid Checksum recievied."));
            retVal = INVALID_CHECKSUM_RX;
         }         
         
        // Find Postamble
         uint8_t postamble = response->data[i];         
                 
         
         if (RESULT_OK(retVal) && postamble != 0x00) 
         {
             retVal = INVALID_POSTAMBLE;
             Serial.println(F("Invalid Postamble."));
         }       
    }
    
   
    #ifdef PN532DEBUG
      response->printResponse();
      
      Serial.println();
      Serial.print(F("<fetchResponse>: Calculated Checksum: 0x"));
      Serial.println(calc_checksum, HEX);
       
    #endif

    return retVal;
}

void PN532::fetchData(uint8_t* buff, uint16_t timeout) 
{    
    #ifdef PN532DEBUG     
        Serial.println("<fetchData>");
    #endif
    
    //TODO add a timeout
      uint16_t time = 0;
      while(checkDataAvailable() == PN532_I2C_NO_DATA){        
        Serial.println("<fetchData>: Nothing to fetch");
        delay(10);
      }
    
    //Fetches a predefined Chunk of data.
    //Will continue to fetch chuks aslong as PN532 has something to send
    while(checkDataAvailable() == PN532_I2C_DATA_TO_FETCH){
      Serial.println("<fetchData>: Fetching new chunk of data");
      Wire.requestFrom((uint8_t)PN532_I2C_ADDRESS, (uint8_t)(CHUNK_OF_DATA+1));
      delay(10);
      Wire.read();//take away rubbish due to I2C delay
    }
    
    //Changed from n = argument set at 150 to actual size of read data..    
    uint8_t n = Wire.available();
    
   #ifdef PN532DEBUG        
      Serial.print("<fetchData>: ");
      Serial.print(n);
      Serial.println(" bytes fetched");
      Serial.println("<fetchData>: Data recieved from PN532: ");
   #endif
    
    for (uint16_t i=0; i<n; i++){
        delay(1);
        buff[i] = Wire.read();
        
        #ifdef PN532DEBUG           
            Serial.print(F(" 0x"));
            Serial.print(buff[i], HEX);
        #endif
    }

    #ifdef PN532DEBUG
        Serial.println();
        Serial.println("<fetchData>: data fetched");
    #endif      
}

/************** low level functions ***********/

// Puts Arduino to sleep
void PN532::sleepArduino()
{    
    // Enable sleep mode
    sleep_enable();
    
    Serial.println("Going to Sleep\n");
    delay(100); // delay so that debug message can be printed before the MCU goes to sleep
    
    // Puts the device to sleep.
    sleep_mode();
    
    
    // Program continues execution HERE
    // when an interrupt is recieved.
    Serial.println("Woke up");
    // Disable sleep mode
    sleep_disable();    
}

//Ers�tter helt readspistatus, i I2C r�cker det med att titta p� IRQ linan f�r att av�gra om PN532 �r upptagen. What? /jonas
uint8_t PN532::checkDataAvailable(void) {
  uint8_t x = digitalRead(_irq);
  
  if (x == 1)
    return PN532_I2C_NO_DATA;
  else
    return PN532_I2C_DATA_TO_FETCH;
}


/******* PN532_CMD_RESPONSE methods */

boolean PN532_CMD_RESPONSE::verifyResponse(uint32_t cmdCode)
{
    return ( header[0] == 0x00 && 
             header[1] == 0xFF &&
            ((uint8_t)(len + len_chksum)) == 0x00 && 
            direction == 0xD5 && 
            (cmdCode + 1) == responseCode);
}

void PN532_CMD_RESPONSE::printResponse(){
  #ifdef PN532DEBUG
    Serial.println();
    Serial.println("<fetchResponse>: Response");
    Serial.print("Preamble: 0x");
    Serial.println(preamble, HEX);
    Serial.print("Startcode: 0x");
    Serial.print(header[0], HEX);
    Serial.print(" 0x");
    Serial.println(header[1], HEX);
    Serial.print("Len: 0x");
    Serial.println(len, HEX);
    Serial.print("LCS: 0x");
    Serial.println(len_chksum, HEX);
    Serial.print("Direction: 0x");
    Serial.println(direction, HEX);
    Serial.print("Response Command: 0x");
    Serial.println(responseCode, HEX);
  
    Serial.println("Data: ");
    
    uint8_t i;
    for (i = 0; i < len-2; ++i){
        Serial.print(F("0x"));
        Serial.print(data[i], HEX);
        Serial.print(F(" "));
    } 
  
      Serial.println();
      Serial.print("Returned Checksum: 0x");
      Serial.println(data[i++], HEX); 
      Serial.print("Postamble: 0x");   
      Serial.println(data[i], HEX);
 #endif     
}


/********** CUT OUT **********


uint32_t PN532::initiatorTxRxData(uint8_t *DataOut,
                           uint32_t dataSize,
                           uint8_t *DataIn,
                           boolean debug)
{
    pn532_packetbuffer[0] = PN532_INDATAEXCHANGE;
    pn532_packetbuffer[1] = 0x01; //Target 01

    for(uint8_t iter=(2+0);iter<(2+dataSize);iter++)
    {
        pn532_packetbuffer[iter] = DataOut[iter-2]; //pack the data to send to target
    }
    
    uint32_t result = sendCommandCheckAck(pn532_packetbuffer, dataSize+2);

    if (IS_ERROR(result))
    {
        return result;
    }
    
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    if (!readspicommand(PN532_INDATAEXCHANGE, response))
    {
       return false;
    }
    
    if (debug)
    {
       response->printResponse();
    }
    
    if (response->data[0] != 0x00)
    {
       return (GEN_ERROR | response->data[0]);
    }
    return RESULT_SUCCESS; //No error
}

uint32_t PN532::getTargetStatus(uint8_t *DataIn)
{
    pn532_packetbuffer[0] = PN532_TGTARGETSTATUS;
    
    if (IS_ERROR(sendCommandCheckAck(pn532_packetbuffer, 1))) {
        return 0;
    }
    
    // read data packet
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    if (RESULT_OK(readspicommand(PN532_TGTARGETSTATUS, response)))
    {
       memcpy(DataIn, response->data, response->data_len);
       return response->data_len;
    }
    
    return 0;
}

uint32_t PN532::authenticateBlock(uint8_t cardnumber  ,//1 or 2
                                  uint32_t cid , //Card NUID
                                  uint8_t blockaddress, //0 to 63
                                  uint8_t authtype, //Either KEY_A or KEY_B
                                  uint8_t * keys)
{
    pn532_packetbuffer[0] = PN532_INDATAEXCHANGE;
    pn532_packetbuffer[1] = cardnumber; // either card 1 or 2 (tested for card 1)
    if(authtype == KEY_A)
    {
        pn532_packetbuffer[2] = PN532_AUTH_WITH_KEYA;
    }
    else
    {
        pn532_packetbuffer[2] = PN532_AUTH_WITH_KEYB;
    }
    pn532_packetbuffer[3] = blockaddress; //This address can be 0-63 for MIFARE 1K card

    pn532_packetbuffer[4] = keys[0];
    pn532_packetbuffer[5] = keys[1];
    pn532_packetbuffer[6] = keys[2];
    pn532_packetbuffer[7] = keys[3];
    pn532_packetbuffer[8] = keys[4];
    pn532_packetbuffer[9] = keys[5];

    pn532_packetbuffer[10] = ((cid >> 24) & 0xFF);
    pn532_packetbuffer[11] = ((cid >> 16) & 0xFF);
    pn532_packetbuffer[12] = ((cid >> 8) & 0xFF);
    pn532_packetbuffer[13] = ((cid >> 0) & 0xFF);
    
    uint32_t result = sendCommandCheckAck(pn532_packetbuffer, 14);

    if (IS_ERROR(result))
    {
        return result;
    }
    
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    result = readspicommand(PN532_INDATAEXCHANGE, response);
    if (IS_ERROR(result))
    {
       return result;
    }


    if((response->data[0] == 0x41) && (response->data[1] == 0x00))
    {
   return RESULT_SUCCESS;
    }

    return (GEN_ERROR | response->data[1]);
}

uint32_t PN532::readMemoryBlock(uint8_t cardnumber, //1 or 2
                                uint8_t blockaddress, //0 to 63
                                uint8_t * block)
{
    pn532_packetbuffer[0] = PN532_INDATAEXCHANGE;
    pn532_packetbuffer[1] = cardnumber; // either card 1 or 2 (tested for card 1)
    pn532_packetbuffer[2] = PN532_MIFARE_READ;
    pn532_packetbuffer[3] = blockaddress; //This address can be 0-63 for MIFARE 1K card
    
    uint32_t result = sendCommandCheckAck(pn532_packetbuffer, 4);

    if (IS_ERROR(result)) {
        return result;
    }

    // read data packet
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    result = readspicommand(PN532_INDATAEXCHANGE, response);
    if (IS_ERROR(result))
    {
       return result;
    }

    if((response->data[0] == 0x41) && (response->data[1] == 0x00))
    {
   return RESULT_SUCCESS; //read successful
    }

    return (GEN_ERROR | response->data[1]);
}

//Do not write to Sector Trailer Block unless you know what you are doing.
uint32_t PN532::writeMemoryBlock(uint8_t cardnumber, //1 or 2
                                 uint8_t blockaddress, //0 to 63
                                 uint8_t * block)
{
    pn532_packetbuffer[0] = PN532_INDATAEXCHANGE;
    pn532_packetbuffer[1] = cardnumber; // either card 1 or 2 (tested for card 1)
    pn532_packetbuffer[2] = PN532_MIFARE_WRITE;
    pn532_packetbuffer[3] = blockaddress;

    for(uint8_t i =0; i <16; i++)
    {
        pn532_packetbuffer[4+i] = block[i];
    }
    
    uint32_t result = sendCommandCheckAck(pn532_packetbuffer, 20);

    if (IS_ERROR(result))
    {
        return result;
    }
    
    // read data packet
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    result = readspicommand(PN532_INDATAEXCHANGE, response);
    if (IS_ERROR(result))
    {
       return result;
    }

    if((response->data[0] == 0x41) && (response->data[1] == 0x00))
    {
   return RESULT_SUCCESS; //read successful
    }

    return (GEN_ERROR | response->data[1]);
}

uint32_t PN532::readPassiveTargetID(uint8_t cardbaudrate)
{
    uint32_t cid;

    pn532_packetbuffer[0] = PN532_INLISTPASSIVETARGET;
    pn532_packetbuffer[1] = 1; // max 1 cards at once (we can set this to 2 later)
    pn532_packetbuffer[2] = cardbaudrate;

    if (IS_ERROR(sendCommandCheckAck(pn532_packetbuffer, 3)))
    {
        return 0; // no cards read
    }

    // read data packet
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    if (IS_ERROR(readspicommand(PN532_INDATAEXCHANGE, response)))
    {
       return 0;
    }
    
    // check some basic stuff
    Serial.print(F("Found "));
    Serial.print(response->data[2], DEC);
    Serial.println(F(" tags"));
    
    if (response->data[2] != 1)
    {
        return 0;
    }
    
    uint16_t sens_res = response->data[4];
    sens_res <<= 8;
    sens_res |= response->data[5];
    
    Serial.print(F("Sens Response: 0x"));
    Serial.println(sens_res, HEX);
    Serial.print(F("Sel Response: 0x"));
    Serial.println(response->data[6], HEX);
    
    cid = 0;
    for (uint8_t i = 0; i < response->data[7]; i++)
    {
        cid <<= 8;
        cid |= response->data[8 + i];
        Serial.print(F(" 0x"));
        Serial.print(response->data[8 + i], HEX);
    }

    return cid;
}


inline boolean PN532::isTargetReleasedError(uint32_t result)
{
   return result == (GEN_ERROR | TARGET_RELEASED_ERROR);
}
*/
