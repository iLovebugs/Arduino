#include "NFCLinkLayer.h"
#include "MemoryFree.h"

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
   PDU targetPayload;
   PDU *receivedPDU;
   uint8_t PDUBuffer[20];
   uint8_t DataIn[64];
   
   if (debug)
   {
      Serial.println(F("SNEP>LLCP>openLinkToServer: Opening link to server."));
   }
   uint32_t result = _nfcReader->configurePeerAsTarget(NPP_CLIENT); // TG_INIT_AS_TARGET

   if (IS_ERROR(result))
   {
       return result;
   }
   
   
  receivedPDU = ( PDU *) DataIn; 
 
   //Recieving a SYMM PDU 
   if (!_nfcReader->targetRxData(DataIn))  
   {
      if (debug)
      {
         Serial.println(F("SNEP>LLCP>openLinkToServer: Connection Failed."));
      }
      return CONNECT_RX_FAILURE;   
   }
   
   //Creating socket
   DSAP = SNEP_CLIENT;
   SSAP = 0x20;
   seq = 0;
   
   buildConnectPDU(&targetPayload); // built!
   Serial.println(F("SNEP>LLCP>openLinkToServer: Connect PDU: "));
   Serial.print(targetPayload.field[0], HEX);
   Serial.print(targetPayload.field[1], HEX);
         
   //Copying Java-man, sedning a SYMM after we have sent a Connect-pdu.
   
    result = _nfcReader->targetTxData((uint8_t *)&targetPayload, CONNECT_SERVER_PDU_LEN); // SEND CONNECT PDU
       
    if(IS_ERROR(result)){
        return CONNECT_TX_FAILURE;   
    }
    
    buildSYMMPDU(&targetPayload);
   
   result = _nfcReader -> targetTxData((uint8_t *)&targetPayload, SYMM_PDU_LEN);
   
   if(IS_ERROR(result)){
    return SYMM_TX_FAILURE;
   }       
   
  _nfcReader->targetRxData(DataIn); // receive an CCPDU
  
   receivedPDU = (PDU *)DataIn; //We must cast to a PDU, else this would not work. He does this in the clientlink.
      
    
   if (receivedPDU->getPTYPE() != CONNECTION_COMPLETE_PTYPE)
   {
      if (debug)
      {
         Serial.println(F("SNEP>LLCP>openLinkToServer: \"Connection Complete\" Failed."));
      }
      return UNEXPECTED_PDU_FAILURE;
   }
   
   //Old code. 
   //Sets DSAP and SSAP for the established data-link. Will be used when sending I-PDUs
    //   DSAP = receivedPDU->getSSAP();
    //   SSAP = receivedPDU->getDSAP();
   
   return RESULT_SUCCESS;
}

// former closeNPPClientLink
//Must be called after recieving SNEP response code. Client closes the link!
uint32_t NFCLinkLayer::closeLinkToServer(){
   uint8_t DataIn[64];
   PDU *receivedPDU;
   uint32_t result;
  
   PDU disconnect;
   
   buildDISCPDU(&disconnect);
   
   if (!_nfcReader->targetTxData((uint8_t *)&disconnect, 2)) 
   {
     Serial.println(F("SNEP>LLCP>closeLinkToServer: Disconnect Failed."));
     return false;   
   }
   
   receivedPDU = (PDU *)DataIn;
   result = _nfcReader->targetRxData(DataIn);   
}

// openNPPServerLink
uint32_t NFCLinkLayer::openLinkToClient(boolean debug) 
{
   uint8_t status[2]; //wat is dis?
   uint8_t DataIn[64];
   PDU *receivedPDU;
   PDU targetPayload;
   uint32_t result;

   if (debug)
   {
      Serial.println(F("SNEP>LLCP>openLinkToClient: Opening Server Link."));
   }
   
   result = _nfcReader->configurePeerAsTarget(NPP_CLIENT);
   Serial.println(F("SNEP>LLCP>openLinkToClient: Done configurePeerAsTarget"));
   if (IS_ERROR(result))
   {
       return result;
   }
   
   receivedPDU = (PDU *)DataIn;
   buildSYMMPDU(&targetPayload);   
      
   do 
   {
     Serial.println(F("SNEP>LLCP>openLinkToClient: --- Recive CONNECTION PDU loop ---"));
     result = _nfcReader->targetRxData(DataIn);
     _nfcReader->targetTxData((uint8_t*)&targetPayload, SYMM_PDU_LEN);
     
     
     if (debug)
     {
       
        Serial.print(F("SNEP>LLCP>openLinkToClient: Configured as Peer: "));
        Serial.print(F("0x"));
        Serial.println(result, HEX);
     }
     
     
     if (IS_ERROR(result)) 
     {
        return result;   
     }
   } while (!receivedPDU->isConnectClientRequest());
   
   //Creating socket
   DSAP = receivedPDU->getSSAP();
   SSAP = receivedPDU->getDSAP();
   seq = 0;
   buildCCPDU(&targetPayload);
 
   Serial.println(F("SNEP>LLCP>openLinkToClient: Trying to send Connection Complete PDU to peer."));
   if (IS_ERROR(_nfcReader->targetTxData((uint8_t *)&targetPayload, 2))) //change this 2 to a predefined value CC_LEN ? 
   {
      if (debug)
      {
         Serial.println(F("SNEP>LLCP>openLinkToClient: Connection Complete Failed."));
      }
      return CONNECT_COMPLETE_TX_FAILURE;   
   } 
 
   return RESULT_SUCCESS;
}

