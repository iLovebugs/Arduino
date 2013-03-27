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
#define SNEP_PDU_HEADER_LEN   0x06

struct SNEP_PDU{
   uint8_t version;
   uint8_t type; //Request OR Response
   uint32_t length;
   uint8_t information[0];   
};

class SNEP{
  public:
    SNEP(NFCLinkLayer *);  
    ~SNEP();
    
    // When a link to a client is established
    uint32_t receiveRequest(uint8_t *&NDEFMessage, uint8_t *&request);
    uint32_t transmitSuccsses(uint8_t *NDEFMessage, uint32_t length);
    
    // When a link to a server is established
    uint32_t transmitPutRequest(uint8_t *NDEFMessage, uint32_t length);
    uint32_t receiveResponse(uint8_t *&NDEFMessage, uint8_t responseType);
    
  private:
    NFCLinkLayer *_linkLayer;
};

