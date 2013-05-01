#include "Arduino.h"
#include "NFCReader.h"


#define SHORT_RECORD_TYPE_LEN   0x0A
#define NDEF_SHORT_RECORD_MESSAGE_HDR_LEN   0x03 + SHORT_RECORD_TYPE_LEN
#define TYPE_STR "text/plain"

#define NDEF_MESSAGE_BEGIN_FLAG          0x80
#define NDEF_MESSAGE_END_FLAG            0x40
#define NDEF_MESSAGE_CHUNK_FLAG          0X20
#define NDEF_MESSAGE_SHORT_RECORD        0X10
#define NDEF_MESSAGE_ID_LENGTH_PRESENT   0X08
#define NDEF_MESSAGE_TYPENAME_FORMAT     0x07


#define TYPE_FORMAT_EMPTY             0x00
#define TYPE_FORMAT_NFC_FORUM_TYPE    0x01
#define TYPE_FORMAT_MEDIA_TYPE        0x02
#define TYPE_FORMAT_ABSOLUTE_URI      0x03
#define TYPE_FORMAT_NFC_FORUM_EXTERNAL_TYPE    0x04
#define TYPE_FORMAT_UNKNOWN_TYPE               0x05
#define TYPE_FORMAT_UNCHANGED_TYPE             0x06
#define TYPE_FORMAT_RESERVED_TYPE              0x07  


#define NFC_FORUM_TEXT_TYPE        0x54      

enum NDEFState 
{
    NDEF_TYPE_LEN,
    NDEF_PAYLOAD_LEN,
    NDEF_ID_LEN,
    NDEF_TYPE,
    NDEF_ID,
    NDEF_PAYLOAD,
    NDEF_FINISHED    
};

class NDEFMessage{
  public:
    NDEFMessage() ;  
    //~NDEFMessage();
    uint32_t retrieveTextPayloadFromShortRecord(uint8_t *NDEFMessage, uint8_t type, uint8_t *&payload, boolean isIDLenPresent); 
    uint32_t retrieveTextPayload(uint8_t *NDEFMessage, uint8_t *&payload, boolean &lastTextPayload);
    uint32_t createNDEFShortRecord(uint8_t *&NDEFMessage, uint8_t payloadLen); 
};

