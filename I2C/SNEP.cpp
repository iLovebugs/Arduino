#include "SNEP.h"
#include "Debug.h"

SNEP::SNEP(NFCLinkLayer *linkLayer) :
_linkLayer(linkLayer){
}

SNEP::~SNEP(){
}
// Receives a SNEP put request from the client. 
// Arguments: data will be set to point to the received NDEF message
// Returns the length of the received NDEF message
uint32_t SNEP::receivePutRequest(uint8_t *&data){

  //Act as server, acts as the SNEP server process present on NFC-enabled devices.
  uint32_t result = _linkLayer->openLinkToClient();

  if(RESULT_OK(result)){
    #ifdef SNEPdebug
    Serial.println(F("SNEP>receiveRequst: CONNECTED."));
    #endif

    result = _linkLayer->receiveSNEP(data);

    if(RESULT_OK(result)){

      if(data[0] != SNEP_SUPPORTED_VERSION){
        #ifdef SNEPdebug
        Serial.print(F("SNEP>receiveRequst: Recieved an SNEP message of an unsupported version: "));
        Serial.println(data[0]);
        #endif
        return SNEP_UNSUPPORTED_VERSION;
      }     

      // Manage the request that was sent as argument

      if(data[1] == SNEP_PUT_REQUEST){
        data = &data[6]; //Discard SNEP header
        #ifdef SNEPdebug              
        Serial.println(F("SNEP>receiveRequst: Receiving a SNEP put request"));
        Serial.print(F("SNEP>receiveRequst: Length: "));
        #endif
        uint32_t length = data[2];
        #ifdef SNEPdebug
        Serial.print(F("0x"));
        Serial.println(length, HEX);
        #endif
        return data[2]; //returing length of NDEF message

      }
      else{ // If a request without any NDEF message is retrieved(Reject, Continue or Get), no data can be returned.    
        data = '\0';
        return SNEP_UNEXPECTED_REQUEST;
      }
    }
  }
  return result;
}
// Transmits a SNEP success PDU to the client. A link must have been established before this function is called
// Arguments: buffer is the buffer used to build the message to transmit.
// Returns the length of the received NDEF message
uint32_t SNEP::transmitSuccessAndTerminateSession(uint8_t *&txNDEFMessagePtr)
{
  SNEP_PDU *snepResponse = (SNEP_PDU *) ALLOCATE_HEADER_SPACE(txNDEFMessagePtr, SNEP_PDU_HEADER_LEN);
  uint32_t result;
  uint8_t * responsPtr = (uint8_t *)snepResponse;
  
  // Incapsulate the NDEF message(which does not exist) into a SNEP response message
  snepResponse->version = SNEP_SUPPORTED_VERSION;
  snepResponse->type = SNEP_SUCCESS;
  snepResponse->nothing[0] = 0;
  snepResponse->nothing[1] = 0;
  snepResponse->nothing[2] = 0;
  snepResponse->length = 0;                  
  
  result = _linkLayer->transmitSNEP(responsPtr, SNEP_PDU_HEADER_LEN);    
  if(RESULT_OK(result)){
    result = _linkLayer->closeLinkToClient(); //Recieve a DISC pdu, Client tears down the connection. 
  }
  return result;  
}

// Transmits a SNEP request to the server. A link must have been established before this function is called
// Arguments: NDEFMessage is the NDEF message to be sent
//            length is the length of the NDEF message to be sent
//            request[0] is the the SNEP request type and request[1] is the acceptable length if SNEP request type is Get
// Returns the length of the received NDEF message
//why *& as argument?
uint32_t SNEP::transmitPutRequest(uint8_t *&NDEFMessage, uint8_t length, boolean sleep){

  uint32_t result;

  //Opening link to server.  
  result = _linkLayer->openLinkToServer(sleep);

  //if data-link was succesfully established continue, else abort.
  if(RESULT_OK(result)){

    //Build SNEP frame   
    SNEP_PDU *snepRequest = (SNEP_PDU *) ALLOCATE_HEADER_SPACE(NDEFMessage, SNEP_PDU_HEADER_LEN);          
    uint8_t * requestPtr = (uint8_t *)snepRequest;
    snepRequest -> version =  SNEP_SUPPORTED_VERSION;
    snepRequest -> type = SNEP_PUT_REQUEST;
    snepRequest -> length = length;

    //caller must check the result
    result = _linkLayer -> transmitSNEP(requestPtr, length + SNEP_PDU_HEADER_LEN);

  } 
  return result;  
}  

// Receives a SNEP response from the server. A link must have been established before this function is called
// Arguments: data will be set to point to the received NDEF message
//            responseType is the response type of the received NDEF message
// Returns the length of the received NDEF message
uint32_t SNEP::receiveSuccessAndTerminateSession(uint8_t *&data){

  uint32_t result = _linkLayer->receiveSNEP(data);

  if(RESULT_OK(result)){

    if(data[0] != SNEP_SUPPORTED_VERSION){
      #ifdef SNEPdebug
      Serial.print(F("SNEP>receiveData: Recieved an SNEP message of an unsupported version: "));
      Serial.println(data[0]);
      #endif
      return SNEP_UNSUPPORTED_VERSION;     
    }

    if(data[1] != SNEP_SUCCESS){
      return SNEP_UNEXPECTED_RESPONSE; //In case of Reject or Continue
    }
      result = _linkLayer -> closeLinkToServer();   
  } 
  return result;
}
