#include "NFCLinkLayer.h"

#define NFCLinkLayerDEBUG 1

NFCLinkLayer::NFCLinkLayer(NFCReader *nfcReader) 
    : _nfcReader(nfcReader)
{
}

NFCLinkLayer::~NFCLinkLayer() 
{

}

// former openNPPClientLink
uint32_t NFCLinkLayer::openLinkToServer(boolean debug)
{
   PDU *targetPayload;
   PDU *recievedPDU;
   uint8_t PDUBuffer[20];
   uint8_t DataIn[64];
   
   if (debug)
   {
      Serial.println(F("Opening NPP Client Link."));
   }
   uint32_t result = _nfcReader->configurePeerAsTarget(NPP_CLIENT); // TG_INIT_AS_TARGET

   if (IS_ERROR(result))
   {
       return result;
   }
   
   recievedPDU = ( PDU *) DataIn; 
   if (!_nfcReader->targetRxData(DataIn, true)) // WHAT DO WE GET?
   {
      if (debug)
      {
         Serial.println(F("Connection Failed."));
      }
      return CONNECT_RX_FAILURE;   
   }

   targetPayload = (PDU *) PDUBuffer;
   targetPayload->setDSAP(0x01);
   targetPayload->setPTYPE(CONNECT_PTYPE);
   targetPayload->setSSAP(0x20);
   
   targetPayload->params.type = SERVICE_NAME_PARAM_TYPE;
   targetPayload->params.length = CONNECT_SERVICE_NAME_LEN;
   
   memcpy(targetPayload->params.data, CONNECT_SERVICE_NAME, CONNECT_SERVICE_NAME_LEN);
   
   //TODO: The java man dosent loop. He send an SYMMPDU after the CONNECT PDU. 
   do
   {
       if (IS_ERROR(_nfcReader->targetTxData((uint8_t *)targetPayload, CONNECT_SERVER_PDU_LEN, true))) { // SEND CONNECT PDU
          return CONNECT_TX_FAILURE;   
       }
   } while(IS_ERROR(_nfcReader->targetRxData(DataIn, true))); // Recieve an CCPDU
    
   if (recievedPDU->getPTYPE() != CONNECTION_COMPLETE_PTYPE)
   {
      if (debug)
      {
         Serial.println(F("Connection Complete Failed."));
      }
      return UNEXPECTED_PDU_FAILURE;
   }
   
   DSAP = recievedPDU->getSSAP();
   SSAP = recievedPDU->getDSAP();
   
   return RESULT_SUCCESS;
}
// former closeNPPClientLink
uint32_t NFCLinkLayer::closeLinkToServer(){

}

