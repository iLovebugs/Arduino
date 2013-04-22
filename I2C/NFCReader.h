#include "Arduino.h"

#ifndef NFC_READER_H
#define NFC_READER_H

#define NFC_READER_CFG_BAUDRATE_106_KPS   0
#define NFC_READER_CFG_BAUDRATE_201_KPS   1
#define NFC_READER_CFG_BAUDRATE_424_KPS   2


#define NPP_CLIENT  1
#define NPP_SERVER  2

enum RESULTS 
{
   RESULT_SUCCESS = 0x00000001,
   GEN_ERROR = 0x80000000,
   CONFIGURE_HARDWARE_ERROR = 0x80000001,
   NFC_READER_COMMAND_FAILURE = 0x80000002,
   NFC_READER_RESPONSE_FAILURE = 0x80000003,
   SYMM_RX_FAILURE = 0x80000010, //Added errors for sending and recieving SYMM-PDUS
   SYMM_TX_FAILURE = 0x80000011,
   CONNECT_RX_FAILURE = 0x80000020,
   CONNECT_TX_FAILURE = 0x80000021,
   CONNECT_COMPLETE_RX_FAILURE = 0x80000022,
   CONNECT_COMPLETE_TX_FAILURE = 0x80000023,
   UNEXPECTED_PDU_FAILURE = 0x80000030,
   NDEF_MESSAGE_RX_FAILURE = 0x80000040,
   NDEF_MESSAGE_TX_FAILURE = 0x80000041,
   SNEP_UNSUPPORTED_VERSION = 0x80000050,
   SNEP_UNEXPECTED_RESPONSE = 0x80000051, //Added errors for all responses except SUCCESS
   SNEP_UNEXPECTED_REQUEST = 0x80000052,
   SEND_COMMAND_TX_TIMEOUT_ERROR = 0x80000060,
   SEND_COMMAND_RX_ACK_ERROR = 0x80000061,
   SEND_COMMAND_RX_TIMEOUT_ERROR = 0x80000062,
   FETCH_COMMAND_RX_TIMEOUT_ERROR = 0x80000063,
   INVALID_CHECKSUM_RX = 0x80000070,
   INVALID_RESPONSE = 0x80000071,
   INVALID_POSTAMBLE = 0x80000072,
   CLIENT_REQ_ERROR = 0x80000080
        
};


#define IS_ERROR(result) (result >= 0x80000000)
#define RESULT_OK(result) (result < 0x80000000)
class NFCReader {

public:
   virtual void initializeReader() = 0;
   virtual uint32_t SAMConfig(void) = 0;
   virtual uint32_t getFirmwareVersion(void) = 0;
    
   virtual uint32_t configurePeerAsTarget(boolean sleep = false) = 0;

   
   virtual uint32_t targetRxData(uint8_t *response) = 0;
   virtual uint32_t targetTxData(uint8_t *DataOut, uint32_t dataSize) = 0;                      
                                                                      
   virtual uint32_t sendCommandCheckAck(uint8_t *cmd, 
                              uint8_t cmdlen,                               
                              uint16_t timeout = 1000,
                              boolean debug = false) = 0;	
   
   virtual boolean isTargetReleasedError(uint32_t result) = 0;
    
   virtual void clearBuffer() = 0;

};


#define ALLOCATE_HEADER_SPACE(buffer, numHdrBytes)  &buffer[-((int)numHdrBytes)];
#endif // NFC_READER_H
