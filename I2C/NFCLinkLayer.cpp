#include "NFCLinkLayer.h"
#include "MemoryFree.h"

//#define NFCLinkLayerDEBUG 1

NFCLinkLayer::NFCLinkLayer(NFCReader *nfcReader) 
    : _nfcReader(nfcReader)
{
}

NFCLinkLayer::~NFCLinkLayer() 
{

}

// former openNPPClientLink
uint32_t NFCLinkLayer::openLinkToServer(boolean sleep)
{
   PDU targetPayload;
   PDU *receivedPDU;
   uint8_t PDUBuffer[20];
   uint8_t DataIn[64];
   uint32_t result;
   
   #ifdef NFCLinkLayerDEBUG   
      Serial.println(F("SNEP>LLCP>openLinkToServer: Opening link to server."));
   #endif

    receivedPDU = ( PDU *) DataIn; 
   
    _nfcReader-> configurePeerAsTarget(sleep);
   
     //Recieving a SYMM PDU 
     result = _nfcReader->targetRxData(DataIn);
     if (RESULT_OK(result) && isSYMMPDU((PDU *)DataIn))
     {       
       //Creating socket
       DSAP = SNEP_CLIENT;
       SSAP = 0x20;
       seq = 0;
       
       buildConnectPDU(&targetPayload); // built!
       Serial.print(F("SNEP>LLCP>openLinkToServer: Connect PDU: "));
       Serial.print(targetPayload.field[0], HEX);
       Serial.println(targetPayload.field[1], HEX);
             
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
       if (RESULT_OK(result))
       {      
         receivedPDU = (PDU *)DataIn; //We must cast to a PDU, else this would not work. He does this in the clientlink.            
          
         if (receivedPDU->getPTYPE() != CONNECTION_COMPLETE_PTYPE)
         {
            #ifdef NFCLinkLayerDEBUG
               Serial.println(F("SNEP>LLCP>openLinkToServer: \"Connection Complete\" Failed."));
            #endif
            return UNEXPECTED_PDU_FAILURE;
         }
         
         //Old code. 
         //Sets DSAP and SSAP for the established data-link. Will be used when sending I-PDUs
          //   DSAP = receivedPDU->getSSAP();
          //   SSAP = receivedPDU->getDSAP();
       }
     }
     else{
        #ifdef NFCLinkLayerDEBUG
            Serial.println(F("SNEP>LLCP>openLinkToServer: Connection Failed."));
        #endif

        return SYMM_RX_FAILURE;
     }
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
   
   result = _nfcReader->targetTxData((uint8_t *)&disconnect, 2); 
   if(IS_ERROR(result))
   {
     Serial.println(F("SNEP>LLCP>closeLinkToServer: Disconnect Failed."));
     return GEN_ERROR;   
   }
   
   receivedPDU = (PDU *)DataIn;
   result = _nfcReader->targetRxData(DataIn);
   if(IS_ERROR(result) || !isDMPDU(receivedPDU))
    return GEN_ERROR;

   return result;   
}

