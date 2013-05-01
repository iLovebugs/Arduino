#include "NDEFMessage.h"

NDEFMessage::NDEFMessage(){
}

uint32_t NDEFMessage::createNDEFShortRecord(uint8_t *&NDEFMessage ,uint8_t payloadLen)
{
   //Serial.print("Message: ");
   //Serial.println((char *)message);
   uint8_t * NDEFMessageHdr = ALLOCATE_HEADER_SPACE(NDEFMessage, NDEF_SHORT_RECORD_MESSAGE_HDR_LEN);
   
   NDEFMessageHdr[0] =  NDEF_MESSAGE_BEGIN_FLAG | NDEF_MESSAGE_END_FLAG | NDEF_MESSAGE_SHORT_RECORD | TYPE_FORMAT_MEDIA_TYPE; 
   NDEFMessageHdr[1] =  SHORT_RECORD_TYPE_LEN;
   NDEFMessageHdr[2] =  payloadLen;
   memcpy(&NDEFMessageHdr[3], TYPE_STR, SHORT_RECORD_TYPE_LEN);
   //Serial.print("NDEF Message: ");
   //Serial.println((char *)NDEFMessage);   
   NDEFMessage = NDEFMessageHdr;
   return (payloadLen + NDEF_SHORT_RECORD_MESSAGE_HDR_LEN);   
}


//This method checks the NDEF header. The header MUST be of MEDIA_TYPE else the NDEF-message will be discarded.
//Will call methods that extracts information from short record or normal records. Only the short record method is implemented.
uint32_t NDEFMessage::retrieveTextPayload(uint8_t *NDEFMessage, uint8_t *&payload, boolean &lastTextPayload)
{
   Serial.print(F("NDEFMessage::retrieveTextPayload:"));
   Serial.println(NDEFMessage[0], HEX);
   Serial.print(F("NDEFMessage::retrieveTextPayload:"));
   Serial.println(NDEFMessage[1], HEX);
   uint8_t type = (NDEFMessage[0] & NDEF_MESSAGE_TYPENAME_FORMAT); //applying a mask 0000 0111 to extract the TNF field.
   if (type != TYPE_FORMAT_NFC_FORUM_TYPE && type != TYPE_FORMAT_MEDIA_TYPE) //If result is not 0000 0010 = mediatype we will not process the NDEF-message
   {
      return 0;
   }
   
   lastTextPayload = (NDEFMessage[0] & NDEF_MESSAGE_END_FLAG); //When is this used? Checks wether this is the last record by applying mask 0100 0000.
   
   if (NDEFMessage[0] & NDEF_MESSAGE_SHORT_RECORD) //Checks if SR flag is set, if so extract information from the payload.    
   {
      retrieveTextPayloadFromShortRecord( NDEFMessage, type, payload, NDEFMessage[0] & NDEF_MESSAGE_ID_LENGTH_PRESENT);    
   }
   else 
   {
      //@TODO: We could remove this part, our application will not use non SR NDEF-messages
   }
}



//This function parses the ShortRecord, extracting the information.
uint32_t NDEFMessage::retrieveTextPayloadFromShortRecord(uint8_t *NDEFMessage, uint8_t type, uint8_t *&payload, boolean isIDLenPresent)
{
   NDEFState currentState =  NDEF_TYPE_LEN; //This is the first field after the NDEF header.
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
             typeLen = NDEFMessage[idx++]; //Saves the typeLen for future use
             currentState = NDEF_PAYLOAD_LEN;
          }
          // Purposefully allowing it to fall through
          case NDEF_PAYLOAD_LEN:
          {
             payloadLen = NDEFMessage[idx++]; //Saves the payload for later extraction
             currentState =  isIDLenPresent ? NDEF_ID_LEN : NDEF_TYPE; //Checks whether ID field is present, if not advance to NDEF_TYPE, else advance to the NDEF_ID_LEN.
          }
          break; //Break?
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
                       Serial.println("NDEFMessage::retrieveTextPayloadFromShortRecord: Unhandled NDEF Message Type.");
                       return 0;
                    }
                 }
                 
                 if ( NDEFMessage[idx++] != NFC_FORUM_TEXT_TYPE ) 
                 {
                     Serial.println("NDEFMessage::retrieveTextPayloadFromShortRecord: Unhandled NDEF Message Type.");
                     return 0;
                 }
             }
             else if (type == TYPE_FORMAT_MEDIA_TYPE) //This will be the our case. We must check if len is ok and if the value is text/plain
             { 
                 const char *typeStr =  (char *)&NDEFMessage[idx];
                 if (typeLen != 0xA || strncmp(typeStr, "text/plain", typeLen) != 0) 
                 {
                    NDEFMessage[typeLen + idx] = NULL;
                    Serial.print("NDEFMessage::retrieveTextPayloadFromShortRecord: Unknown Type: ");
                    Serial.println(NDEFMessage[idx]);
                    return 0;
                 }
                 idx += typeLen; //Confirmed the mediatype to be text/plain, advance idx to the actuall payload.
                 
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
              payload = &NDEFMessage[idx]; //Succesfully extracted the payload from the SR.
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
