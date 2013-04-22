

////////////////////////////////////////////////
////////  Lägga till kontroll av frames runtime. 

///////// Denna koden fungera inte. Antagligen för att det blir galet med configurePeerAstarget nu. Ärver från reader och tar enbart ett argument i PN532.

#include <Wire.h>
#include "PN532.h"
#include "NFCLinkLayer.h"
#include "NDEFMessage.h"
#include "SNEP.h"
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/tools.h>

#define _irq  2
#define _reset  3
#define _lock  8

PN532 nfc(_irq, _reset);
NFCLinkLayer linkLayer(&nfc);
SNEP snep(&linkLayer);
NDEFMessage ndefmessage;

// This message shall be used to rx or tx 
// NDEF messages it shall never be released
#define MAX_PKT_HEADER_SIZE  50
#define MAX_PKT_PAYLOAD_SIZE 100
uint8_t rxNDEFMessage[MAX_PKT_HEADER_SIZE + MAX_PKT_PAYLOAD_SIZE];
uint8_t txNDEFMessage[MAX_PKT_HEADER_SIZE + MAX_PKT_PAYLOAD_SIZE];
uint8_t *txNDEFMessagePtr; 
uint8_t *rxNDEFMessagePtr; 
 uint8_t *ndefTextPayload;
uint8_t txLen;


uint32_t receiveResult; 
uint32_t transmitResult;

boolean tryAgain;
		
#define SHORT_RECORD_TYPE_LEN   0x0A
#define NDEF_SHORT_RECORD_MESSAGE_HDR_LEN   0x03 + SHORT_RECORD_TYPE_LEN
#define TYPE_STR "text/plain"

// When arduino has woken up this function will be executed

void setup(void) {
		Serial.begin(9600);
		Serial.println("Hello!");
		
		//////////////////////////////////////////////
		/////Setting sleep mode parameters

		
		// initiate pointers and variables
		txNDEFMessagePtr = &txNDEFMessage[MAX_PKT_HEADER_SIZE];
		rxNDEFMessagePtr = &rxNDEFMessage[0];  
		tryAgain = false;
		
		// initiate PN532
		nfc.initializeReader();
		
		// check if board exists
		uint32_t versiondata = nfc.getFirmwareVersion();
		if (! nfc.getFirmwareVersion()) {
				Serial.print("Didn't find PN53x board");
				while (1); // halt
		}
		 
		pinMode(_irq,INPUT);
		pinMode(_lock,OUTPUT);
		
		// configure board to read RFID tags and cards
		nfc.SAMConfig();
		//nfc.configurePeerAsTarget(true);
		
		Serial.print(F("Minne:"));
		Serial.println(freeMemory());
}


