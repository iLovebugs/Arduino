#include "PN532.h"
#include "NFCLinkLayer.h"
#include "NDEFPushProtocol.h"
#include <avr/power.h>
#include <avr/sleep.h>

#define SCK 13
#define MOSI 11
#define SS 10
#define MISO 12

PN532 nfc(SCK, MISO, MOSI, SS);
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
    txLen = nppLayer.createNDEFShortRecord(message, 5, txNDEFMessagePtr);    
    
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
    nfc.configurePeerAsTarget(NPP_SERVER);
    attachInterrupt(0, phoneInRange, FALLING);
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
        //Serial.println("Begin Rx Loop");
        rxResult = nppLayer.rxNDEFPayload(rxNDEFMessagePtr);
        
        if (rxResult == SEND_COMMAND_RX_TIMEOUT_ERROR)
        {
           break;
        }
      
        txResult = nppLayer.pushPayload(txNDEFMessagePtr, txLen);
               
        if (RESULT_OK(rxResult))
        {
           Serial.println(F("Rx Packet"));
           boolean lastPayload;
           uint8_t *ndefTextPayload;
           uint8_t len = nppLayer.retrieveTextPayload(rxNDEFMessagePtr, ndefTextPayload, lastPayload);
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
            rxNDEFMessagePtr = &rxNDEFMessage[0];
            txLen = nppLayer.createNDEFShortRecord(ndefTextPayload, len, txNDEFMessagePtr);    
            
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
    
    sleepMCU();
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
   
