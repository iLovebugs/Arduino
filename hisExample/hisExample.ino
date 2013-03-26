#include "PN532.h"
#include "NFCLinkLayer.h"
#include "MemoryFree.h"
#include "NDEFPushProtocol.h"
#include "SNEP.h"
#include <avr/power.h>
#include <avr/sleep.h>
/* //UNO
#define SCK 13
#define MOSI 11
#define SS 10
#define MISO 12
*/
// MEGA
#define SCK 52
#define MOSI 51
#define SS 53
#define MISO 50

PN532 nfc(SCK, MISO, MOSI, SS);
NFCLinkLayer linkLayer(&nfc);
SNEP snep(&linkLayer);
//NDEFPushProtocol nppLayer(&linkLayer);

uint32_t retrieveTextPayload(uint8_t *NDEFMessage, uint8_t type, uint8_t *&payload, boolean &lastTextPayload);
uint32_t retrieveTextPayloadFromShortRecord(uint8_t *NDEFMessage, uint8_t *&payload, boolean isIDLenPresent);
uint32_t createNDEFShortRecord(uint8_t *message, uint8_t payloadLen, uint8_t *&NDEFMessage);

// This message shall be used to rx or tx 
// NDEF messages it shall never be released
#define MAX_PKT_HEADER_SIZE  50
#define MAX_PKT_PAYLOAD_SIZE 100
uint8_t rxNDEFMessage[MAX_PKT_HEADER_SIZE + MAX_PKT_PAYLOAD_SIZE];
uint8_t txNDEFMessage[MAX_PKT_HEADER_SIZE + MAX_PKT_PAYLOAD_SIZE];
uint8_t *txNDEFMessagePtr; 
uint8_t *rxNDEFMessagePtr; 
uint8_t txLen;
uint8_t requestType[5];
uint8_t success;
uint8_t *requestType_success;

#define SHORT_RECORD_TYPE_LEN   0x0A
#define NDEF_SHORT_RECORD_MESSAGE_HDR_LEN   0x03 + SHORT_RECORD_TYPE_LEN
#define TYPE_STR "text/plain"

void phoneInRange()
{
  //sleep_disable(); // Prevents the arduino from going to sleep if it was about too. 
}

void setup(void) {
    Serial.begin(9600);
    Serial.println("Hello!");


    uint8_t message[33] = "01234";
    txNDEFMessagePtr = &txNDEFMessage[MAX_PKT_HEADER_SIZE];
    rxNDEFMessagePtr = &rxNDEFMessage[0];
    txLen = createNDEFShortRecord(message, 5, txNDEFMessagePtr);    
    
    if (!txLen)
    { 
        Serial.println("Failed to create NDEF Message.");
        while(true); //halt
    }
    
    
    nfc.initializeReader();

    uint32_t versiondata = nfc.getFirmwareVersion();
    if (! versiondata) {
        Serial.print("Didn't find PN53x board");
        while (1); // halt
    }
    // Got ok data, print it out!
    Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
    Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
    Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
    Serial.print("Supports "); Serial.println(versiondata & 0xFF, HEX);
    
    // set power sleep mode
    set_sleep_mode(SLEEP_MODE_ADC);

    // configure board to read RFID tags and cards
    nfc.SAMConfig();
    //nfc.configurePeerAsTarget(NPP_SERVER);
    attachInterrupt(0, phoneInRange, FALLING);
    
    Serial.print(F("<Setup> Minne:"));
    Serial.println(freeMemory());
}