void loop(void) 
{
	while(true){
	 Serial.println();
	 Serial.println(F("---------------- LOOP ----------------------"));
	 Serial.println();
			
			digitalWrite(_lock, LOW); 
			if(tryAgain == false){
				Serial.println(F("----- Computing keys and creates a NDEF-message"));
				//////////////////////////////////
				//Compute public and private key//
				//////////////////////////////////
				 
				
				//Move to setup, this is lock id that is not changed.
				uint8_t message[12] = "TE01100    ";
				txNDEFMessagePtr = &txNDEFMessage[MAX_PKT_HEADER_SIZE];
				rxNDEFMessagePtr = &rxNDEFMessage[0];
				txLen = ndefmessage.createNDEFShortRecord(message, 11, txNDEFMessagePtr);
				if(!txLen){ 
						Serial.println("----- Failed to create NDEF Message.");
						break;
				} 
			}else{
				Serial.println(F("----- Tries to complete the process with the same keys as before"));
			}
			
			tryAgain = true;
			
				// Print out the created NDEF-message
				uint8_t *buf = (uint8_t *) txNDEFMessagePtr;
				Serial.println(F("NDEF Message")); 
				for (uint16_t i = 0; i < txLen; i++)
				{
						Serial.print(F("0x")); 
						Serial.print(buf[i], HEX);
						Serial.print(" ");
				}
				
				////////////////////////////////////////////////////////////////////////////
				/////Send LOCK id and public key to NFC-device.
			
				Serial.println(F("\n----- Transmit request"));
				
				////////////////////////////////////////////////////////////////////////////
				////Handle sending and receiveing ART frames

				Serial.println(F("Staring sending procedure"));
			 
				///////////////////////////////////////////////////////////////////////////
				////Establish a LLC datalink connection and send a SNEP message.
				
				transmitResult = snep.transmitPutRequest(txNDEFMessagePtr, txLen, true);
				if(IS_ERROR(transmitResult)){
					Serial.print(F("----- The SNEP put request could not be sent due to error: 0x"));
					Serial.println(transmitResult, HEX);
					break;
				}else
					Serial.println(F("----- Request transmitted\n"));
					
				////////////////////////////////////////////////////////////////////////////
				/////Receive a SNEP Success PDU from the NFC-device and close the session.
				
				Serial.println(F("----- Receive response"));
				receiveResult = snep.receiveSuccessAndTerminateSession(rxNDEFMessagePtr);
				if(IS_ERROR(receiveResult)){
					Serial.print(F("----- The SNEP success message could not be received due to error: 0x"));
					Serial.println(transmitResult, HEX);
					break;
				}else
					Serial.println(F("----- Response received\n"));
					Serial.println(transmitResult, HEX);
			
			
				Serial.println(F("----- DELAY 1000\n"));
				delay(1000);
				
				////////////////////////////////////////////////////////////////////////////
				///// Receive an encrypted message from the NFC-device
				///////////////////////////////////////////////////////////////////////////
							 
				
				Serial.println(F("----- Receive request"));
				
				 ////////////////////////////////////////////////////////////////////////////
				////Handle sending and receiveing ART frames
				
				Serial.println(F("Staring recieving procedure"));
				
				/////////////////////////////////////////////////////////////////////////////
				////Establishes a datalink connection with NFC device. Recieves an NDEF message.        
				
				receiveResult = snep.receivePutRequest(rxNDEFMessagePtr);
			 if(IS_ERROR(receiveResult)){
					Serial.print(F("----- The SNEP put request could not be received due to error: 0x"));
					Serial.println(receiveResult, HEX);
					break;
				}else
					Serial.println(F("----- Request received\n"));
					
					
					//////////////////////////////////////////////////////////////////////////
					/////Decrypt key          
					
				// Print out the decrypted message!
				if (RESULT_OK(receiveResult))
				{
					 Serial.println(F("----- Recevied data:"));
					 boolean lastPayload;
					
					 uint8_t len = ndefmessage.retrieveTextPayload(rxNDEFMessagePtr, ndefTextPayload, lastPayload);
					 if (len) 
					 {
							 for (uint32_t i = 0; i < len +1 ; ++i)
							 {  
									 Serial.print("0x"); Serial.print(ndefTextPayload[i], HEX); Serial.print(" ");
							 }
							 Serial.println();
							 Serial.println((char *) ndefTextPayload);
					 }           
				}
			

		Serial.print(F("<Setup> Minne:"));
		Serial.println(freeMemory());
				
				
		////////////////////////////////////////////////////////////////////////////
		/////Send a SNEP Success PDU the the NFC-device and close the datalink connection. 
		
		Serial.println(F("\n----- Transmit SNEP success message"));
		transmitResult = snep.transmitSuccessAndTerminateSession(txNDEFMessagePtr);
		if(IS_ERROR(transmitResult)){
			Serial.print(F("----- SNEP success messsage could not be transmitted due to error: 0x"));
			Serial.println(transmitResult, HEX);
			break;
		}else
			Serial.println(F("----- SNEP success messsage transmitted\n"));

			if(!strcmp( "TE01200Anna",  (char *) ndefTextPayload))
			{
				uint8_t message[12] = "TE01300    ";
				txNDEFMessagePtr = &txNDEFMessage[MAX_PKT_HEADER_SIZE];
				rxNDEFMessagePtr = &rxNDEFMessage[0];
				txLen = ndefmessage.createNDEFShortRecord(message, 11, txNDEFMessagePtr);
				if(!txLen){ 
						Serial.println("----- Failed to create NDEF Message.");
						break;
				}
			}
			else
			{
				uint8_t message[12] = "TE01310    ";
				txNDEFMessagePtr = &txNDEFMessage[MAX_PKT_HEADER_SIZE];
				rxNDEFMessagePtr = &rxNDEFMessage[0];
				txLen = ndefmessage.createNDEFShortRecord(message, 11, txNDEFMessagePtr);
				if(!txLen){ 
						Serial.println("----- Failed to create NDEF Message.");
						break;
				}
			}

			Serial.println(F("Staring sending procedure"));
			 
				///////////////////////////////////////////////////////////////////////////
				////Establish a LLC datalink connection and send a SNEP message.
				
				transmitResult = snep.transmitPutRequest(txNDEFMessagePtr, txLen, false);
				if(IS_ERROR(transmitResult)){
					Serial.print(F("----- The SNEP put request could not be sent due to error: 0x"));
					Serial.println(transmitResult, HEX);
					break;
				}else
					Serial.println(F("----- Request transmitted\n"));
					
				////////////////////////////////////////////////////////////////////////////
				/////Receive a SNEP Success PDU from the NFC-device and close the session.
				
				Serial.println(F("----- Receive response"));
				receiveResult = snep.receiveSuccessAndTerminateSession(rxNDEFMessagePtr);
				if(IS_ERROR(receiveResult)){
					Serial.print(F("----- The SNEP success message could not be received due to error: 0x"));
					Serial.println(transmitResult, HEX);
					break;
				}else
					Serial.print(F("----- Response received: "));

					digitalWrite(_lock, HIGH);
					Serial.println(F("----- Door is now open!"));
					delay(5000);
						
			 tryAgain = false;


	}
	
		////////////////////////////////////////////////////////////////////////////////////
		/////// Determine if key was correct or not


		////////////////////////////////////////////////////////////////////////////////////
		///////Correct key. Initialize new session. Send success code.    
	
	
	
	////////////////////////////////////////////////////////////////////////////
	/////Key was incorrect. Initialize new session. Send fail code.  
	
	
}

////////////////////////////////////////////
///sleep is used in combination with configurepeerastarget when it is called in the sedning procedure.


 
	 