// openNPPServerLink
uint32_t NFCLinkLayer::openLinkToClient(boolean debug) 
{
   uint8_t status[2];
   uint8_t DataIn[64];
   PDU *recievedPDU;
   PDU targetPayload;
   uint32_t result;

   if (debug)
   {
      Serial.println(F("Opening Server Link."));
   }
   
   result = _nfcReader->configurePeerAsTarget(NPP_CLIENT);
   Serial.println(F("Done configurePeerAsTarget"));
   if (IS_ERROR(result))
   {
       return result;
   }
   
   recievedPDU = (PDU *)DataIn;
   do 
   {
     Serial.println(F("--- Recive CONNECTION PDU loop ---"));
     result = _nfcReader->targetRxData(DataIn, true);
     
     if (debug)
     {
       
        Serial.print(F("Configured as Peer: "));
        Serial.print(F("0x"));
        Serial.println(result, HEX);
     }
     
     
     if (IS_ERROR(result)) 
     {
        return result;   
     }
   } while (!recievedPDU->isConnectClientRequest());
   
   targetPayload = buildCCPDU(targetPayload, recievedPDU);
 
   /*
   //We need to fix this!
   targetPayload.setDSAP(recievedPDU->getSSAP());
   Serial.println(targetPayload.getDSAP(), HEX);
   targetPayload.setPTYPE(CONNECTION_COMPLETE_PTYPE);
   Serial.println(targetPayload.getPTYPE(), HEX);
   targetPayload.setSSAP(recievedPDU->getDSAP());
   Serial.println(targetPayload.getSSAP(), HEX);
   */
   Serial.println(F("Trying to send Connection Complete PDU to peer."));
   if (IS_ERROR(_nfcReader->targetTxData((uint8_t *)&targetPayload, 2, true))) 
   {
      if (debug)
      {
         Serial.println(F("Connection Complete Failed."));
      }
      return CONNECT_COMPLETE_TX_FAILURE;   
   } 
 
   return RESULT_SUCCESS;
}
//former closeNPPServerLink
uint32_t NFCLinkLayer::closeLinkToClient() 
{
   uint8_t DataIn[64];
   PDU *recievedPDU;
   
   recievedPDU = (PDU *)DataIn;
   
   uint32_t result = _nfcReader->targetRxData(DataIn);
   
   if (_nfcReader->isTargetReleasedError(result))
   {
      return RESULT_SUCCESS;
   }
   else if (IS_ERROR(result))
   {
      return result;
   }
   
   //Serial.println(F("Recieved disconnect Message."));
   
   return result;
}
// former serverLinkRxData
uint32_t NFCLinkLayer::receiveFromClient(uint8_t *&Data, boolean debug)
{
  PDU targetPayload;
  uint32_t result;
  PDU *recievedPDU;
  
  recievedPDU = (PDU *)Data;
  buildSYMMPDU(targetPayload);
   do
   {
       Serial.println(F("Trying to get SNEP!"));
       _nfcReader->targetTxData((uint8_t *)&targetPayload, 2, true);
       result = _nfcReader->targetRxData(Data,true);
       
       if(IS_ERROR(result))
       {
         Serial.println(F("Failed to Recieve NDEF Message."));
         return NDEF_MESSAGE_RX_FAILURE;
       }
   }while(recievedPDU->getPTYPE() != INFORMATION_PTYPE);
   
   Serial.println(F("Recieved INFORMATION_PTYPE"));
   PDU ackPDU;
   
   recievedPDU = (PDU *) Data;   

   // Acknowledge reciept of Information PDU
   ackPDU.setDSAP(recievedPDU->getSSAP());
   ackPDU.setPTYPE(RECEIVE_READY_TYPE);
   ackPDU.setSSAP(recievedPDU->getDSAP());
   
   ackPDU.params.sequence = recievedPDU->params.sequence & 0x0F;
   
   result = _nfcReader->targetTxData((uint8_t *)&ackPDU, 3, true);
   if (IS_ERROR(result)) 
   {
      if (debug)
      {
         Serial.println(F("Ack Failed."));
      }
      return result;   
   }   
   // Discard the LLCPDU
   Data = &Data[3];
   
   return RESULT_SUCCESS;
}
// former clientLinkTxData
uint32_t NFCLinkLayer::transmitToServer(uint8_t *nppMessage, uint32_t len, boolean debug)
{
   PDU *infoPDU = (PDU *) ALLOCATE_HEADER_SPACE(nppMessage, 3);
   infoPDU->setDSAP(DSAP);
   infoPDU->setSSAP(SSAP);
   infoPDU->setPTYPE(INFORMATION_PTYPE);
   
   infoPDU->params.sequence = 0; // Create an INFOPDU
   
   /*
   uint8_t *buf = (uint8_t *) infoPDU;
   Serial.println("PDU + NPP + NDEF Message"); 
   for (uint16_t i = 0; i < len + 3; ++i)
   {
       Serial.print(F("0x")); 
       Serial.print(buf[i], HEX);
       Serial.print(F(" "));
   }
   */
    
   if (IS_ERROR(_nfcReader->targetTxData((uint8_t *)infoPDU, len + 3))) // Send the SNEP+PDU
   {
     if (debug)
     {
        Serial.println(F("Sending NDEF Message Failed."));
     }
     return NDEF_MESSAGE_TX_FAILURE;   
   }
   
   PDU disconnect;
   disconnect.setDSAP(DSAP);
   disconnect.setSSAP(SSAP);
   disconnect.setPTYPE(DISCONNECT_PTYPE); // Want to disconnect?
   
   /*if (!_nfcReader->targetTxData((uint8_t *)&disconnect, 2)) 
   {
     Serial.println(F("Disconnect Failed."));
     return false;   
   }*/
   
   if (debug)
   {
      Serial.println(F("Sent NDEF Message"));
   } 
   return RESULT_SUCCESS;
}