//former closeNPPServerLink
uint32_t NFCLinkLayer::closeLinkToClient() 
{
   uint8_t DataIn[64];
   PDU *receivedPDU;
   PDU targetPayload;
   uint32_t result;
   
   receivedPDU = (PDU *)DataIn;
   
   result = _nfcReader->targetRxData(DataIn);
   
   if (_nfcReader->isTargetReleasedError(result))
   {
      return RESULT_SUCCESS;
   }
   else if (IS_ERROR(result))
   {
      return result;
   }
   
   buildSYMMPDU(&targetPayload);
   
   result = _nfcReader -> targetTxData((uint8_t *)&targetPayload, SYMM_PDU_LEN);   
   
   result = _nfcReader->targetRxData(DataIn);   
   
   buildDMPDU(&targetPayload);
   
   result = _nfcReader -> targetTxData((uint8_t*)&targetPayload,SYMM_PDU_LEN + 1);
   
   Serial.println(F("SNEP>LLCP>closeLinkToClient: We are now disconnected.")); 
   
   return result;  
}
// former serverLinkRxData
uint32_t NFCLinkLayer::receiveSNEP(uint8_t *&Data, boolean debug){
  PDU targetPayload;
  uint32_t result;
  PDU *receivedPDU;
  
  receivedPDU = (PDU *)Data;
  
  buildSYMMPDU(&targetPayload);
   do{
       Serial.println(F("SNEP>LLCP>receiveSNEP: Trying to get Information-PDU!"));
       _nfcReader->targetTxData((uint8_t *)&targetPayload, 2);
       result = _nfcReader->targetRxData(Data);
       
       if(IS_ERROR(result)){
         Serial.println(F("SNEP>LLCP>receiveSNEP: Failed to receive Information-PDU."));
         return NDEF_MESSAGE_RX_FAILURE;
       }
   }while(receivedPDU->getPTYPE() != INFORMATION_PTYPE);
   
   Serial.println(F("SNEP>LLCP>receiveSNEP: Received INFORMATION_PTYPE"));
   /*if(params[1] == 0x81){
     Serial.println(F("SNEP>LLCP>receiveSNEP: Received 0x81 SUCCESS"));
   }*/
   
   increaceReceiveWindow(); //Receive window increaced, we recieved a number llc pdu.
   
   buildRRPDU(&targetPayload);
   result = _nfcReader->targetTxData((uint8_t *)&targetPayload, 3);
   if (IS_ERROR(result)) 
   {
      if (debug)
      {
         Serial.println(F("SNEP>LLCP>receiveSNEP: Ack Failed."));
      }
      return result;   
   }   
   // Discard the LLCPDU, what remains is the SNEP PDU
   Data = &Data[3];
   
   return RESULT_SUCCESS;
}
// former clientLinkTxData
uint32_t NFCLinkLayer::transmitSNEP(uint8_t *SNEPMessage, uint32_t len, boolean debug)
{
   PDU *infoPDU = (PDU *) ALLOCATE_HEADER_SPACE(SNEPMessage, 3);
   infoPDU->setDSAP(DSAP);
   infoPDU->setSSAP(SSAP);
   infoPDU->setPTYPE(INFORMATION_PTYPE);
   
   infoPDU->setSeq(seq); // Setting the sequence number, must increment!
   increaceSendWindow(); //Increasing send window.      
   
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
   
    if (debug)
     {
    Serial.print(F("SNEP>LLCP>transmitSNEP: Minne:"));
    Serial.println(freeMemory());
     }
   if (IS_ERROR(_nfcReader->targetTxData((uint8_t *)infoPDU, len + 3))) // Send the SNEP+PDU
   {
     if (debug)
     {
        Serial.println(F("SNEP>LLCP>transmitSNEP: Sending NDEF Message Failed."));
     }
     return NDEF_MESSAGE_TX_FAILURE;   
   }   

   
   if (debug)
   {
      Serial.println(F("SNEP>LLCP>transmitSNEP: NDEF Message is sent"));
   }   
   
     
   return RESULT_SUCCESS;
}

