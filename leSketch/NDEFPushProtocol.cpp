#include "NDEFPushProtocol.h"

NDEFPushProtocol::NDEFPushProtocol(NFCLinkLayer *linkLayer) :
    _linkLayer(linkLayer)
{
}

NDEFPushProtocol::~NDEFPushProtocol() 
{
}


//Moved from INO-file. Uses NDEF format not SNEP.
uint32_t NDEFPushProtocol::createNDEFShortRecord(uint8_t *message, uint8_t payloadLen, uint8_t *&NDEFMessage)
{
   Serial.print(F("Message: "));
   Serial.println((char *)message); //Unintresing? We know that we want to send 01234?
   uint8_t * NDEFMessageHdr = ALLOCATE_HEADER_SPACE(NDEFMessage, NDEF_SHORT_RECORD_MESSAGE_HDR_LEN);
   
   
   //We need to create a NDEF header, following existing standards.
   NDEFMessageHdr[0] =  NDEF_MESSAGE_BEGIN_FLAG | NDEF_MESSAGE_END_FLAG | NDEF_MESSAGE_SHORT_RECORD | TYPE_FORMAT_MEDIA_TYPE; 
   //MESSAGE_BEGIN_FLAG is set, this is the first record in the NDEF message!
   //MESSAGE_END_FLAG is set, this is the last record of the NDEF message. Thus we send a NDEF message with only one record.
   //MESSAGE_SHORT_RECORD is set, the payloadlentgth field is now one octet.
   //The way this message is created is kinda strange since all the flags are bitwise or:ed together.
   
  //To summerize: We will send a message of with only one record (MB ME flags set) length field is now only one octet (SR flag set length field )
   //and we send Media-type as defined in RFC 2046 by setting TYPE_FORMAT_MEDIA_TYPE
   
   NDEFMessageHdr[1] =  SHORT_RECORD_TYPE_LEN; //Indicates the length of the TYPE field. 0x0A 
   NDEFMessageHdr[2] =  payloadLen;            //This field is now only one octet, That is we can only send messages with max length 255
   memcpy(&NDEFMessageHdr[3], TYPE_STR, SHORT_RECORD_TYPE_LEN); //Adds the TYPE_STR to the record, This message is now a text message!
                                                                //Also since the IL flag was not set in the header this field is not the ID_LENGTH!
   memcpy(NDEFMessage, message, payloadLen); //Copies the message if the lenght is set accordingly.
   NDEFMessage = NDEFMessageHdr; // NDEFMessage now points to first header byte, and subsecuent bytes should also follow.
   return (payloadLen + NDEF_SHORT_RECORD_MESSAGE_HDR_LEN); 
}
// Stängt härifrån
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
                       Serial.println(F("Unhandled NDEF Message Type."));
                       return 0;
                    }
                 }
                 
                 if ( NDEFMessage[idx++] != NFC_FORUM_TEXT_TYPE ) 
                 {
                     Serial.println(F("Unhandled NDEF Message Type."));
                     return 0;
                 }
             }
             else if (type == TYPE_FORMAT_MEDIA_TYPE)
             { 
                 const char *typeStr =  (char *)&NDEFMessage[idx];
                 if (typeLen != 0xA || strncmp(typeStr, "text/plain", typeLen) != 0)
                 {
                    NDEFMessage[typeLen + idx] = NULL;
                    Serial.print(F("Unknown Type: "));
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
      Serial.println(F("TODO"));
      //@TODO:
   }
}

// Här börjar orginalet!
uint32_t NDEFPushProtocol::rxNDEFPayload(uint8_t *&data)
{
    Serial.println(F("<rxNDEFPayload>"));
    uint32_t result = _linkLayer->openNPPServerLink();

    if(RESULT_OK(result)) //if connection is error-free
    {
       Serial.println(F("CONNECTED."));
       result = _linkLayer->serverLinkRxData(data);
       if (RESULT_OK(result))
       {
           NPP_MESSAGE *nppMessage = (NPP_MESSAGE *)data; 
           nppMessage->numNDEFEntries = MODIFY_ENDIAN(nppMessage->numNDEFEntries);
           nppMessage->NDEFLength = MODIFY_ENDIAN(nppMessage->NDEFLength);
           
           if (nppMessage->version != NPP_SUPPORTED_VERSION) 
           {
              //Serial.println(F("Recieved an NPP message of an unsupported version."));
              return NPP_UNSUPPORTED_VERSION;
           } 
           else if (nppMessage->numNDEFEntries != 1) 
           {
              //Serial.println(F("Invalid number of NDEF entries"));
              return NPP_INVALID_NUM_ENTRIES;
           } 
           else if (nppMessage->actionCode != NPP_ACTION_CODE)
           {
              //Serial.println(F("Invalid Action Code"));
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
    Serial.println(F("Error: NO server link"));    
    return result;
}
// stängt hit
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
    uint32_t result =  _linkLayer->openNPPClientLink();
  
    if(RESULT_OK(result)) //if connection is error-free
    { 
        Serial.println(F("pushPayload: Transmitt data to client")); 
        result =  _linkLayer->clientLinkTxData((uint8_t *)nppMessage, length + NPP_MESSAGE_HDR_LEN);
    }
    Serial.println(F("Link open: ")); 
    Serial.println(RESULT_OK(result));
    return result;       
}
