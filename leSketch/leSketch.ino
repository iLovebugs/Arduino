#include <Wire.h>
#include "PN532.h"

int _irq = 2;
int _reset = 3;

PN532 nfc(_irq,_reset);

void setup(void){

  Serial.begin(9600);
  Serial.println("Hello!");


  pinMode(_irq, INPUT);
  pinMode(_reset, OUTPUT);

  nfc.initializeReader();

}

void loop(){
  Serial.println();
  Serial.println(F("---------------- LOOP ----------------------"));
  Serial.println();

  Serial.println("Samconfig");
  nfc.SAMConfig(true);
  
  if(digitalRead(_irq));
    Serial.println("Hög");
  
  delay(100000);
}