// openNPPServerLink
uint32_t NFCLinkLayer::openLinkToClient() 
{
   uint8_t status[2]; //wat is dis?
   uint8_t DataIn[64];
   PDU *receivedPDU;
   PDU targetPayload;
   uint32_t result;

   #ifdef NFCLinkLayerDEBUG
      Serial.println(F("SNEP>LLCP>openLinkToClient: Opening Server Link."));
   #endif
   
  _nfcReader-> configurePeerAsTarget();   
   
   
   receivedPDU = (PDU *)DataIn;
   buildSYMMPDU(&targetPayload);   
      
   do 
   {
     Serial.println(F("SNEP>LLCP>openLinkToClient: --- Recive CONNECTION PDU loop ---"));
     result = _nfcReader->targetRxData(DataIn);     
   if (IS_ERROR(result)){
      return result;
   }
     result = _nfcReader->targetTxData((uint8_t*)&targetPayload, SYMM_PDU_LEN);     
   if (IS_ERROR(result)){
      return result;
   }
     
   } while (!receivedPDU->isConnectClientRequest());
   
   //Creating socket
   DSAP = receivedPDU->getSSAP();
   SSAP = receivedPDU->getDSAP();
   seq = 0;
   buildCCPDU(&targetPayload);
 
   Serial.println(F("SNEP>LLCP>openLinkToClient: Trying to send Connection Complete PDU to peer."));
   if (_nfcReader->targetTxData((uint8_t *)&targetPayload, 2) >= 0x80000000)
   {
      #ifdef NFCLinkLayerDEBUG
         Serial.println(F("SNEP>LLCP>openLinkToClient: Connection Complete Failed."));
      #endif
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
   if (IS_ERROR(result) || !isRRPDU(receivedPDU)){
      return result;
   }
   
   buildSYMMPDU(&targetPayload);
   
   result = _nfcReader -> targetTxData((uint8_t *)&targetPayload, SYMM_PDU_LEN);      
   if (IS_ERROR(result)){
      return result;
   }
   
   result = _nfcReader->targetRxData(DataIn);      
   if (IS_ERROR(result) || !isDISCPDU(receivedPDU)){
      return result;
   }
   
   buildDMPDU(&targetPayload);
   
   result = _nfcReader -> targetTxData((uint8_t*)&targetPayload,SYMM_PDU_LEN + 1);
   
   Serial.println(F("SNEP>LLCP>closeLinkToClient: We are now disconnected.")); 
   
   return result;  
}
// former serverLinkRxData
uint32_t NFCLinkLayer::receiveSNEP(uint8_t *&Data){
  PDU targetPayload;
  uint32_t result;
  PDU *receivedPDU;
  
  receivedPDU = (PDU *)Data;
  
  buildSYMMPDU(&targetPayload);
   do{
       Serial.println(F("SNEP>LLCP>receiveSNEP: Trying to get Information-PDU!"));
       //delay(100);
       if(IS_ERROR(_nfcReader->targetTxData((uint8_t *)&targetPayload, 2))){
         Serial.println(F("SNEP>LLCP>receiveSNEP: Failed to receive Information-PDU."));
         return NDEF_MESSAGE_RX_FAILURE;
       }
       if(IS_ERROR(_nfcReader->targetRxData(Data))){
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
      #ifdef NFCLinkLayerDEBUG{
         Serial.println(F("SNEP>LLCP>receiveSNEP: Ack Failed."));
      #endif
      return result;   
   }   
   // Discard the LLCPDU, what remains is the SNEP PDU
   Data = &Data[3];
   
   return RESULT_SUCCESS;
}
// former clientLinkTxData
uint32_t NFCLinkLayer::transmitSNEP(uint8_t *SNEPMessage, uint32_t len)
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
   
    #ifdef NFCLinkLayerDEBUG
      Serial.print(F("SNEP>LLCP>transmitSNEP: Minne:"));
      Serial.println(freeMemory());
    #endif
   if (IS_ERROR(_nfcReader->targetTxData((uint8_t *)infoPDU, len + 3))) // Send the SNEP+PDU
   {
     #ifdef NFCLinkLayerDEBUG
        Serial.println(F("SNEP>LLCP>transmitSNEP: Sending NDEF Message Failed."));
     #endif
     return NDEF_MESSAGE_TX_FAILURE;   
   }   

   
   #ifdef NFCLinkLayerDEBUG
      Serial.println(F("SNEP>LLCP>transmitSNEP: NDEF Message is sent"));
   #endif   
   
     
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
boolean NFCLinkLayer::isSYMMPDU(PDU *pdu)
{
  return (pdu->field[0] == 0) && (pdu->field[1] == 0); 
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

boolean NFCLinkLayer::isCCPDU(PDU *pdu)
{
  return true; 
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

boolean NFCLinkLayer::isConnectPDU(PDU *pdu)
{
  return true; 
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

boolean NFCLinkLayer::isDISCPDU(PDU *pdu)
{
  return pdu->getPTYPE() == DISCONNECT_PTYPE; 
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

boolean NFCLinkLayer::isRRPDU(PDU *pdu)
{
  return pdu->getPTYPE() == RECEIVE_READY_TYPE;
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

boolean NFCLinkLayer::isDMPDU(PDU *pdu)
{
  return pdu->getPTYPE() == DISCONNECTED_MODE_PTYPE; 
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