inline bool PDU::isConnectClientRequest()
{
    return ((getPTYPE() == CONNECT_PTYPE));
}

void NFCLinkLayer::buildSYMMPDU(PDU targetPayload)
{
  targetPayload.setDSAP(0x00);
  targetPayload.setPTYPE(0x00);
  targetPayload.setSSAP(0x00);
  
  #ifdef NFCLinkLayerDEBUG
  Serial.print("SYMMPDU: ");
  Serial.print(targetPayload.getDSAP(), HEX);
  Serial.print(" ");
  Serial.print(targetPayload.getPTYPE(), HEX);
  Serial.print(" ");
  Serial.println(targetPayload.getSSAP(), HEX);
  #endif
}

PDU NFCLinkLayer::buildCCPDU(PDU targetPayload, PDU *recievedPDU)
{
   targetPayload.setDSAP(recievedPDU -> getSSAP());
   targetPayload.setPTYPE(CONNECTION_COMPLETE_PTYPE);
   targetPayload.setSSAP(recievedPDU -> getDSAP());
   
   #ifdef NFCLinkLayerDEBUG
   Serial.print("CCPDU: ");
   Serial.print(targetPayload.getDSAP(), HEX);
   Serial.print(" ");
   Serial.print(targetPayload.getPTYPE(), HEX);
   Serial.print(" ");
   Serial.println(targetPayload.getSSAP(), HEX);
   #endif
   
   return targetPayload;
}

void NFCLinkLayer::buildConnectPDU(PDU targetPayload)
{
  targetPayload.setDSAP(SNEP_CLIENT);
  targetPayload.setPTYPE(CONNECT_PTYPE);
  targetPayload.setSSAP(0x20);
  
  #ifdef NFCLinkLayerDEBUG
  Serial.print("ConnectPDU: ");
  Serial.print(targetPayload.getDSAP(), HEX);
  Serial.print(" ");
  Serial.print(targetPayload.getPTYPE(), HEX);
  Serial.print(" ");
  Serial.println(targetPayload.getSSAP(), HEX);
  #endif
}

CCPDU::CCPDU()
{
  field[0] = 0;
  field[1] = 0;
  tlvField[0] = 0;
  tlvField[1] = 0;
  tlvField[2] = 0;
  tlvField[3] = 0;
  tlvField[4] = 0;
  tlvField[5] = 0;
  tlvField[6] = 0;
}
PDU::PDU() 
{
   field[0] = 0;
   field[1] = 0;
   params.type = 0;
   params.length = 0;
}

uint8_t PDU::getDSAP() 
{
   return (field[0] >> 2);
}

uint8_t PDU::getSSAP() 
{
   return (field[1] & 0x3F);
}

uint8_t PDU::getPTYPE()
{
   return (((field[0] & 0x03) << 2) | ((field[1] & 0xC0) >> 6));
}

void PDU::setDSAP(uint8_t DSAP)
{
   field[0] &= 0x03; // Clear the DSAP bits 
   field[0] |= ((DSAP & 0x3F) << 2);  // Set the bits
}

void PDU::setSSAP(uint8_t SSAP)
{
  field[1] &= (0xC0);    // Clear the SSAP bits
  field[1] |= (0x3F & SSAP);
}

void PDU::setPTYPE(uint8_t PTYPE)
{
   field[0] &= 0xFC; // Clear the last two bits that contain the PTYPE
   field[1] &= 0x3F; // Clear the upper two bits that contain the PTYPE
   field[0] |= (PTYPE & 0x0C) >> 2;
   field[1] |= (PTYPE & 0x03) << 6;
}
