#include <Wire.h>
#include "PN532.h"

uint8_t _irq = 2;
uint8_t _reset = 3;
boolean debug = true;

PN532 nfc(_irq,_reset);

void setup(void){

  Serial.begin(9600);
  Serial.println("Hello!");

  
  nfc.initializeReader();

}

void loop(){
  Serial.println();
  Serial.println(F("---------------- LOOP ----------------------"));
  Serial.println();
 
 
 //TODO, this method returns weither it was successfull or not
  nfc.SAMConfig(debug);
  
  delay(5);
  nfc.getFirmwareVersion(debug);
  delay(5);
  
  if(RESULT_SUCCESS == nfc.configurePeerAsTarget(NPP_SERVER, debug))
    Serial.println("Kababu!!!!!!!!!!!!!!!!!");
  delay(5);
  
    
  Serial.println();
  Serial.println(F("---------------- END LOOP ----------------------"));
  Serial.println();
  
  //Type y in Serial Monitor
  while((char)Serial.read() != 'y' );
 }
