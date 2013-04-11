// PN532 library by adafruit/ladyada
// MIT license

// authenticateBlock, readMemoryBlock, writeMemoryBlock contributed
// by Seeed Technology Inc (www.seeedstudio.com)

#include "PN532.h"
#include <Wire.h>

#define PN532DEBUG
#define PN532PRINTRESPONSE
#define getFirmwareVersionDEBUG
#define sendFrameDEBUG
//#define fetchCheckAckDEBUG
//#define fetchdataDEBUG

uint8_t pn532ack[] = {
  0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
uint8_t pn532error[] = {
  0x00, 0x00, 0xFF, 0x01, 0xFF, 0x7F, 0x81, 0x00};
uint8_t pn532response_firmwarevers[] = {
  0x00, 0xFF, 0x06, 0xFA, 0xD5, 0x03};

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
        Serial.println(F("<SAMconfig>"));
      #endif
    
      pn532_packetbuffer[0] = PN532_SAMCONFIGURATION;
      pn532_packetbuffer[1] = 0x01; // normal mode;
      pn532_packetbuffer[2] = 0x14; // timeout 50ms * 20 = 1 second
      pn532_packetbuffer[3] = 0x01; // use IRQ pin!
    
      uint32_t result = sendCommandCheckAck(pn532_packetbuffer, 4, 1000);
    
      if (IS_ERROR(result)) 
      {
        #ifdef PN532DEBUG
          Serial.println(F("SAMConfig: SEND_COMMAND_TX_TIMEOUT_ERROR"));
        #endif
        return result;
      }
    
      // read data packet
      PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
      return fetchResponse(PN532_SAMCONFIGURATION, response);
    }

    uint32_t PN532::getFirmwareVersion() 
    {
      #ifdef PN532DEBUG
        Serial.println(F("<getFirmwareVersion>"));
      #endif
      
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
        #ifdef PN532DEBUG
          Serial.print(F("Didn't find PN53x board"));
        #endif  
        while (1); // halt
      }
      #ifdef getFirmwareVersionDEBUG
       Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
       Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
       Serial.print('.'); 
       Serial.println((versiondata>>8) & 0xFF, DEC);
       Serial.print("Supports "); Serial.println(versiondata & 0xFF, HEX); 
      #endif
      
      return versiondata;   
    }

uint32_t PN532::getGeneralStatus(){

  pn532_packetbuffer[0] = PN532_GETGENERALSTATUS;

  uint32_t result = sendCommandCheckAck(pn532_packetbuffer, 1, 1000);

  if (IS_ERROR(result)) 
  {
    Serial.println(F("getGeneralStatus: SEND_COMMAND_TX_TIMEOUT_ERROR"));
    return result;
  }

  // read data packet
  PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
  return fetchResponse(PN532_SAMCONFIGURATION, response);
}

