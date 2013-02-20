#include <Wire.h>
#include "test.h"
#include <Arduino.h>
#include <stdint.h>

//Buffern som allt läggs på, rymmer ett standard PN532 commando
uint8_t pn532_packetbuffer[265];
uint8_t pn532ack[] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};


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
  
  if(digitalRead(_irq));
    Serial.println("Hög");
  
  
  delay(100000);

}

void initializeReader() 
{
                                        //Pinnarna som ställer in vilket interfacelage som skall köras är antagligen konstanta.
  Wire.begin();				// Hoppar med på I2C bussen som "master"
  digitalWrite(_reset, HIGH);           // skriva till reset verar vara överflödig.... ? 
  digitalWrite(_reset, LOW);
  delay(400); 
  digitalWrite(_reset, HIGH);

}

void SAMConfig(void) 
{
    pn532_packetbuffer[0] = PN532_SAMCONFIGURATION;
    pn532_packetbuffer[1] = 0x01; // normal mode;
    pn532_packetbuffer[2] = 0x14; // timeout 50ms * 20 = 1 second
    pn532_packetbuffer[3] = 0x01; // use IRQ pin!
    
    uint32_t result = sendCommandCheckAck(pn532_packetbuffer, 4, 1000, true); //ture = debugg!, 1000 är timeout. Är fördefinerade i .h

    
   /*
    if (IS_ERROR(result)) 
    {
        return result;
    }
    */

    // read data packet
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    readwirecommand(PN532_SAMCONFIGURATION, response,true); 
    
    
}

//Portad!! (Snart)
uint32_t sendCommandCheckAck(uint8_t *cmd, 
                                    uint8_t cmdlen, 
                                    uint16_t timeout, 
                                    boolean debug) 
{
    uint16_t timer = 0;
    Serial.println("sendCommandCheckAck");
    
        // write the command
    wiresendcommand(cmd, cmdlen, debug);
    
    // Wait for chip to say its ready!
    while ((wirereadstatus()) != PN532_I2C_READY) //Här pollas datalinjen efter Statusbit!
    {
        if (timeout != 0) 
        {
            timer+=10;
            if (timer > timeout) 
            {
                Serial.println("Timeout");
                return SEND_COMMAND_TX_TIMEOUT_ERROR;                 
            }
        }
        delay(10);
    }
    
    Serial.println("Halfway through");
    
    // read acknowledgement
    if (!wire_readack(debug)) {
        return SEND_COMMAND_RX_ACK_ERROR;
        Serial.println("ACK error"); 
    }
    
    timer = 0;
    
    // Wait for chip to say its ready!
    while (wirereadstatus() != PN532_I2C_READY) //Dessa måste fixas!!!!!! 
    {
        if (timeout != 0) 
        {
            timer+=10;
            if (timer > timeout) 
            {
                return SEND_COMMAND_RX_TIMEOUT_ERROR;
            }
        }
        delay(10);
    }
    
    Serial.println("Great success!");
    return RESULT_SUCCESS; // ack'd command
}


void wirereaddata(uint8_t* buff, uint32_t n, boolean debug) 
{    
    delay(2);
    
    
    if (debug) 
    {
        Serial.println(F("wirereaddata"));
    }
    
    Wire.requestFrom((uint8_t)PN532_I2C_ADDRESS, (uint8_t)(n+1)); //Begär data från PN532, +2 för att?
    
    Serial.println("successful request");
    wirerecv();//testing delay
    Serial.println("snopp");
    // Vänta tills alla bytes har kommit in
    
    Serial.println(Wire.available());
    
    while(Wire.available() != (int)n){ // Vet ej om type-casten fungerar
      delay(1);
      Serial.println("I2C not avilable");
    }
    
    Serial.println("I2C avilable");
    
    for (uint8_t i=0; i<n; i++){
        delay(1);
        buff[i] = wirerecv();
        
        if (debug)
        {
            Serial.print(F(" 0x"));
            Serial.print(buff[i], HEX);
        }
    }

    if (debug)
    {
        Serial.println();
    }   
}



