#include <Wire.h>
#include "PN532.h"
#include "NFCLinkLayer.h"
#include "NDEFPushProtocol.h"
#include "MemoryFree.h"

uint8_t wakePin = 2;
uint8_t _irq = 2;
uint8_t _reset = 3;
boolean first = true;

PN532 nfc(_irq,_reset);
NFCLinkLayer linkLayer(&nfc);
NDEFPushProtocol nppLayer(&linkLayer);

// This message shall be used to rx or tx 
// NDEF messages it shall never be released
#define MAX_PKT_HEADER_SIZE  50
#define MAX_PKT_PAYLOAD_SIZE 100
uint8_t rxNDEFMessage[MAX_PKT_HEADER_SIZE + MAX_PKT_PAYLOAD_SIZE];
uint8_t txNDEFMessage[MAX_PKT_HEADER_SIZE + MAX_PKT_PAYLOAD_SIZE];
uint8_t *txNDEFMessagePtr; 
uint8_t *rxNDEFMessagePtr; 
uint8_t txLen;

void setup(void) {
    Serial.begin(9600);
    Serial.println(F("<Setup>"));


    uint8_t message[33] = "01234";
    txNDEFMessagePtr = &txNDEFMessage[MAX_PKT_HEADER_SIZE];
    rxNDEFMessagePtr = &rxNDEFMessage[0]; //This is defined in LOOP, so why here?
    txLen = nppLayer.createNDEFShortRecord(message, 5, txNDEFMessagePtr); //The message has now be created and formated as a NDEF message!    
    
  
  //Prints the NDEF message we try to send!  
    #ifdef PN532DEBUG 
    for(uint8_t i = 0; i < txLen; i++){
      Serial.print(" 0x");
      Serial.print(txNDEFMessagePtr[i], HEX );
    }    
   #endif
   
    Serial.println(F(""));
      
    
    if (!txLen) //When will this ever happen?
    { 
        Serial.println(F("<Setup> Failed to create NDEF Message"));
        while(true); //halt
    }
    
    
    nfc.initializeReader();  //Wire.begin().

    uint32_t versiondata = nfc.getFirmwareVersion();
    if (! versiondata) {
        Serial.print("<Setup> Didn't find PN53x board");
        while (1); // halt
    }

    // set power sleep mode, the deep sleep?
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);

    // configure board to read RFID tags and cards
    nfc.SAMConfig();
    Serial.println(F("configurePeerAsTarget SETUP"));
    nfc.configurePeerAsTarget(NPP_SERVER);
    
    Serial.println(F("<Setup> Minne:"));
    Serial.print(freeMemory());

}

void loop(void) 
{
   Serial.println(F("\n<Loop>\n"));

    uint32_t rxResult = GEN_ERROR; 
    uint32_t txResult = GEN_ERROR;
    rxNDEFMessagePtr = &rxNDEFMessage[0]; //Here or up there? We never get here again anyways!
    
    uint8_t *buf = (uint8_t *) txNDEFMessagePtr;
    
    //Serial.println(F("NDEF Message")); 
    /*for (uint16_t i = 0; i < len; ++i){
        Serial.print(F("0x")); 
        Serial.print(buf[i], HEX);
        Serial.print(F(" "));
    }*/
    
     
    do 
    {
        Serial.print(F("<Loop> Minne:"));
        Serial.println(freeMemory());
        
        rxResult = nppLayer.rxNDEFPayload(rxNDEFMessagePtr);
        if (rxResult == SEND_COMMAND_RX_TIMEOUT_ERROR){
           Serial.println(F("<Loop> SEND_COMMAND_RX_TIMEOUT_ERROR"));
           break;
        }
      

        txResult = nppLayer.pushPayload(txNDEFMessagePtr, txLen);
               
        if (RESULT_OK(rxResult))
        {
           Serial.println(F("<Loop> rxResult"));
           boolean lastPayload;
           uint8_t *ndefTextPayload;
           uint8_t len = nppLayer.retrieveTextPayload(rxNDEFMessagePtr, ndefTextPayload, lastPayload);
           if (len)
           {
               for (uint32_t i = 0; i < len ; ++i)
               {  
                   Serial.print(F("0x")); Serial.print(ndefTextPayload[i], HEX); Serial.print(F(" "));
               }
               Serial.println();
               Serial.println((char *) ndefTextPayload);
           }
           
            txNDEFMessagePtr = &txNDEFMessage[MAX_PKT_HEADER_SIZE];
            rxNDEFMessagePtr = &rxNDEFMessage[0];
            txLen = nppLayer.createNDEFShortRecord(ndefTextPayload, len, txNDEFMessagePtr);    
            
            if (!txLen)
            { 
                Serial.println(F("<Loop> Failed 2 to create NDEF Message"));
                while(true); //halt
            }
        }
        if (txResult == SEND_COMMAND_RX_TIMEOUT_ERROR)
        {
           break;
        }
        Serial.print(F("<Loop> Result: 0x"));
        Serial.print(rxResult, HEX);
        Serial.print(F("<Loop>, 0x"));
        Serial.println(txResult, HEX);  
        Serial.println(F("<Loop> Reaches the end of do-while in LOOP"));      
     } while(true);
  //Type y in Serial Monitor
  while((char)Serial.read() != 'y' );
}