uint32_t PN532::configurePeerAsTarget(uint8_t type)
{
  #ifdef PN532DEBUG
  Serial.println(F("<configurePeerAsTarget>"));
  #endif
//  Serial.println(F("<configurePeerAsTarget>"));
//  static const uint8_t npp_client[44] = { 
//    PN532_TGINITASTARGET,
//    0x00, 
//    0x00, 0x00, //SENS_RES
//    0x00, 0x00, 0x00, //NFCID1
//    0x00, //SEL_RES
//
//    0x01, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // POL_RES
//    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//
//    0x00, 0x00,
//
//    0x01, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //NFCID3t: Change this to desired value
//
//    0x06, 0x46, 0x66, 0x6D, 0x01, 0x01, 0x10, 0x00
//  };
//  
//  static const uint8_t npp_server[44] = { 
//    PN532_TGINITASTARGET,
//    0x01,
//    0x00, 0x00, //SENS_RES
//    0x00, 0x00, 0x00, //NFCID1
//    0x40, //SEL_RES
//
//    0x01, 0xFE, 0x0F, 0xBB, 0xBA, 0xA6, 0xC9, 0x89, // POL_RES
//    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//
//    0xFF, 0xFF,
//
//    0x01, 0xFE, 0x0F, 0xBB, 0xBA, 0xA6, 0xC9, 0x89, 0x00, 0x00, //NFCID3t: Change this to desired value
//
//    0x06, 0x46, 0x66, 0x6D, 0x01, 0x01, 0x10, 0x00
//    
//  }; 
    int pdu_len = 51;
       static const uint8_t npp_client[51] = {PN532_TGINITASTARGET,
                                (byte) 0x00,   //MODE
                                    //MIFARE PARAMS	
    	       			(byte) 0x08, (byte) 0x00,  // SENS_RES
                                (byte) 0x12, (byte) 0x34, (byte) 0x56, // NFCID1
                                (byte) 0x40, // SEL_RES
                                    //FELICA PARAMS
    	       			(byte) 0x01, (byte) 0xfe, (byte) 0xa2, (byte) 0xa3, (byte) 0xa4, (byte) 0xa5, // POL_RES
    	       			(byte) 0xa6, (byte) 0xa7, (byte) 0xc0, (byte) 0xc1, (byte) 0xc2, (byte) 0xc3, 
    	       			(byte) 0xc4, (byte) 0xc5, (byte) 0xc6, (byte) 0xc7,
                                (byte) 0xff, (byte) 0xff,
                                    // NFCID3
    	       			(byte) 0xaa, (byte) 0x99, (byte) 0x88, (byte) 0x77, (byte) 0x66, 
    	       			(byte) 0x55, (byte) 0x44, (byte) 0x33, (byte) 0x22, (byte) 0x11, 	
    	       			(byte) 0x0d, //LEN Gt
    	       			(byte) 0x46, (byte) 0x66, (byte) 0x6D, //LLCP WORD 	       			
    	       			(byte) 0x01, (byte) 0x01, (byte) 0x11, //VERSION NUMBER
    	       			(byte) 0x03, (byte) 0x02, (byte) 0x00, (byte) 0x01, //WELL KNOWN SERVICE LIST
    	       			(byte) 0x04, (byte) 0x01, (byte) 0xFF, //LINK TIMEOUT
                                (byte) 0x00}; // LEN ?


  if (type == NPP_CLIENT)
  {
    for(uint8_t iter = 0;iter < pdu_len;iter++)
    {
      pn532_packetbuffer[iter] = npp_client[iter];
    }
  }
//  else if (type == NPP_SERVER)
//  {
//    for(uint8_t iter = 0;iter < pdu_len;iter++)
//    {
//      pn532_packetbuffer[iter] = npp_server[iter];
//    }
//  }

  uint32_t result;
  result = sendCommandCheckAck(pn532_packetbuffer, pdu_len, 2000);

  if (IS_ERROR(result))
  {
    return result;
  }

  // sleepArduino(); // Sleep until phone wakes up the PN532

  PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;

  return fetchResponse(PN532_TGINITASTARGET, response);  

}

uint32_t PN532::targetRxData(uint8_t *DataIn)
{
  #ifdef PN532DEBUG
  Serial.println(F("<targetRxData>"));
  #endif
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
  //response->printResponse();

  if (IS_ERROR(result))
  {
    return NFC_READER_RESPONSE_FAILURE;
  }

  if (response->data[0] == 0x00) // börjar NDEF message med 0x00?
  {
    uint32_t ret_len = response->len - 1; // ändrat
    memcpy(DataIn, &(response->data[1]), ret_len);
    return ret_len;
  }

  return (GEN_ERROR | response->data[0]);
}

