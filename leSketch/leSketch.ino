#include <Wire.h>
#include "NDEFPushProtocol.h"
#include "NFCLinkLayer.h"
#include "PN532.h"
#include "MemoryFree.h";

uint8_t wakePin = 2;
uint8_t _irq = 2;
uint8_t _reset = 3;
boolean first = true;

PN532 nfc(_irq,_reset);
NFCLinkLayer linkLayer(&nfc);
NDEFPushProtocol nppLayer(&linkLayer);

// This message shall be used to rx or tx
// NDEF messages it shall never be released
#define MAX_PKT_HEADER_SIZE 50
#define MAX_PKT_PAYLOAD_SIZE 100
uint8_t rxNDEFMessage[MAX_PKT_HEADER_SIZE + MAX_PKT_PAYLOAD_SIZE];
uint8_t txNDEFMessage[MAX_PKT_HEADER_SIZE + MAX_PKT_PAYLOAD_SIZE];
uint8_t *txNDEFMessagePtr = &txNDEFMessage[MAX_PKT_HEADER_SIZE]; //txNDEFMessagePtr now points to first byte of the actual message
uint8_t *rxNDEFMessagePtr = &rxNDEFMessage[0]; //rxNDEFMessagePtr now points to the header of rxNDEFMessage[0]
uint8_t txLen;  
uint8_t message[33] = "01234";  // This is the message that we want to send
 //txNDEFMessagePtr = &txNDEFMessage[MAX_PKT_HEADER_SIZE]; 
 //rxNDEFMessagePtr = &rxNDEFMessage[0]; 



void setup(void){
  
  Serial.begin(9600);
  Serial.println("Hello!");
  
  // set power sleep mode, wakes on IRQ from PN532
  pinMode(wakePin, INPUT);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  attachInterrupt(0, wakeUpFunction, FALLING);
  
    
  nfc.initializeReader();
  Serial.println("Minne:");
  Serial.println(freeMemory());
  Serial.println("Minne:");
  Serial.println(availableMemory());
}
void loop(){
    
  Serial.println();
  Serial.println(F("---------------- LOOP ----------------------"));
  Serial.println();

 
 //TODO, this method returns weither it was successfull or not
  Serial.println("BEGIN SAMCONFIG.");
  nfc.SAMConfig();
  Serial.println("END SAMCONFIG.");
  
  Serial.println("BEGIN GetFirmwareVersion.");
  delay(5);
  nfc.getFirmwareVersion();
  delay(5);
  Serial.println("END GetFirmwareVersion.");
  
  Serial.println("BEGIN GENERAL STATUS.");
  if(RESULT_SUCCESS == nfc.getGeneralStatus()){
    Serial.println("SUCCESS.");
  }
  delay(5);
  Serial.println("END GENERAL STATUS.");
  
  if(RESULT_SUCCESS == nfc.configurePeerAsTarget(NPP_SERVER))
    Serial.println("Back in Loop");
  delay(5);
  
    
 
 if(first){

 txLen = nppLayer.createNDEFShortRecord(message, 5, txNDEFMessagePtr);
  
  if (!txLen)
  {
      Serial.println("Failed to create NDEF Message.");
      while(true); //halt
  }
  
    Serial.println("Created NDEF Message:");
    for(uint8_t i = 0; i < (MAX_PKT_HEADER_SIZE + MAX_PKT_PAYLOAD_SIZE); i++){
    Serial.print(F(" 0x")); 
    Serial.print(txNDEFMessage[i], HEX);
  }
  
  first = false;
   
 }
  
  
  
  Serial.println();
  Serial.println(F("---------------- END LOOP ----------------------"));
  Serial.println();
  
  //Type y in Serial Monitor
  while((char)Serial.read() != 'y' );
 }
 
void wakeUpFunction(){
  
}
