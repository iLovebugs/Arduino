#include "Arduino.h"

#ifndef NFC_READER_H
#define NFC_READER_H

#define NFC_READER_CFG_BAUDRATE_106_KPS   0
#define NFC_READER_CFG_BAUDRATE_201_KPS  1
#define NFC_READER_CFG_BAUDRATE_424_KPS   2


#define NPP_CLIENT  1
#define NPP_SERVER  2

enum RESULTS 
{
   RESULT_SUCCESS = 0x00000001,
   GEN_ERROR = 0x80000000,
   CONFIGURE_HARDWARE_ERROR,
   NFC_READER_COMMAND_FAILURE,
   NFC_READER_RESPONSE_FAILURE,
   SYMM_RX_FAILURE, //Added errors for sending and recieving SYMM-PDUS
   SYMM_TX_FAILURE,
   CONNECT_RX_FAILURE,
   CONNECT_TX_FAILURE,
   CONNECT_COMPLETE_RX_FAILURE,
   CONNECT_COMPLETE_TX_FAILURE,
   UNEXPECTED_PDU_FAILURE,
   NDEF_MESSAGE_RX_FAILURE,
   NDEF_MESSAGE_TX_FAILURE,
   SNEP_UNSUPPORTED_VERSION,
   SNEP_UNEXPECTED_RESPONSE, //Added errors for all responses except SUCCESS
   SNEP_UNEXPECTED_REQUEST,
   NPP_UNSUPPORTED_VERSION,
   NPP_INVALID_NUM_ENTRIES,
   NPP_INVALID_ACTION_CODE,
   SEND_COMMAND_TX_TIMEOUT_ERROR,
   SEND_COMMAND_RX_ACK_ERROR,
   SEND_COMMAND_RX_TIMEOUT_ERROR,
   INVALID_CHECKSUM_RX,
   INVALID_RESPONSE,
   INVALID_POSTAMBLE,
   CLIENT_REQ_ERROR
        
};


#define IS_ERROR(result) ((result) & 0x80000000)
#define RESULT_OK(result) !IS_ERROR(result)
class NFCReader {

public:
   virtual void initializeReader() = 0;
   virtual uint32_t SAMConfig(void) = 0;
   virtual uint32_t getFirmwareVersion(void) = 0;
    
   virtual uint32_t configurePeerAsTarget(uint8_t type) = 0;   

   
   virtual uint32_t targetRxData(uint8_t *response) = 0;
   virtual uint32_t targetTxData(uint8_t *DataOut, uint32_t dataSize) = 0;                      
                                                                      
   virtual uint32_t sendCommandCheckAck(uint8_t *cmd, 
                              uint8_t cmdlen,                               
                              uint16_t timeout = 1000,
                              boolean debug = false) = 0;	
   
   virtual boolean isTargetReleasedError(uint32_t result) = 0;   
      
};


#define ALLOCATE_HEADER_SPACE(buffer, numHdrBytes)  &buffer[-((int)numHdrBytes)];
#endif // NFC_READER_H
