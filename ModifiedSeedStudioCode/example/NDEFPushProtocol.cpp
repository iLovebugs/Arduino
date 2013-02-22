#include "NDEFPushProtocol.h"

NDEFPushProtocol::NDEFPushProtocol(NFCLinkLayer *linkLayer) :
    _linkLayer(linkLayer)
{
}

NDEFPushProtocol::~NDEFPushProtocol() 
{
}

// Har flyttat hit från ino filen
uint32_t NDEFPushProtocol::createNDEFShortRecord(uint8_t *message, uint8_t payloadLen, uint8_t *&NDEFMessage)
{
   //Serial.print("Message: ");
   //Serial.println((char *)message);
   uint8_t * NDEFMessageHdr = ALLOCATE_HEADER_SPACE(NDEFMessage, NDEF_SHORT_RECORD_MESSAGE_HDR_LEN);
   
   NDEFMessageHdr[0] =  NDEF_MESSAGE_BEGIN_FLAG | NDEF_MESSAGE_END_FLAG | NDEF_MESSAGE_SHORT_RECORD | TYPE_FORMAT_MEDIA_TYPE; 
   NDEFMessageHdr[1] =  SHORT_RECORD_TYPE_LEN;
   NDEFMessageHdr[2] =  payloadLen;
   memcpy(&NDEFMessageHdr[3], TYPE_STR, SHORT_RECORD_TYPE_LEN);
   memcpy(NDEFMessage, message, payloadLen);
   //Serial.print("NDEF Message: ");
   //Serial.println((char *)NDEFMessage);   
   NDEFMessage = NDEFMessageHdr;
   return (payloadLen + NDEF_SHORT_RECORD_MESSAGE_HDR_LEN);   
}

// Har flyttat hit från ino filen
uint32_t NDEFPushProtocol::retrieveTextPayloadFromShortRecord(uint8_t *NDEFMessage, uint8_t type, uint8_t *&payload, boolean isIDLenPresent)
{
   NDEFState currentState =  NDEF_TYPE_LEN;
   payload = NULL;
   
   uint8_t typeLen;
   uint8_t idLen;
   uint8_t payloadLen;
   uint8_t idx = 1;
   
   while (currentState != NDEF_FINISHED)
   {
      switch (currentState) 
      {
          case NDEF_TYPE_LEN:
          {
             typeLen = NDEFMessage[idx++];
             currentState = NDEF_PAYLOAD_LEN;
          }
          // Purposefully allowing it to fall through
          case NDEF_PAYLOAD_LEN:
          {
             payloadLen = NDEFMessage[idx++];
             currentState =  isIDLenPresent ? NDEF_ID_LEN : NDEF_TYPE;
          }
          break;
          case NDEF_ID_LEN:
          {
             idLen = NDEFMessage[idx++];
             currentState = NDEF_TYPE;
          }
          // Purposefully allowing it to fall through
          case NDEF_TYPE:
          {
             if (type == TYPE_FORMAT_NFC_FORUM_TYPE )
             {
                 for (uint8_t i = 0; i < (typeLen - 1); ++i) {
                    if (NDEFMessage[idx++] != 0x00)
                    {
                       Serial.println("Unhandled NDEF Message Type.");
                       return 0;
                    }
                 }
                 
                 if ( NDEFMessage[idx++] != NFC_FORUM_TEXT_TYPE ) 
                 {
                     Serial.println("Unhandled NDEF Message Type.");
                     return 0;
                 }
             }
             else if (type == TYPE_FORMAT_MEDIA_TYPE)
             { 
                 const char *typeStr =  (char *)&NDEFMessage[idx];
                 if (typeLen != 0xA || strncmp(typeStr, "text/plain", typeLen) != 0)
                 {
                    NDEFMessage[typeLen + idx] = NULL;
                    Serial.print("Unknown Type: ");
                    Serial.println(NDEFMessage[idx]);
                    return 0;
                 }
                 idx += typeLen;
                 
             }
             else 
             {
                 return 0;
             }
             currentState =  isIDLenPresent ? NDEF_ID : NDEF_PAYLOAD;
          }
          break;
          case NDEF_ID:
          {
              // Skip over the ID field
              idx += idLen;
              currentState = NDEF_PAYLOAD;
          }
          // Purposefully allowing it to fall through
          case NDEF_PAYLOAD:
          {
              payload = &NDEFMessage[idx];
              payload[payloadLen] = '\0';  
              currentState = NDEF_FINISHED;
          }
          break;
          default:
            currentState = NDEF_FINISHED;
      }
   }
   
   return (payload != NULL) ? payloadLen : 0;
}

