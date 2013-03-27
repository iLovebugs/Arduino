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

class NDEFMessage{
  
 public uint32_t retrieveTextPayloadFromShortRecord(uint8_t *NDEFMessage, uint8_t type, uint8_t *&payload, boolean isIDLenPresent); 
 public uint32_t retrieveTextPayload(uint8_t *NDEFMessage, uint8_t *&payload, boolean &lastTextPayload);
 public uint32_t createNDEFShortRecord(uint8_t *message, uint8_t payloadLen, uint8_t *&NDEFMessage);
 
}

