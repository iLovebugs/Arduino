#include "SNEP.h"

SNEP::SNEP(NFCLinkLayer *linkLayer) :
    _linkLayer(linkLayer){
}

SNEP::~SNEP(){
}
// Receives a SNEP request from the client. A link must have been established before this function is called.
// Arguments: data will be set to point to the received NDEF message
//            request[0] will be set the the SNEP request type and request[1] will be set to acceptable length if SNEP request type is Get
// Returns the length of the received NDEF message
uint32_t SNEP::receiveRequest(uint8_t *&data, uint8_t *&request){
  
  uint32_t result = _linkLayer->openLinkToClient(true);
  
  if(RESULT_OK(result)){
    Serial.println(F("SNEP>receiveData: CONNECTED."));
    
    result = _linkLayer->receiveFromClient(data,true);
    
    if(RESULT_OK(result)){
      
      if(data[0] != SNEP_SUPPORTED_VERSION){
          Serial.print(F("SNEP>receiveData: Recieved an SNEP message of an unsupported version: "));
          Serial.println(data[0]);
          return SNEP_UNSUPPORTED_VERSION;
      }
      
      _linkLayer->closeLinkToClient();
      
      // Manage the request that was sent as argument
      request[0] = data[1];
      if(data[1] == SNEP_GET_REQUEST){
        memcpy(&request[1], &data[6], 4);
        data = &data[10];
      }else{
        data = &data[6];
      }
      
      // If a request without any NDEF message is retrieved, no data can be given.
      if(data[1] == SNEP_CONTINUE_REQUEST || data[1] == SNEP_REJECT_REQUEST){
        data = '\0';
      }
      
      
      Serial.println(F("SNEP>receiveData: Returning a SNEP request"));
      Serial.print(F("SNEP>receiveData: Length: "));
      uint32_t length = data[2];
      Serial.println(length, HEX);
      
      return data[2];
    }
  }
  return result;
}
// Transmits a SNEP response to the client. A link must have been established before this function is called
// Arguments: NDEFMessage is the NDEF message to be sent
//            length is the length of the NDEF message to be sent
//            response is the response type to be used
// Returns the length of the received NDEF message
uint32_t SNEP::transmitResponse(uint8_t *NDEFMessage, uint32_t length, uint8_t responseType){
  SNEP_RESPONSE *snepMessage = (SNEP_RESPONSE *) ALLOCATE_HEADER_SPACE(NDEFMessage, SNEP_MESSAGE_HEADER_LEN);
  
  // Incapsulate the NDEF message into a SNEP response message
  snepMessage->version = SNEP_SUPPORTED_VERSION;
  snepMessage->response = responseType;
  snepMessage->length = length;

  uint32_t result = _linkLayer->openLinkToServer(true);
  
  // If connection is error-free
  if(RESULT_OK(result)){ 
    result =  _linkLayer->transmitToServer((uint8_t *)snepMessage, length + SNEP_MESSAGE_HEADER_LEN,true);
  } 
    
  return result;  
}
// Transmits a SNEP request to the server. A link must have been established before this function is called
// Arguments: NDEFMessage is the NDEF message to be sent
//            length is the length of the NDEF message to be sent
//            request[0] is the the SNEP request type and request[1] is the acceptable length if SNEP request type is Get
// Returns the length of the received NDEF message
uint32_t SNEP::transmitRequest(uint8_t *NDEFMessage, uint32_t length, uint8_t *&request){
  

}
// Receives a SNEP response from the server. A link must have been established before this function is called
// Arguments: data will be set to point to the received NDEF message
//            responseType is the response type of the received NDEF message
// Returns the length of the received NDEF message
uint32_t SNEP::receiveResponse(uint8_t *NDEFMessage, uint8_t responseType){
  
}