// Har flyttat hit från ino filen
uint32_t NDEFPushProtocol::retrieveTextPayload(uint8_t *NDEFMessage, uint8_t *&payload, boolean &lastTextPayload)
{
   uint8_t type = (NDEFMessage[0] & NDEF_MESSAGE_TYPENAME_FORMAT);
   if (type != TYPE_FORMAT_NFC_FORUM_TYPE && type != TYPE_FORMAT_MEDIA_TYPE)
   {
      return 0;
   }
   
   lastTextPayload = (NDEFMessage[0] & NDEF_MESSAGE_END_FLAG);
   
   if (NDEFMessage[0] & NDEF_MESSAGE_SHORT_RECORD)   
   {
      retrieveTextPayloadFromShortRecord( NDEFMessage, type, payload, NDEFMessage[0] & NDEF_MESSAGE_ID_LENGTH_PRESENT);    
   }
   else 
   {
      Serial.println("TODO");
      //@TODO:
   }
}

// Här börjar orginalet!
uint32_t NDEFPushProtocol::rxNDEFPayload(uint8_t *&data)
{
    uint32_t result = _linkLayer->openNPPServerLink();

    if(RESULT_OK(result)) //if connection is error-free
    {
       //Serial.println(F("CONNECTED."));
       result = _linkLayer->serverLinkRxData(data);
       if (RESULT_OK(result))
       {
           NPP_MESSAGE *nppMessage = (NPP_MESSAGE *)data; 
           nppMessage->numNDEFEntries = MODIFY_ENDIAN(nppMessage->numNDEFEntries);
           nppMessage->NDEFLength = MODIFY_ENDIAN(nppMessage->NDEFLength);
           
           if (nppMessage->version != NPP_SUPPORTED_VERSION) 
           {
              Serial.println(F("Recieved an NPP message of an unsupported version."));
              return NPP_UNSUPPORTED_VERSION;
           } 
           else if (nppMessage->numNDEFEntries != 1) 
           {
              Serial.println(F("Invalid number of NDEF entries"));
              return NPP_INVALID_NUM_ENTRIES;
           } 
           else if (nppMessage->actionCode != NPP_ACTION_CODE)
           {
              Serial.println(F("Invalid Action Code"));
              return NPP_INVALID_ACTION_CODE;
           }
           
           _linkLayer->closeNPPServerLink();
           
           //Serial.println(F("Returning NPP Message"));
           //Serial.print(F("Length: "));
           //Serial.println(nppMessage->NDEFLength, HEX);
           data = nppMessage->NDEFMessage;
           return nppMessage->NDEFLength;               
       }            
    }        
    return result;
}

uint32_t NDEFPushProtocol::pushPayload(uint8_t *NDEFMessage, uint32_t length)
{
    NPP_MESSAGE *nppMessage = (NPP_MESSAGE *) ALLOCATE_HEADER_SPACE(NDEFMessage, NPP_MESSAGE_HDR_LEN);
    
    nppMessage->version = NPP_SUPPORTED_VERSION;
    nppMessage->numNDEFEntries = MODIFY_ENDIAN((uint32_t)0x00000001);
    nppMessage->actionCode = NPP_ACTION_CODE;
    nppMessage->NDEFLength = MODIFY_ENDIAN(length);
 
    
    /*uint8_t *buf = (uint8_t *) nppMessage;
    Serial.println(F("NPP + NDEF Message")); 
    for (uint16_t i = 0; i < length + NPP_MESSAGE_HDR_LEN; ++i)
    {
        Serial.print(F("0x")); 
        Serial.print(buf[i], HEX);
        Serial.print(F(" "));
    }*/
    uint32_t result = _linkLayer->openNPPClientLink();
  
    if(RESULT_OK(result)) //if connection is error-free
    { 
        result =  _linkLayer->clientLinkTxData((uint8_t *)nppMessage, length + NPP_MESSAGE_HDR_LEN);
    } 
    
    return result;       
}
