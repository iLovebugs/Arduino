#include "Arduino.h"
#include "NFCLinkLayer.h"

#define SNEP_SUPPORTED_VERSION 0x10

#define SNEP_CONTINUE_REQUEST                 0x00
#define SNEP_GET_REQUEST                      0x01
#define SNEP_PUT_REQUEST                      0x02
#define SNEP_REJECT_REQUEST                   0x03
#define SNEP_SUCCESS                          0x81
#define SNEP_PDU_HEADER_LEN                   0x06

struct SNEP_PDU {
   uint8_t version;
   uint8_t type; //Request OR Response
   uint8_t nothing[3]; //nothing, fixes length issue.
   uint8_t length;
   uint8_t information[0];   
};

class SNEP{
  public:
    SNEP(NFCLinkLayer *);  
    ~SNEP();
    
    // When a link to a client is established
    uint32_t receiveRequest(uint8_t *&NDEFMessage);
    uint32_t transmitSuccess();
    
    // When a link to a server is established
    uint32_t transmitPutRequest(uint8_t *NDEFMessage, uint8_t len);
    uint32_t receiveResponse(uint8_t *&NDEFMessage);
    uint32_t transmitFAIL(uint8_t *NDEFMessage, uint8_t length);
    
  private:
    NFCLinkLayer *_linkLayer;
};