void loop(void) 
{
   Serial.println();
   Serial.println(F("---------------- LOOP ----------------------"));
   Serial.println();

    uint32_t rxResult = GEN_ERROR; 
    uint32_t txResult = GEN_ERROR;
    rxNDEFMessagePtr = &rxNDEFMessage[0];
    
    /*uint8_t *buf = (uint8_t *) txNDEFMessagePtr;
    Serial.println("NDEF Message"); 
    for (uint16_t i = 0; i < len; ++i)
    {
        Serial.print("0x"); 
        Serial.print(buf[i], HEX);
        Serial.print(" ");
    }
    
    Serial.println();*/
     
    do 
    {
        Serial.println(F("---- Begin Rx Loop ----"));
        //rxResult = nppLayer.rxNDEFPayload(rxNDEFMessagePtr);
        rxResult = snep.receiveRequest(rxNDEFMessagePtr, (uint8_t *&)requestType);
        
        
        
        if (rxResult == SEND_COMMAND_RX_TIMEOUT_ERROR)
        {
           break;
        }
      
        //txResult = nppLayer.pushPayload(txNDEFMessagePtr, txLen);
        //We succesfully recieved the SNEP message, 0x81 indicates success!
        success = SNEP_SUCCESS;
        requestType_success = &success;
        txResult = snep.transmitResponse(txNDEFMessagePtr, txLen, requestType_success); //Why txLen!?
            
        if (RESULT_OK(rxResult))
        {
           Serial.println(F("Rx Packet"));
           boolean lastPayload;
           uint8_t *ndefTextPayload;
           uint8_t len = retrieveTextPayload(rxNDEFMessagePtr, ndefTextPayload, lastPayload);

           if (len) 
           {
               for (uint32_t i = 0; i < len ; ++i)
               {  
                   Serial.print("0x"); Serial.print(ndefTextPayload[i], HEX); Serial.print(" ");
               }
               Serial.println();
               Serial.println((char *) ndefTextPayload);
           }
           
            txNDEFMessagePtr = &txNDEFMessage[MAX_PKT_HEADER_SIZE];
            rxNDEFMessagePtr = &rxNDEFMessage[0]; //Why RX here?
            txLen = createNDEFShortRecord(ndefTextPayload, len, txNDEFMessagePtr);   
           
           //Where do we send the NDEF-message?  
            
            if (!txLen)
            { 
                Serial.println("Failed to create NDEF Message.");
                while(true); //halt
            }
        }
 
        if (txResult == SEND_COMMAND_RX_TIMEOUT_ERROR)
        {
           break;
        }
        Serial.print(F("Result: 0x"));
        Serial.print(rxResult, HEX);
        Serial.print(F(", 0x"));
        Serial.println(txResult, HEX);        
     } while(true);
    
    Serial.print(F("Going to sleep."));
    sleepMCU();
}

uint32_t createNDEFShortRecord(uint8_t *message, uint8_t payloadLen, uint8_t *&NDEFMessage)
{
   //Serial.print("Message: ");
   //Serial.println((char *)message);
   uint8_t * NDEFMessageHdr = ALLOCATE_HEADER_SPACE(NDEFMessage, NDEF_SHORT_RECORD_MESSAGE_HDR_LEN);
   
   NDEFMessageHdr[0] =  NDEF_MESSAGE_BEGIN_FLAG | NDEF_MESSAGE_END_FLAG | NDEF_MESSAGE_SHORT_RECORD | TYPE_FORMAT_MEDIA_TYPE; 
   NDEFMessageHdr[1] =  SHORT_RECORD_TYPE_LEN;
   NDEFMessageHdr[2] =  payloadLen;
   memcpy(&NDEFMessageHdr[3], TYPE_STR, SHORT_RECORD_TYPE_LEN);
   memcpy(NDEFMessage, message, payloadLen);
   //Serial.print("NDEF Message: ");
   //Serial.println((char *)NDEFMessage);   
   NDEFMessage = NDEFMessageHdr;
   return (payloadLen + NDEF_SHORT_RECORD_MESSAGE_HDR_LEN);   
}


//This method checks the NDEF header. The header MUST be of MEDIA_TYPE else the NDEF-message will be discarded.
//Will call methods that extracts information from short record or normal records. Only the short record method is implemented.
uint32_t retrieveTextPayload(uint8_t *NDEFMessage, uint8_t *&payload, boolean &lastTextPayload)
{
   Serial.println(NDEFMessage[0], HEX);
   Serial.println(NDEFMessage[1], HEX);
   uint8_t type = (NDEFMessage[0] & NDEF_MESSAGE_TYPENAME_FORMAT); //applying a mask 0000 0111 to extract the TNF field.
   if (type != TYPE_FORMAT_NFC_FORUM_TYPE && type != TYPE_FORMAT_MEDIA_TYPE) //If result is not 0000 0010 = mediatype we will not process the NDEF-message
   {
      return 0;
   }
   
   lastTextPayload = (NDEFMessage[0] & NDEF_MESSAGE_END_FLAG); //When is this used? Checks wether this is the last record by applying mask 0100 0000.
   
   if (NDEFMessage[0] & NDEF_MESSAGE_SHORT_RECORD) //Checks if SR flag is set, if so extract information from the payload.    
   {
      retrieveTextPayloadFromShortRecord( NDEFMessage, type, payload, NDEFMessage[0] & NDEF_MESSAGE_ID_LENGTH_PRESENT);    
   }
   else 
   {
      //@TODO: We could remove this part, our application will not use non SR NDEF-messages
   }
}