uint32_t PN532::targetTxData(uint8_t *DataOut, uint32_t dataSize)
    {
      #ifdef PN532DEBUG
        Serial.println(F("<targetTxData>"));
    Serial.print(F("<targetTxData> Minne:"));
    Serial.println(freeMemory());
      #endif
      
      ///////////////////////////////////// Sending to Initiator ///////////////////////////
      pn532_packetbuffer[0] = PN532_TGSETDATA;
      uint8_t commandBufferSize = (1 + dataSize);
      for(uint8_t iter=(1+0);iter < commandBufferSize; ++iter)
      {
        pn532_packetbuffer[iter] = DataOut[iter-1]; //pack the data to send to target
      }
    
      uint32_t result = sendCommandCheckAck(pn532_packetbuffer, commandBufferSize, 1000);
      if (IS_ERROR(result)) {
        #ifdef PN532DEBUG
          Serial.println(F("TX_Target Command Failed."));
        #endif
        
        return result;
      }
    
    
      // read data packet
      PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    
      result = fetchResponse(PN532_TGSETDATA, response);
      if (IS_ERROR(result))
      {
        #ifdef PN532DEBUG
          Serial.println(F("TX_Target Command Response Failed."));
        #endif
        return result;
      }
    
      #ifdef PN532PRINTRESPONSE
        response->printResponse();
      #endif
    
      if (response->data[0] != 0x00)
      {
        #ifdef PN532DEBUG
          Serial.println(F("Gen Error!."));
        #endif
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
  
  //Never EVER touch this!
  if(debug)
    Serial.println(F("<sendCommandCheckAck>"));

  #ifdef PN532DEBUG  
    Serial.print(F("<sendCommandCheckAck> Minne:"));
    Serial.println(freeMemory());
  #endif 
  // send the command frame
  sendFrame(cmd, cmdlen);

  // Wait for chip to say it has data to send+

  // read acknowledgement
  if (!fetchCheckAck(timeout)) {
    return SEND_COMMAND_RX_ACK_ERROR;
    Serial.println(F("<SendcommandCheckAck>: ACK error")); 
  }

#ifdef PN532DEBUG       
  Serial.println(F("<sendCommandCheckAck>: PN532 Successfully recieved host command"));
#endif  
  return RESULT_SUCCESS; // ack'd command
}

void PN532::sendFrame(uint8_t* cmd, uint8_t cmdlen)
    {
      #ifdef PN532DEBUG
        Serial.println(F("<sendFrame>"));
      #endif
      
      uint8_t checksum;
    
      //Increasing the cmdlen to include the TFI (Direction)
      cmdlen++;
 
      //delay(20);                               // or whatever the delay is for waking up the board  
    
      Wire.beginTransmission(PN532_I2C_ADDRESS);
    
      // Make whole frame  
      Wire.write(PN532_PREAMBLE);
      Wire.write(PN532_STARTCODE1);
      Wire.write(PN532_STARTCODE2);
    
    #ifdef sendFrameDEBUG
        Serial.println(F("<sendFrame> between 2>"));
    Serial.print(F("<sendFrame> Minne:"));
    Serial.println(freeMemory());
    #endif
      Wire.write(cmdlen);                   //LEN
      uint8_t cmdlen_1 = ~cmdlen + 1;       //calculating the two's complement of cmdlen is used as the LCS
      Wire.write(cmdlen_1);		        //LCS        
      Wire.write(PN532_HOSTTOPN532);        //TFI 
    
      checksum += PN532_HOSTTOPN532;         
      
    
    #ifdef sendFrameDEBUG
      Serial.print(F("<sendFrame>: Sending: "));
      Serial.print(F(" 0x")); 
      Serial.print(PN532_PREAMBLE, HEX);
      Serial.print(F(" 0x")); 
      Serial.print(PN532_STARTCODE1, HEX);
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
    
    #ifdef sendFrameDEBUG
        Serial.print(F(" 0x")); 
        Serial.print(cmd[i], HEX);
    #endif
      }
    
      uint8_t checksum_1= ~checksum + 1;      
    
      Wire.write(checksum_1);             //DCS
      Wire.write(PN532_POSTAMBLE); 
    
    
    #ifdef sendFrameDEBUG     
      Serial.print(F(" 0x")); 
      Serial.print(checksum_1, HEX);
      Serial.print(F(" 0x")); 
      Serial.print(PN532_POSTAMBLE, HEX);
      Serial.println();
      Serial.println(F("<sendFrame>: Command sent"));
    #endif
    
      Wire.endTransmission(); // Nu datan faktiskt sänds iväg     
    }
    
boolean PN532::fetchCheckAck(uint16_t timeout)
    {
      #ifdef PN532DEBUG
        Serial.println(F("<fetchCheckAck>"));
      #endif
      
      //Create buffer of size 6 to recieve an ack
      uint8_t ackbuff[6];
    
    
      uint16_t timer = 0;
      //Wait timeout for PN532 to send the ack, else timeout
      while ((checkDataAvailable()) == PN532_I2C_NO_DATA){
        delay(10);
        timer+=10;
    
        if (timer > timeout){
          
          #ifdef fetchCheckAckDEBUG
            Serial.println(F("<fetchCheckAck>: Timeout"));
          #endif
          
          return 0;
        }
      }
    
      Wire.requestFrom((uint8_t)PN532_I2C_ADDRESS, (uint8_t)7);
      Wire.read();//take away rubbish due to I2C delay
    
    #ifdef fetchCheckAckDEBUG
      Serial.println(F("<fetchCheckAck>: Data recieved from PN532:  "));
    #endif  
      for (uint16_t i=0; i<7; i++){
        delay(1);
        ackbuff[i] = Wire.read();
    
    #ifdef fetchCheckAckDEBUG    
    
        Serial.print(F(" 0x"));
        Serial.print(ackbuff[i], HEX);            
    #endif
      }
    
    #ifdef fetchCheckAckDEBUG
      Serial.println();   
      if(0 == strncmp((char *)ackbuff, (char *)pn532ack, 6))
        Serial.println(F("<fetchCheckAck>: Ack recieved"));
      Serial.println();
    #endif
    
      return (0 == strncmp((char *)ackbuff, (char *)pn532ack, 6));
    }


