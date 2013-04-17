#include "Arduino.h"
#include "NFCReader.h"

#ifndef NFC_LINK_LAYER_H
#define NFC_LINK_LAYER_H

#define SYMM_PTYPE                    0x00
#define PAX_PTYPE                     0x01
#define AGGREGATE_PTYPE               0x02
#define UNNUMBERED_INFORMATION_PTYPE  0x03
#define CONNECT_PTYPE                 0x04
#define DISCONNECT_PTYPE              0x05
#define CONNECTION_COMPLETE_PTYPE     0x06
#define DISCONNECTED_MODE_PTYPE       0x07
#define FRAME_REJECT_PTYPE            0X08
#define INFORMATION_PTYPE             0x0C
#define RECEIVE_READY_TYPE            0x0D
#define RECEIVE_NOT_READY_TYPE        0x0E

#define SERVICE_NAME_PARAM_TYPE      0x06


#define SNEP_CLIENT                  0x04

#define CONNECT_SERVER_PDU_LEN       0x02
#define SYMM_PDU_LEN                 0x02


struct PARAMETER_DESCR {
   union {
      uint8_t type;
      uint8_t sequence;
   };
   uint8_t length;
   uint8_t data[0];
};

struct PDU {
public:
   uint8_t field[2]; 
   PARAMETER_DESCR  params;

   //Functions:
   PDU();
   
   uint8_t getDSAP();
   uint8_t getSSAP();
   uint8_t getPTYPE();
   uint8_t getSeq();

   void setDSAP(uint8_t DSAP);
   void setSSAP(uint8_t SSAP);
   void setPTYPE(uint8_t PTYPE);
   void setSeq(uint8_t seq);
   bool isConnectClientRequest();
};

class NFCLinkLayer {
public:
   NFCLinkLayer(NFCReader *nfcReader);
   ~NFCLinkLayer();
   
   uint32_t openLinkToClient(boolean debug = true);
   uint32_t closeLinkToClient();
      
   uint32_t openLinkToServer(boolean debug = true);
   uint32_t closeLinkToServer();
   
   uint32_t receiveSNEP(uint8_t *&Data, boolean debug = true);
   uint32_t transmitSNEP(uint8_t *SNEPMessage, uint32_t len, boolean debug = true);
private:
   NFCReader *_nfcReader;
   uint8_t DSAP;
   uint8_t SSAP;
   uint8_t seq;
   
   void buildCCPDU(PDU *targetPayload);
   void buildConnectPDU(PDU *targetPayload);
   void buildSYMMPDU(PDU *targetPayload);
   void buildDISCPDU(PDU *targetPayload);
   void buildRRPDU(PDU *targetPayload);
   void buildDMPDU(PDU *targetPayload);
   void increaceSendWindow();
   void increaceReceiveWindow();
};


#endif
