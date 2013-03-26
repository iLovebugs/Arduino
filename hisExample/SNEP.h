#include "Arduino.h"
#include "NFCLinkLayer.h"

#define SNEP_H

#define SNEP_SUPPORTED_VERSION 0x10

#define MODIFY_ENDIAN(x) ((((x) >> 24) & 0xFF)         /* move byte 3 to byte 0 */ \
                                   | (((x) << 24) & 0xFF000000) /* move byte 0 to byte 3 */ \
                                   | (((x) << 8)  & 0xFF0000)   /* move byte 1 to byte 2 */ \
                                   | (((x) >> 8)  & 0xFF00))    /* move byte 2 to byte 1 */         


#define SNEP_CONTINUE_REQUEST                 0x00
#define SNEP_GET_REQUEST                      0x01
#define SNEP_PUT_REQUEST                      0x02
#define SNEP_REJECT_REQUEST                   0x03
#define SNEP_SUCCESS                          0x81

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


#define SNEP_PDU_HEADER_LEN   0x06
#define SNEP_GET_REQ_HEADER_LEN       0x0A //INCLUDES Acceptable Length
#define SNEP_ACCEPTABLE_LENGTH    0x80 //128-bits

struct SNEP_PDU{
   uint8_t version;
   uint8_t type; //Request OR Response
   uint32_t length;
   uint8_t information[0];   
};

struct SNEP_GET_REQ_PDU{
   uint8_t version;
   uint8_t type; //Request OR Response
   uint32_t length;
   uint32_t acceptableLength;
   uint8_t information[0];
};   

class SNEP{

  public:
    SNEP(NFCLinkLayer *);  
    ~SNEP();
    
    // When a link to a client is established
    uint32_t receiveRequest(uint8_t *&NDEFMessage, uint8_t *&request);
    uint32_t transmitResponse(uint8_t *NDEFMessage, uint32_t length, uint8_t * responseType);
    
    // When a link to a server is established
    uint32_t transmitRequest(uint8_t *NDEFMessage, uint32_t length, uint8_t request);
    uint32_t receiveResponse(uint8_t *&NDEFMessage, uint8_t responseType);
    
  private:
    NFCLinkLayer *_linkLayer;
};