//Bryter isär svaret fr�n PN532 
uint32_t PN532::fetchResponse(uint8_t cmdCode, PN532_CMD_RESPONSE *response) 
    {
    
      #ifdef PN532DEBUG
      Serial.println(F("<fetchResponse>"));
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
    
    
    #ifdef PN532PRINTRESPONSE
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
      Serial.println(F("<fetchData>"));
    #endif
    
      //TODO add a timeout
      uint16_t time = 0;
      while(checkDataAvailable() == PN532_I2C_NO_DATA){        
    //    Serial.println(F("<fetchData>: Nothing to fetch, delay 250ms"));
        delay(250);
      }
    
      //Fetches a predefined Chunk of data.
      //Will continue to fetch chuks aslong as PN532 has something to send
      while(checkDataAvailable() == PN532_I2C_DATA_TO_FETCH){
        #ifdef fetchdataDEBUG
          Serial.println(F("<fetchData>: Fetching new chunk of data"));
        #endif
        Wire.requestFrom((uint8_t)PN532_I2C_ADDRESS, (uint8_t)(CHUNK_OF_DATA+1)); //öka?
        delay(10);
        Wire.read();//take away rubbish due to I2C delay
      }
    
      //Changed from n = argument set at 150 to actual size of read data..    
      uint8_t n = Wire.available();
    
    #ifdef fetchdataDEBUG        
      Serial.print(F("<fetchData>: "));
      Serial.print(n);
      Serial.println(F(" bytes fetched"));
      Serial.println(F("<fetchData>: Data recieved from PN532: "));
    #endif
    
      for (uint16_t i=0; i<n; i++){
        delay(1);
        buff[i] = Wire.read();
    
    #ifdef fetchdataDEBUG           
        Serial.print(F(" 0x"));
        Serial.print(buff[i], HEX);
    #endif
      }
    
    #ifdef fetchdataDEBUG
      Serial.println();
      Serial.println(F("<fetchData>: data fetched"));
    #endif      
    }

/************** low level functions ***********/

// Puts Arduino to sleep
void PN532::sleepArduino()
{    
  // Enable sleep mode
  sleep_enable();

  Serial.println(F("Going to Sleep\n"));
  delay(100); // delay so that debug message can be printed before the MCU goes to sleep

  // Puts the device to sleep.
  sleep_mode();


  // Program continues execution HERE
  // when an interrupt is recieved.
  Serial.println(F("Woke up"));
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
#ifdef PN532PRINTRESPONSE
  Serial.println();
  Serial.println(F("<fetchResponse>: Response"));
  Serial.print(F("Preamble: 0x"));
  Serial.println(preamble, HEX);
  Serial.print(F("Startcode: 0x"));
  Serial.print(header[0], HEX);
  Serial.print(F(" 0x"));
  Serial.println(header[1], HEX);
  Serial.print(F("Len: 0x"));
  Serial.println(len, HEX);
  Serial.print(F("LCS: 0x"));
  Serial.println(len_chksum, HEX);
  Serial.print(F("Direction: 0x"));
  Serial.println(direction, HEX);
  Serial.print(F("Response Command: 0x"));
  Serial.println(responseCode, HEX);

  Serial.println(F("Data: "));

  uint8_t i;
  for (i = 0; i < len-2; ++i){
    Serial.print(F("0x"));
    Serial.print(data[i], HEX);
    Serial.print(F(" "));
  } 

  Serial.println();
  Serial.print(F("Returned Checksum: 0x"));
  Serial.println(data[i++], HEX); 
  Serial.print(F("Postamble: 0x"));   
  Serial.println(data[i], HEX);
#endif     
}

  inline boolean PN532::isTargetReleasedError(uint32_t result)
  {
  return result == (GEN_ERROR | TARGET_RELEASED_ERROR);
  }
 

