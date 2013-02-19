#include <Wire.h>
#include "test.h"

//Buffern som allt läggs på, rymmer ett standard PN532 commando
uint8_t pn532_packetbuffer[265];
int _irq = 2;
int _reset = 3;


void setup(void){

  Serial.begin(9600);
  Serial.println("Hello!");


  pinMode(_irq, INPUT);
  pinMode(_reset, OUTPUT);

  initializeReader();

}

void loop(){
  Serial.println();
  Serial.println(F("---------------- LOOP ----------------------"));
  Serial.println();

  Serial.println("Samconfig");
  SAMConfig();
  
  delay(100000);




}


void initializeReader() 
{
  //Pinnarna som ställer in vilket interfacelage som skall köras är antagligen konstanta.
  Wire.begin();				// Hoppar med på I2C bussen som "master"
  digitalWrite(_reset, HIGH);             // skriva till reset verar vara överflödig.... ? 
  digitalWrite(_reset, LOW);
  delay(400); 
  digitalWrite(_reset, HIGH);

}

void SAMConfig() 
{
  pn532_packetbuffer[0] = PN532_SAMCONFIGURATION;
  pn532_packetbuffer[1] = 0x01; // normal mode;
  pn532_packetbuffer[2] = 0x14; // timeout 50ms * 20 = 1 second
  pn532_packetbuffer[3] = 0x01; // use IRQ pin!

  wiresendcommand(pn532_packetbuffer, 4, true);
  // read data packet
}


static inline uint8_t wirerecv()
{
  return Wire.read();
}

static inline void wiresend(uint8_t x)
{
  //Vi har Arduino version 1.0, dvs Arduino >= 100
  //Lägger x på I2C kön...
  Wire.write((uint8_t)x);
}

void wiresendcommand(uint8_t* cmd, uint8_t cmdlen, boolean debug) 
{
  uint8_t checksum;
  cmdlen++;

  //Skriver ut på terminalen
  if (debug) 
  {
    Serial.print(F("\nSending: "));
  }

  delay(2);     // or whatever the delay is for waking up the board    
  Wire.beginTransmission(PN532_I2C_ADDRESS);


  checksum = PN532_PREAMBLE + PN532_PREAMBLE + PN532_STARTCODE2; //Skall dessa verkligen vara med? se p28 UM

  //Mycket riktigt är det 00x x00 xFF som är startsekvensen för PN532
  wiresend(PN532_PREAMBLE);
  wiresend(PN532_PREAMBLE);
  wiresend(PN532_STARTCODE2);

  wiresend(cmdlen);
  uint8_t cmdlen_1=~cmdlen + 1;
  wiresend(cmdlen_1);		//tvåkompleterar cmdlen, används som LCS length checksum
  wiresend(PN532_HOSTTOPN532);	//TFI specificeras som från host till PN532 (0xD4)

  checksum += PN532_HOSTTOPN532; //Checksumen ökas...

  if (debug) 
  {
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
  }


  //Denna loopen står för skickadet av själva kommandot dvs PD0...PDN
  for (uint8_t i=0; i<cmdlen-1; i++) {
    wiresend(cmd[i]);
    checksum += cmd[i];  

    if (debug) 
    {
      Serial.print(F(" 0x")); 
      Serial.print(cmd[i], HEX);
    }
  }

  uint8_t checksum_1=~checksum;
  wiresend(checksum_1); //Varför invereras bara detta? Hur vet man att den verklige uppfyller kravet
  wiresend(PN532_POSTAMBLE); 


  if (debug) 
  {
    Serial.print(F(" 0x")); 
    Serial.print(checksum_1, HEX);
    Serial.print(F(" 0x")); 
    Serial.print(PN532_POSTAMBLE, HEX);
    Serial.println();
  }
  Wire.endTransmission(); // Nu datan faktiskt sänds iväg
} 



