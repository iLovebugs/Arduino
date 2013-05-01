  
  
  ////////////////////////////////////////////////
  ////////  Lägga till kontroll av frames runtime. 
  
  ///////// Denna koden fungera inte. Antagligen för att det blir galet med configurePeerAstarget nu. Ärver från reader och tar enbart ett argument i PN532.


  #include "PN532.h"
  #include "NFCLinkLayer.h"
  #include "NDEFMessage.h"
  #include "SNEP.h"
  #include <Wire.h>
  #include <avr/power.h>
  #include <avr/sleep.h>
  #include <avr/tools.h>
  #include <bignum.h>
  #include <config.h>
  #include <rsa.h>
  
  
  
#define _irq  2
#define _reset  3
#define _red  8
#define _green 9
  
  PN532 nfc(_irq, _reset);
  NFCLinkLayer linkLayer(&nfc);
  SNEP snep(&linkLayer);
  NDEFMessage ndefmessage;
  
  // This message shall be used to rx or tx 
  // NDEF messages it shall never be released
  #define MAX_PKT_HEADER_SIZE  50
  #define MAX_PKT_RX_PAYLOAD_SIZE 100
  #define MAX_PKT_PAYLOAD_SIZE 11
  #define MAX_PKT_PUBLIC_KEY_SIZE 128
  uint8_t rxNDEFMessage[MAX_PKT_HEADER_SIZE + MAX_PKT_RX_PAYLOAD_SIZE];
  uint8_t txNDEFMessage[MAX_PKT_HEADER_SIZE + MAX_PKT_PAYLOAD_SIZE + MAX_PKT_PUBLIC_KEY_SIZE];
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
  
  //Belongs to the cryptography//////////////////////
      int ret;
      rsa_context rsa;
      char *n;
      char *e;
      char *d;
      char *p;
      char *q;
      char *dp;
      char *dq;
      char *qp;
      
      size_t i;
      unsigned char result[64];
      unsigned char buf[64];
      char *pek;
      int c;
      unsigned long rando;
      //////////////////////////////////////////////////
  
  
  
  void setup(void) {
  		Serial.begin(9600);
  		Serial.println("Hello!");
  		

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
	pinMode(_red,OUTPUT);
        pinMode(_green,OUTPUT);
  
                //Public/Private keys + padding 
                
                
                
                n = "BB16FC7AA5C19F8FE4EE32D6A532FCAEE777EC77BC9AEEC103BA51BA0C9A36DFE6F7DF89C14D2F0186E0EF13A12C6103D4281FF8F4C14B18064EBCE66D0E0B07";
                e = "010001";
                d = "AC9ADD5E8DF45FB092C60BD329E02B6D761196F134E93FA2853CED4F9776E36E0198CD0D652A0A92D7B986E9B7FA8428805D2E44B827DEE9F977CB0B63D1BB19";
                p = "F8206BEEAF0C46D6809340458CC8E356D68D35310DD087397E2943EEC897C505";
                q = "C106C042EA157346C4CB245071DDA3083E640DF85312FF2555D5B5DB0BEE8D9B";
                dp = "19903D8E79BA6A11EF6D3C51EE0F445CCDFDFE5CEF6F6C7F1FE0607F596B4981";
                dq = "AE37B8A9EC7B25CB76ED5EBE58B75161AC664411A07161E641BD9CE0B2B94207";
                qp = "C7506F034F9474457BB9C3AF5583551D378FD1681DF22C04D327AD52B33DEC66";
                
         
                //initialize rsa keys THIS SHOULD REMAIN IN SETUP
                rsa_init( &rsa, RSA_PKCS_V15, 0 );
                if( (ret = mpi_read_string(&rsa.N, 16, n))!=0 ||
                    ( ret = mpi_read_string( &rsa.E , 16, e ) ) != 0 ||
                    ( ret = mpi_read_string( &rsa.D , 16, d ) ) != 0 ||
                    ( ret = mpi_read_string( &rsa.P , 16, p ) ) != 0 ||
                    ( ret = mpi_read_string( &rsa.Q , 16, q ) ) != 0 ||
                    ( ret = mpi_read_string( &rsa.DP, 16, dp ) ) != 0 ||
                    ( ret = mpi_read_string( &rsa.DQ, 16, dq ) ) != 0 ||
                     ( ret = mpi_read_string( &rsa.QP, 16, qp ) ) != 0 )
                    {
                    Serial.println(F("Error: Failed to initialize keys"));
                    }
                   else{
                      Serial.println(F("RSA initializing: Done"));
                   }  
  		
  		// configure board to read RFID tags and cards
  		nfc.SAMConfig();
  
                ////Initializes the random generator//////  
                 randomSeed(analogRead(0));
  		
  		Serial.print(F("Minne:"));
  		Serial.println(freeMemory());
  }
  
  
  void loop(void) 
  {
  	while(true){
  	 Serial.println();
  	 Serial.println(F("---------------- LOOP ----------------------"));
  	 Serial.println();
  			
  			digitalWrite(_red, LOW);
                        digitalWrite(_green, LOW); 
  			if(tryAgain == false){
  				Serial.println(F("----- Computing keys and creates a NDEF-message"));

                         
                            
  		
  		          // initiate pointers and variables
  		          txNDEFMessagePtr = &txNDEFMessage[MAX_PKT_HEADER_SIZE + MAX_PKT_PAYLOAD_SIZE];
                          
                          //Add public key to every message to be sent.
                          memcpy(txNDEFMessagePtr, n, MAX_PKT_PUBLIC_KEY_SIZE);
                          txNDEFMessagePtr = &txNDEFMessage[MAX_PKT_HEADER_SIZE];				 
   
  
  				
  				//Move to setup, this is lock id that is not changed.
  				
                                memcpy(txNDEFMessagePtr, "TE01100", 7);
                                /////////////////////////////////
                                /////Generating a pseudo-random number from 0-299
                                ////////////////////////////////
                                rando = random(1000);
                                char buffer[4];
                                // convert from an unsigned long int to a 4-byte array and place in txNDEFMessage
                                sprintf(buffer, "%lu",rando);
                                                                
                                txNDEFMessagePtr[7] = buffer[0];
                                txNDEFMessagePtr[8] = buffer[1];
                                txNDEFMessagePtr[9] = buffer[2];
                                txNDEFMessagePtr[10] = buffer[3];
  				txLen = ndefmessage.createNDEFShortRecord(txNDEFMessagePtr,139);
  				if(!txLen){ 
  						Serial.println("----- Failed to create NDEF Message.");
  						break;
  				}     
                                Serial.print(F("Random: "));
                                Serial.println(rando);
                                Serial.println(txLen);
                                
  			}else{
  				Serial.println(F("----- Tries to complete the process with the same keys as before"));
  			}
  			
  			tryAgain = true;
  			
  				// Print out the created NDEF-message
  				
  				Serial.println(F("NDEF Message")); 
  				for (uint16_t i = 0; i < txLen; i++)
  				{
  						Serial.print(F("0x")); 
  						Serial.print(txNDEFMessagePtr[i], HEX);
  						Serial.print(" ");
  				}

  			
  				Serial.println(F("\n----- Transmit request"));
  				
  				////////////////////////////////////////////////////////////////////////////////////////////////////////////
				////Establish a LLC datalink connection and send a SNEP message and send LOCKID, random number and publicKey
				////////////////////////////////////////////////////////////////////////////////////////////////////////////
  
  				Serial.println(F("Staring sending procedure"));
  				
  				transmitResult = snep.transmitPutRequest(txNDEFMessagePtr, txLen, true);
  				if(IS_ERROR(transmitResult)){
  					Serial.print(F("----- The SNEP put request could not be sent due to error: 0x"));
  					Serial.println(transmitResult, HEX);
  					break;
  				}else
  					Serial.println(F("----- Request transmitted\n"));
  					
  				///////////////////////////////////////////////////////////////////////////
				/////Receive a SNEP Success PDU from the NFC-device and close the session.
                                ///////////////////////////////////////////////////////////////////////////
  				
  				Serial.println(F("----- Receive response"));
  				receiveResult = snep.receiveSuccessAndTerminateSession(rxNDEFMessagePtr);
  				if(IS_ERROR(receiveResult)){
  					Serial.print(F("----- The SNEP success message could not be received due to error: 0x"));
  					Serial.println(transmitResult, HEX);
  					break;
  				}else
  					Serial.println(F("----- Response received\n"));
  					Serial.println(transmitResult, HEX);
  			
  		
  				Serial.println(F("3000 ms delay"));
  				delay(3000);    // now testing with 3000:: 1000 is working on HTC
  				
  //5000 with prints, 1000 without.
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
				  /////Decrypt the NDEF message         
			          /////////////////////////////////////////////////////////////////////////         
  					
  				if (RESULT_OK(receiveResult))
  				{                                           		
                                        txNDEFMessagePtr = &txNDEFMessage[MAX_PKT_HEADER_SIZE];
                          		transmitResult = snep.transmitSuccessAndTerminateSession(txNDEFMessagePtr);    
    
  					 Serial.println(F("----- Recevied data:"));
  					 boolean lastPayload;
  					
  					 uint8_t len = ndefmessage.retrieveTextPayload(rxNDEFMessagePtr, ndefTextPayload, lastPayload);
  	
                                  // message to be decrypted begins here.
                                  pek =  (char *)ndefTextPayload[11]; 
                                  
                                 if (len) 
					 {
  
                                           rsa.len = ( mpi_msb( &rsa.N ) + 7 ) >> 3;
                                           
                                           i=0; //här blir det ett konstigt vector error
                                           while( sscanf( pek, "%02x", &c ) > 0 &&
                                           i < (int) sizeof( buf ) ){
                                             pek+=2;
                                             buf[i++] = (unsigned char) c;
                                           }
                                            Serial.println("Starting to decrypt...");
                                            
                                            if( ( ret = rsa_pkcs1_decrypt( &rsa, RSA_PRIVATE, &i, buf, result,
                                               64 ) ) != 0 )
                                             {
                                             Serial.println(ret);
                                             }else{
                                               //For debuggning only
                                                                                       
        
                                               Serial.println(F("Done Decrypting The Key is:"));
                                               
                                               int le=i;
                                               for(int x=0;x<le;x++){
                                                 Serial.print((char) result[x]);
                                               }
 
                                              }
                                              Serial.println(F("\nDone Everything"));
                                           
                                           
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
  		
  		if(IS_ERROR(transmitResult)){
  			Serial.print(F("----- SNEP success messsage could not be transmitted due to error: 0x"));
  			Serial.println(transmitResult, HEX);
  			break;
  		}else
  			Serial.println(F("----- SNEP success messsage transmitted\n"));
  
                        txNDEFMessagePtr = &txNDEFMessage[MAX_PKT_HEADER_SIZE];  
  			if(!strcmp( "TE01200Anna",  (char *) result))
  			{
                                memcpy(txNDEFMessagePtr, "TE01300", 7);
                               txLen = ndefmessage.createNDEFShortRecord(txNDEFMessagePtr,7);
  				if(!txLen){ 
  						Serial.println("----- Failed to create NDEF Message.");
  						break;
  				}  
                                digitalWrite(_green, HIGH);                 
  
  			}
  			else
  			{
  				memcpy(txNDEFMessagePtr, "TE01310", 7);
     				txLen = ndefmessage.createNDEFShortRecord(txNDEFMessagePtr,7);
  				if(!txLen){ 
  						Serial.println("----- Failed to create NDEF Message.");
  						break;
  				}
                                digitalWrite(_red, HIGH);
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
  
  
   
  	 
