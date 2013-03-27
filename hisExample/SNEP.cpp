#include "SNEP.h"

SNEP::SNEP(NFCLinkLayer *linkLayer) :
    _linkLayer(linkLayer){
}

SNEP::~SNEP(){
}
// Receives a SNEP request from the client. 
// Arguments: data will be set to point to the received NDEF message
//            request[0] will be set the the SNEP request type and request[1] will be set to acceptable length if SNEP request type is Get
// Returns the length of the received NDEF message
uint32_t SNEP::receiveRequest(uint8_t *&data){
  
  //Act as server, acts as the SNEP server process present on NFC-enabled devices.
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
      //_linkLayer->closeLinkToClient(); //Recieve a DISC pdu, Client tears down the connection.
      
      // Manage the request that was sent as argument

      if(data[1] == SNEP_PUT_REQUEST){
        data = &data[6]; //Discard SNEP header              
        Serial.println(F("SNEP>receiveData: Returning a SNEP request"));
        Serial.print(F("SNEP>receiveData: Length: "));
        uint32_t length = data[2];
        Serial.println(length, HEX);
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
// Transmits a SNEP response to the client. A link must have been established before this function is called
// Arguments: NDEFMessage is the NDEF message to be sent
//            length is the length of the NDEF message to be sent
//            response is the response type to be used
// Returns the length of the received NDEF message
uint32_t SNEP::transmitSuccsses(){
  SNEP_PDU *snepResponse;
 
  // Incapsulate the NDEF message into a SNEP response message
  snepResponse->version = SNEP_SUPPORTED_VERSION;
  snepResponse->type = SNEP_SUCCESS;
  snepResponse->length = 0;
  
  uint32_t result =  _linkLayer->transmitToServer((uint8_t *)snepResponse, SNEP_PDU_HEADER_LEN,true);    
     
  _linkLayer->closeLinkToClient(); //Recieve a DISC pdu, Client tears down the connection. TODO: ADD TEST IF DISC?
  
  return result;  
}

// Transmits a SNEP request to the server. A link must have been established before this function is called
// Arguments: NDEFMessage is the NDEF message to be sent
//            length is the length of the NDEF message to be sent
//            request[0] is the the SNEP request type and request[1] is the acceptable length if SNEP request type is Get
// Returns the length of the received NDEF message
//why *& as argument?
uint32_t SNEP::transmitPutRequest(uint8_t *NDEFMessage, uint32_t length){
  
  uint32_t result;
  
  //Opening link to server.  
  result = _linkLayer->openLinkToServer(true);
  
  //if data-link was succesfully established continue, else abort.
  if(RESULT_OK(result)){
       
      //If a data-link was established send the request, else abort.
          SNEP_PDU *snepRequest;
          snepRequest = (SNEP_PDU *) ALLOCATE_HEADER_SPACE(NDEFMessage, SNEP_PDU_HEADER_LEN);          
          snepRequest -> version =  SNEP_SUPPORTED_VERSION;
          snepRequest -> type = SNEP_PUT_REQUEST;
          snepRequest -> length = length;
         
          //caller must check the result
          result = _linkLayer -> transmitToServer((uint8_t *)snepRequest, length + SNEP_PDU_HEADER_LEN, true);
      
  } 
  return result;  
}  
  
// Receives a SNEP response from the server. A link must have been established before this function is called
// Arguments: data will be set to point to the received NDEF message
//            responseType is the response type of the received NDEF message
// Returns the length of the received NDEF message
uint32_t SNEP::receiveResponse(uint8_t *&data){
  
  uint32_t result = _linkLayer->receiveFromClient(data,true);
  
  if(RESULT_OK(result)){
         
     if(data[0] != SNEP_SUPPORTED_VERSION){
       Serial.print(F("SNEP>receiveData: Recieved an SNEP message of an unsupported version: "));
       Serial.println(data[0]);
       return SNEP_UNSUPPORTED_VERSION;     
     }
    
     if(data[1] == SNEP_SUCCESS){
       _linkLayer -> closeLinkToServer(); 
       return 0; //in all other cases there is no information present, return 0.
           
      
     }else //In case of Reject or Continue
         return SNEP_UNEXPECTED_RESPONSE;      
  } 
 
   return result; //error code from receiveFromClient 
  
}
