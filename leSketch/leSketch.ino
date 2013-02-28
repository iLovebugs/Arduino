#include <Wire.h>
#include "NDEFPushProtocol.h"
#include "NFCLinkLayer.h"
#include "PN532.h"

uint8_t _irq = 2;
uint8_t _reset = 3;
boolean debug = true;

PN532 nfc(_irq,_reset);
NFCLinkLayer linkLayer(&nfc);
NDEFPushProtocol nppLayer(&linkLayer);

// This message shall be used to rx or tx
// NDEF messages it shall never be released
#define MAX_PKT_HEADER_SIZE 50
#define MAX_PKT_PAYLOAD_SIZE 100
uint8_t rxNDEFMessage[MAX_PKT_HEADER_SIZE + MAX_PKT_PAYLOAD_SIZE];
uint8_t txNDEFMessage[MAX_PKT_HEADER_SIZE + MAX_PKT_PAYLOAD_SIZE];
uint8_t *txNDEFMessagePtr;
uint8_t *rxNDEFMessagePtr;
uint8_t txLen;

void setup(void){

  Serial.begin(9600);
  Serial.println("Hello!");

  uint8_t message[33] = "01234";                           // This is the message that we want to send
  txNDEFMessagePtr = &txNDEFMessage[MAX_PKT_HEADER_SIZE]; //txNDEFMessagePtr now points to first byte of the actual message
  rxNDEFMessagePtr = &rxNDEFMessage[0];                   //rxNDEFMessagePtr now points to the header of rxNDEFMessage[0]
  txLen = createNDEFShortRecord(message, 5, txNDEFMessagePtr);
    
  if (!txLen)
  {
      Serial.println("Failed to create NDEF Message.");
      while(true); //halt
  }
    
  nfc.initializeReader();

}

void loop(){
  Serial.println();
  Serial.println(F("---------------- LOOP ----------------------"));
  Serial.println();
 
 
 //TODO, this method returns weither it was successfull or not
  Serial.println("BEGIN SAMCONFIG.");
  nfc.SAMConfig(debug);
  Serial.println("END SAMCONFIG.");
  
  Serial.println("BEGIN GetFirmwareVersion.");
  delay(5);
  nfc.getFirmwareVersion(debug);
  delay(5);
  Serial.println("END GetFirmwareVersion.");
  
  Serial.println("BEGIN GENERAL STATUS.");
  if(RESULT_SUCCESS == nfc.getGeneralStatus(debug)){
    Serial.println("SUCCESS.");
  }
  delay(5);
  Serial.println("END GENERAL STATUS.");
  
  if(RESULT_SUCCESS == nfc.configurePeerAsTarget(NPP_SERVER, debug))
    Serial.println("Kababu!!!!!!!!!!!!!!!!!");
  delay(5);
  
  Serial.println();
  Serial.println(F("---------------- END LOOP ----------------------"));
  Serial.println();
  
  //Type y in Serial Monitor
  while((char)Serial.read() != 'y' );
 }
