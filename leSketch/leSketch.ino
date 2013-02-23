#include <Wire.h>
#include "PN532.h"

uint8_t _irq = 2;
uint8_t _reset = 3;

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

  Serial.println("Samconfig");
  nfc.SAMConfig(true);
  
  delay(5);
  Serial.println(nfc.getFirmwareVersion());
  delay(5);
  
  //Lite hybris kanske...
  nfc.configurePeerAsTarget(1);
  delay(5);
  if(digitalRead(_irq));
    Serial.println("ingen data att hämta");
  
  delay(100000);
}
