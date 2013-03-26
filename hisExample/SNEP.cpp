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
uint32_t SNEP::receiveRequest(uint8_t *&data, uint8_t *&request){
  
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
      
     // _linkLayer->closeLinkToClient(); //Recieve a DISC pdu, Client tears down the connection.
      
      // Manage the request that was sent as argument
      request[0] = data[1];
      if(data[1] == SNEP_GET_REQUEST){
        memcpy(&request[1], &data[6], 4); //Why 4, this point to the actual data that whas transmitted, 
        data = &data[10];
      }else{
        data = &data[6]; //Discard SNEP header
      }
      
     /* // If a request without any NDEF message is retrieved, no data can be returned.
      if(data[1] == SNEP_CONTINUE_REQUEST || data[1] == SNEP_REJECT_REQUEST){
        data = '\0';
      }*/
      Serial.println(F("SNEP>receiveData: Returning a SNEP request"));
      Serial.print(F("SNEP>receiveData: Length: "));
      uint32_t length = data[2];
      Serial.println(length, HEX);
      
      return data[2]; //Why data 2?
    }
  }
  return result;
}
// Transmits a SNEP response to the client. A link must have been established before this function is called
// Arguments: NDEFMessage is the NDEF message to be sent
//            length is the length of the NDEF message to be sent
//            response is the response type to be used
// Returns the length of the received NDEF message
uint32_t SNEP::transmitResponse(uint8_t *NDEFMessage, uint32_t length, uint8_t *responseType){
  SNEP_PDU *snepResponse;
  
  //We only add information IF a GET_REQUEST was received 
  if(responseType[0] == SNEP_GET_REQUEST)
    snepResponse = (SNEP_PDU *) ALLOCATE_HEADER_SPACE(NDEFMessage, SNEP_PDU_HEADER_LEN);
  
  
  //What about Acceptable Lenght?
  
  // Incapsulate the NDEF message into a SNEP response message
  snepResponse->version = SNEP_SUPPORTED_VERSION;
  snepResponse->type = *responseType;
  snepResponse->length = length;

  // uint32_t result = _linkLayer->openLinkToServer(true); //We open the link here, so why does it say that a link must be opened before this is called?
  
    uint32_t result =  _linkLayer->transmitToServer((uint8_t *)snepResponse, length + SNEP_PDU_HEADER_LEN,true);    
   
   //TODO bad name?
   _linkLayer->closeLinkToClient(); //Recieve a DISC pdu, Client tears down the connection.
  
  return result;  
}

// Transmits a SNEP request to the server. A link must have been established before this function is called
// Arguments: NDEFMessage is the NDEF message to be sent
//            length is the length of the NDEF message to be sent
//            request[0] is the the SNEP request type and request[1] is the acceptable length if SNEP request type is Get
// Returns the length of the received NDEF message
//why *& as argument?
uint32_t SNEP::transmitRequest(uint8_t *NDEFMessage, uint32_t length, uint8_t request){
  
  uint32_t result;
  //Opening link to server.  
  result = _linkLayer->openLinkToServer(true);
  
  //if data-link was succesfully established continue, else abort.
  if(RESULT_OK(result)){
  
    if(request == SNEP_GET_REQUEST){
      SNEP_GET_REQ_PDU *snepGetRequest;          
      snepGetRequest = (SNEP_GET_REQ_PDU *) ALLOCATE_HEADER_SPACE(NDEFMessage, SNEP_GET_REQ_HEADER_LEN);
      snepGetRequest -> version =  SNEP_SUPPORTED_VERSION;
      snepGetRequest -> type = request;
      snepGetRequest -> length = length;
      snepGetRequest -> acceptableLength = SNEP_ACCEPTABLE_LENGTH; //set to default size?
          
      //Caller must check the result
      result = _linkLayer -> transmitToServer((uint8_t *)snepGetRequest, length + SNEP_GET_REQ_HEADER_LEN, true);        
    } 
        //All Request frames except GET uses a common structure
    else{  
      //If a data-link was established send the request, else abort.
          SNEP_PDU *snepRequest;
          snepRequest = (SNEP_PDU *) ALLOCATE_HEADER_SPACE(NDEFMessage, SNEP_PDU_HEADER_LEN);          
          snepRequest -> version =  SNEP_SUPPORTED_VERSION;
          snepRequest -> type = request;
          snepRequest -> length = length;
         
          //caller must chech the result
          result = _linkLayer -> transmitToServer((uint8_t *)snepRequest, length + SNEP_PDU_HEADER_LEN, true);
      }
  } 
  return result;  
}  
  
// Receives a SNEP response from the server. A link must have been established before this function is called
// Arguments: data will be set to point to the received NDEF message
//            responseType is the response type of the received NDEF message
// Returns the length of the received NDEF message
uint32_t SNEP::receiveResponse(uint8_t *&data, uint8_t responseType){
  
  uint32_t result = _linkLayer->receiveFromClient(data,true);
  
  if(RESULT_OK(result)){
         
      if(data[0] != SNEP_SUPPORTED_VERSION){
          Serial.print(F("SNEP>receiveData: Recieved an SNEP message of an unsupported version: "));
          Serial.println(data[0]);
          return SNEP_UNSUPPORTED_VERSION;     
    }
    
     if(data[1] == SNEP_SUCCESS){
         if(responseType == SNEP_GET_REQUEST){
           data = &data[6]; //discard SNEP header
           return data[2]; //return payloadlength of NDEF-pdu
         }
         else{
           return 0; //in all other cases there is no information present, return 0.
           data ='\0'; 
         } 
     }else
         return SNEP_UNEXPECTED_RESPONSE;      
  } 
  
    _linkLayer -> closeLinkToServer(); //TODO must write this function...
    
  
  return result; //error code from receiveFromClient
  
  
  
  
  
  
}