//This function parses the ShortRecord, extracting the information.
uint32_t retrieveTextPayloadFromShortRecord(uint8_t *NDEFMessage, uint8_t type, uint8_t *&payload, boolean isIDLenPresent)
{
   NDEFState currentState =  NDEF_TYPE_LEN; //This is the first field after the NDEF header.
   payload = NULL;
   
   uint8_t typeLen;
   uint8_t idLen;
   uint8_t payloadLen;
   uint8_t idx = 1;
   
   while (currentState != NDEF_FINISHED)
   {
      switch (currentState) 
      {
          case NDEF_TYPE_LEN:
          {
             typeLen = NDEFMessage[idx++]; //Saves the typeLen for future use
             currentState = NDEF_PAYLOAD_LEN;
          }
          // Purposefully allowing it to fall through
          case NDEF_PAYLOAD_LEN:
          {
             payloadLen = NDEFMessage[idx++]; //Saves the payload for later extraction
             currentState =  isIDLenPresent ? NDEF_ID_LEN : NDEF_TYPE; //Checks whether ID field is present, if not advance to NDEF_TYPE, else advance to the NDEF_ID_LEN.
          }
          break; //Break?
          case NDEF_ID_LEN:
          {
             idLen = NDEFMessage[idx++];
             currentState = NDEF_TYPE;
          }
          // Purposefully allowing it to fall through
          case NDEF_TYPE:
          {
             if (type == TYPE_FORMAT_NFC_FORUM_TYPE )
             {
                 for (uint8_t i = 0; i < (typeLen - 1); ++i) {
                    if (NDEFMessage[idx++] != 0x00)
                    {
                       Serial.println("Unhandled NDEF Message Type.");
                       return 0;
                    }
                 }
                 
                 if ( NDEFMessage[idx++] != NFC_FORUM_TEXT_TYPE ) 
                 {
                     Serial.println("Unhandled NDEF Message Type.");
                     return 0;
                 }
             }
             else if (type == TYPE_FORMAT_MEDIA_TYPE) //This will be the our case. We must check if len is ok and if the value is text/plain
             { 
                 const char *typeStr =  (char *)&NDEFMessage[idx];
                 if (typeLen != 0xA || strncmp(typeStr, "text/plain", typeLen) != 0) 
                 {
                    NDEFMessage[typeLen + idx] = NULL;
                    Serial.print("Unknown Type: ");
                    Serial.println(NDEFMessage[idx]);
                    return 0;
                 }
                 idx += typeLen; //Confirmed the mediatype to be text/plain, advance idx to the actuall payload.
                 
             }
             else 
             {
                 return 0;
             }
             currentState =  isIDLenPresent ? NDEF_ID : NDEF_PAYLOAD;
          }
          break;
          case NDEF_ID:
          {
              // Skip over the ID field
              idx += idLen;
              currentState = NDEF_PAYLOAD;
          }
          // Purposefully allowing it to fall through
          case NDEF_PAYLOAD:
          {
              payload = &NDEFMessage[idx]; //Succesfully extracted the payload from the SR.
              payload[payloadLen] = '\0';  
              currentState = NDEF_FINISHED;
          }
          break;
          default:
            currentState = NDEF_FINISHED;
      }
   }
   
   return (payload != NULL) ? payloadLen : 0;
}


void sleepMCU()
{
    delay(100);  // delay so that debug message can be printed before the MCU goes to sleep
    
    // Enable sleep mode
    sleep_enable();          
    
    power_adc_disable();
    power_spi_disable();
    power_timer0_disable();
    power_timer1_disable();
    power_timer2_disable();
    power_twi_disable();
    
    //Serial.println("Going to Sleep\n");
    //delay(1000);
    
    // Puts the device to sleep.
    sleep_mode();  
    Serial.println("Woke up");          
    
    // Program continues execution HERE
    // when an interrupt is recieved.

    // Disable sleep mode
    sleep_disable();         
    
    power_all_enable();
}   
   