inline bool PDU::isConnectClientRequest()
{
    return ((getPTYPE() == CONNECT_PTYPE));
}

void NFCLinkLayer::buildSYMMPDU(PDU *targetPayload)
{
  targetPayload->setDSAP(0x00);
  targetPayload->setPTYPE(0x00);
  targetPayload->setSSAP(0x00);
  
  #ifdef NFCLinkLayerDEBUG
  Serial.print("SNEP>LLCP>....buildSYMMPDU: SYMMPDU: ");
  Serial.print(targetPayload->getDSAP(), HEX);
  Serial.print(" ");
  Serial.println(targetPayload->getPTYPE(), HEX);
  #endif
}

void NFCLinkLayer::buildCCPDU(PDU *targetPayload)
{
   targetPayload->setDSAP(DSAP);
   targetPayload->setPTYPE(CONNECTION_COMPLETE_PTYPE);
   targetPayload->setSSAP(SSAP);
   
   #ifdef NFCLinkLayerDEBUG
   Serial.print("SNEP>LLCP>....buildCCPDU: CCPDU: ");
   Serial.print(targetPayload->getDSAP(), HEX);
   Serial.print(" ");
   Serial.print(targetPayload->getPTYPE(), HEX);
   Serial.print(" ");
   Serial.println(targetPayload->getSSAP(), HEX);
   #endif
}

void NFCLinkLayer::buildConnectPDU(PDU *targetPayload)
{
  targetPayload->setDSAP(DSAP);
  targetPayload->setPTYPE(CONNECT_PTYPE);
  targetPayload->setSSAP(SSAP);
  
  #ifdef NFCLinkLayerDEBUG
  Serial.print("SNEP>LLCP>....buildConnectPDU: ConnectPDU: ");
  Serial.print(targetPayload->getDSAP(), HEX);
  Serial.print(" ");
  Serial.print(targetPayload->getPTYPE(), HEX);
  Serial.print(" ");
  Serial.println(targetPayload->getSSAP(), HEX);
  #endif
}

void NFCLinkLayer::buildDISCPDU(PDU *targetPayload)
{
  targetPayload->setDSAP(DSAP);
  targetPayload->setPTYPE(DISCONNECT_PTYPE);
  targetPayload->setSSAP(SSAP);
  
  #ifdef NFCLinkLayerDEBUG
  Serial.print("SNEP>LLCP>....buildDISCPDU: DISCPDU: ");
  Serial.print(targetPayload->getDSAP(), HEX);
  Serial.print(" ");
  Serial.print(targetPayload->getPTYPE(), HEX);
  Serial.print(" ");
  Serial.println(targetPayload->getSSAP(), HEX);
  #endif 
}

void NFCLinkLayer::buildRRPDU(PDU *targetPayload)
{
   targetPayload->setDSAP(DSAP);
   targetPayload->setPTYPE(RECEIVE_READY_TYPE);
   targetPayload->setSSAP(SSAP);
   targetPayload->setSeq(0x0F & seq); //ONLY Recieve window!!  
  #ifdef NFCLinkLayerDEBUG
  Serial.print("SNEP>LLCP>....buildRRPDU: RRPDU: ");
  Serial.print(targetPayload->getDSAP(), HEX);
  Serial.print(" ");
  Serial.print(targetPayload->getPTYPE(), HEX);
  Serial.print(" ");
  Serial.println(targetPayload->getSSAP(), HEX);
  #endif 
}

void NFCLinkLayer::buildDMPDU(PDU *targetPayload)
{
   targetPayload->setDSAP(DSAP);
   targetPayload->setPTYPE(DISCONNECTED_MODE_PTYPE);
   targetPayload->setSSAP(SSAP);
   targetPayload->params.data[0] = 0x00;
   
  #ifdef NFCLinkLayerDEBUG
  Serial.print("SNEP>LLCP>....buildDMPDU: DMPDU: ");
  Serial.print(targetPayload->getDSAP(), HEX);
  Serial.print(" ");
  Serial.print(targetPayload->getPTYPE(), HEX);
  Serial.print(" ");
  Serial.println(targetPayload->getSSAP(), HEX);
  #endif   
}

void NFCLinkLayer::increaceSendWindow(){
  seq = seq + 0x10;  
}

void NFCLinkLayer::increaceReceiveWindow(){
  seq = seq + 0x01; 
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

void PDU::setSeq(uint8_t seq){
 params.sequence = seq; 
}

uint8_t PDU::getSeq(){
 return params.sequence; 
}