//Ersätter helt readspiStatus, i I2C räcker det med att titta på IRQ linan för att avögra om PN532 är upptagen. What? /jonas
uint8_t wirereadstatus(void) {
  uint8_t x = digitalRead(_irq);
  
  if (x == 1)    
    return PN532_I2C_BUSY;
  else
    return PN532_I2C_READY;
    
}

//Behövs en speciell metod för att ackframen är annorlunda...
boolean wire_readack(boolean debug) 
{
    //Acken är 6 bytes lång, den är specialuntformad.
    uint8_t ackbuff[6];

    wirereaddata(ackbuff, 6, debug);

    return (0 == strncmp((char *)ackbuff, (char *)pn532ack, 6));
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
  wiresend(PN532_PREAMBLE); //Egetnligen start code 1!!!!
  wiresend(PN532_STARTCODE2);

  wiresend(cmdlen); //LEN
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

  uint8_t checksum_1= ~checksum;
  
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
  
  Serial.println("wiresendcommand done");
} 

uint32_t readwirecommand(uint8_t cmdCode, PN532_CMD_RESPONSE *response, boolean debug) 
{

    uint8_t calc_checksum = 0;
    uint8_t ret_checksum;
        
    //Begär data från PN532    
    Wire.requestFrom((uint8_t)PN532_I2C_ADDRESS, (unit8_t)(n+2));
   
    delay(2);
   
    
    // Wattafack?
    response->header[0] = response->header[1] = 0xAA;
    
    uint32_t retVal = RESULT_SUCCESS;
    
    do 
    {
       response->header[0] = response->header[1];
       delay(1);
       response->header[1] = wirerecv();
    } while (response->header[0] != 0x00 || response->header[1] != 0xFF);

    delay(1);
    response->len = wirerecv();  
    
    delay(1);
    response->len_chksum = wirerecv();  
    
    delay(1);
    response->direction = wirerecv(); 
    calc_checksum += response->direction;
    
    delay(1);
    response->responseCode = wirerecv(); 
    
    calc_checksum += response->responseCode;
    
    // Kontrollerar så att datan given från PN532 faktiskt är svaret på det givna kommandot
    retVal = response->verifyResponse(cmdCode) ? RESULT_SUCCESS : INVALID_RESPONSE;  
    
    // Hämta datan från svaret
    if (RESULT_OK(retVal)) 
    {  
        // Readjust the len to account only for the data
        // Removing the Direction and response byte from the data length parameter
        response->data_len = response->len - 2;
        
        for (uint8_t i = 0; i < response->data_len; ++i) 
        {
            delay(1); 
            response->data[i] = wirerecv();
            calc_checksum +=  response->data[i];
         }
         
         delay(1); 
         ret_checksum = wirerecv();
        
         if (((uint8_t)(calc_checksum + ret_checksum)) != 0x00) 
         {
            Serial.println(F("Invalid Checksum recievied."));
            retVal = INVALID_CHECKSUM_RX;
         }
        
         delay(1); 
         uint8_t postamble = wirerecv();
         
         
         if (RESULT_OK(retVal) && postamble != 0x00) 
         {
             retVal = INVALID_POSTAMBLE;
             Serial.println(F("Invalid Postamble."));
         }
       
    }
    
   
    if (debug)
    {
      response->printResponse();
      
      Serial.print(F("Calculated Checksum: 0x"));
      Serial.print(calc_checksum, HEX);
      Serial.print(F(" Returned Checksum: 0x") );
      Serial.println(ret_checksum, HEX);
      Serial.println();    
    }

    return retVal;
}

boolean PN532_CMD_RESPONSE::verifyResponse(uint32_t cmdCode)
{
    return ( header[0] == 0x00 && 
             header[1] == 0xFF &&
            ((uint8_t)(len + len_chksum)) == 0x00 && 
            direction == 0xD5 && 
            (cmdCode + 1) == responseCode);
}



