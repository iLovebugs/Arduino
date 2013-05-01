#include "NFCLinkLayer.h"
#include "MemoryFree.h"

#define NFCLinkLayerDEBUG


uint8_t PDUout[20];
uint8_t PDUin[64];
PDU *receivedPDU;
uint32_t resultLLC;
PDU * targetPayload;
uint8_t* PDUoutPtr;

NFCLinkLayer::NFCLinkLayer(NFCReader *nfcReader) 
    : _nfcReader(nfcReader)
{
  PDUoutPtr = (uint8_t*)&PDUout;
  DSAP = 0x00;
  SSAP = 0x00;
  seq  = 0x00;
}

NFCLinkLayer::~NFCLinkLayer() 
{
}

// former openNPPClientLink
uint32_t NFCLinkLayer::openLinkToServer(boolean sleep)
{   
   
   #ifdef NFCLinkLayerDEBUG   
      Serial.println(F("SNEP>LLCP>openLinkToServer: Opening link to server."));
   #endif

    receivedPDU = ( PDU *) PDUin; 
   
    _nfcReader-> configurePeerAsTarget(sleep);
   
     //Recieving a SYMM PDU 
     resultLLC = _nfcReader->targetRxData(PDUin);
     if (RESULT_OK(resultLLC) && isSYMMPDU(receivedPDU))
     {       
       //Creating socket
       DSAP = SNEP_CLIENT;
       SSAP = 0x20;
       seq = 0;
              
       buildConnectPDU();
             
       //Copying Java-man, sedning a SYMM after we have sent a Connect-pdu.
        
        resultLLC = _nfcReader->targetTxData(PDUoutPtr, CONNECT_SERVER_PDU_LEN); // SEND CONNECT PDU
           
        if(IS_ERROR(resultLLC)){
            return CONNECT_TX_FAILURE;   
        }
        
        buildSYMMPDU();
       
       resultLLC = _nfcReader -> targetTxData(PDUoutPtr, SYMM_PDU_LEN);
       
       if(IS_ERROR(resultLLC)){
        return SYMM_TX_FAILURE;
        }       
       
      _nfcReader->targetRxData(PDUin); // receive an CCPDU
       if (RESULT_OK(resultLLC))
       {      
  
          
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
  receivedPDU = (PDU *)PDUin; 
   
   buildDISCPDU();
   
   resultLLC = _nfcReader->targetTxData(PDUoutPtr, 2); 
   if(IS_ERROR(resultLLC))
   {
     Serial.println(F("SNEP>LLCP>closeLinkToServer: Disconnect Failed."));
     return GEN_ERROR;   
   }
      
   resultLLC = _nfcReader->targetRxData(PDUin);
   if(IS_ERROR(resultLLC) || !isDMPDU(receivedPDU))
    return GEN_ERROR;

   return resultLLC;   
}

// openNPPServerLink
uint32_t NFCLinkLayer::openLinkToClient() 
{
   receivedPDU = (PDU *)PDUin;
   #ifdef NFCLinkLayerDEBUG
      Serial.println(F("SNEP>LLCP>openLinkToClient: Opening Server Link."));
   #endif
   
  _nfcReader-> configurePeerAsTarget();   
   
   buildSYMMPDU();   
      
   do 
   {
     Serial.println(F("SNEP>LLCP>openLinkToClient: --- Recive CONNECTION PDU loop ---"));
     resultLLC = _nfcReader->targetRxData(PDUin);     
   if (IS_ERROR(resultLLC)){
      return resultLLC;
   }
     resultLLC = _nfcReader->targetTxData(PDUoutPtr, SYMM_PDU_LEN);     
   if (IS_ERROR(resultLLC)){
      return resultLLC;
   }
     
   } while (!receivedPDU->isConnectClientRequest());
   
   //Creating socket
   DSAP = receivedPDU->getSSAP();
   SSAP = receivedPDU->getDSAP();
   seq = 0;
   buildCCPDU();
 
   Serial.println(F("SNEP>LLCP>openLinkToClient: Trying to send Connection Complete PDU to peer."));
   if (_nfcReader->targetTxData(PDUoutPtr, CCPDU_PDU_LEN) >= 0x80000000)
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
   receivedPDU = (PDU *)PDUin;   
   resultLLC = _nfcReader->targetRxData(PDUin);   
   if (IS_ERROR(resultLLC) || !isRRPDU()){
      return resultLLC;
   }
   
   buildSYMMPDU();
   
   resultLLC = _nfcReader -> targetTxData(PDUoutPtr, SYMM_PDU_LEN);      
   if (IS_ERROR(resultLLC)){
      return resultLLC;
   }
   
   resultLLC = _nfcReader->targetRxData(PDUin);
   receivedPDU = (PDU *)PDUin; // Need to do this! why?
   if (IS_ERROR(resultLLC) || !isDISCPDU()){
      Serial.println("DERP!");
      return resultLLC;
   }
   
   PDUoutPtr = (uint8_t*)&PDUout; ///////////WHHHHHHHHHHHHHHHHHHHHHHHYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYHHHHHHHHHHHHHYYYYYYYYYYYYYYYYYYYYYYYHHHHHHHHHHHHHHYYYYYYYYYYYYYYYYYYYYYy!
   
   buildDMPDU();
   
   resultLLC = _nfcReader -> targetTxData(PDUoutPtr,DMPDU_PDU_LEN);
   
   Serial.println(F("SNEP>LLCP>closeLinkToClient: We are now disconnected.")); 
   
   return resultLLC;  
}
// former serverLinkRxData
uint32_t NFCLinkLayer::receiveSNEP(uint8_t *&Data){
  
  receivedPDU = (PDU *)Data;
  
  buildSYMMPDU();
   do{
       Serial.println(F("SNEP>LLCP>receiveSNEP: Trying to get Information-PDU!"));
       //delay(3000);                                                                                  //Can this delay fix the samsungbug
       if(IS_ERROR(_nfcReader->targetTxData(PDUoutPtr, SYMM_PDU_LEN))){                           //No SYMM should be transmitted when transmitting procedure!!!!!!!!!!!!
         Serial.println(F("SNEP>LLCP>receiveSNEP: Failed to receive Information-PDU1."));
         return NDEF_MESSAGE_RX_FAILURE;
       }
       if(IS_ERROR(_nfcReader->targetRxData(Data))){
         Serial.println(F("SNEP>LLCP>receiveSNEP: Failed to receive Information-PDU2."));
         return NDEF_MESSAGE_RX_FAILURE;
       }
   }while(receivedPDU->getPTYPE() != INFORMATION_PTYPE);
   
   Serial.println(F("SNEP>LLCP>receiveSNEP: Received INFORMATION_PTYPE"));
   /*if(params[1] == 0x81){
     Serial.println(F("SNEP>LLCP>receiveSNEP: Received 0x81 SUCCESS"));
   }*/
   
   increaceReceiveWindow(); //Receive window increaced, we recieved a number llc pdu.
   
   buildRRPDU();
   resultLLC = _nfcReader->targetTxData(PDUoutPtr, 3);
   if (IS_ERROR(resultLLC)) 
   {
      #ifdef NFCLinkLayerDEBUG{
         Serial.println(F("SNEP>LLCP>receiveSNEP: Ack Failed."));
      #endif
      return resultLLC;   
   }   
   // Discard the LLCPDU, what remains is the SNEP PDU
   Data = &Data[3];
   
   return RESULT_SUCCESS;
}
// former clientLinkTxData
uint32_t NFCLinkLayer::transmitSNEP(uint8_t *&SNEPMessage, uint32_t len)
{
   PDU *infoPDU = (PDU *) ALLOCATE_HEADER_SPACE(SNEPMessage, 3);
   infoPDU->setDSAP(DSAP);
   infoPDU->setSSAP(SSAP);
   infoPDU->setPTYPE(INFORMATION_PTYPE);
   uint8_t *payloadPtr = (uint8_t *)infoPDU; //Retard C forces us to cast shitt
   
   infoPDU->setSeq(seq); // Setting the sequence number, must increment!
   increaceSendWindow(); //Increasing send window.      
   
   
    #ifdef NFCLinkLayerDEBUG
      Serial.print(F("SNEP>LLCP>transmitSNEP: Minne:"));
      Serial.println(freeMemory());
    #endif
   if (IS_ERROR(_nfcReader->targetTxData(payloadPtr, (uint32_t) len + 3))) // Send the SNEP+PDU
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

void NFCLinkLayer::buildSYMMPDU()
{
  targetPayload = (PDU *) PDUoutPtr;
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
void NFCLinkLayer::buildCCPDU()
{
   targetPayload = (PDU *) PDUoutPtr;
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
void NFCLinkLayer::buildConnectPDU()
{
  targetPayload = (PDU *) PDUoutPtr;
  targetPayload->setDSAP(DSAP);
  targetPayload->setPTYPE(CONNECT_PTYPE);
  targetPayload->setSSAP(SSAP);
  targetPayload->params.sequence = 0x02;
  targetPayload->params.data[0] = 0x02;
  targetPayload->params.data[1] = 0x00;
  targetPayload->params.data[2] = 0x20;
  
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
void NFCLinkLayer::buildDISCPDU()
{
  targetPayload = (PDU * ) PDUoutPtr;
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

boolean NFCLinkLayer::isDISCPDU()
{
  return receivedPDU->getPTYPE() == DISCONNECT_PTYPE; 
}

boolean NFCLinkLayer :: isDICK(){
  return (((PDUin[0] & 0x03) << 2) | ((PDUin[1] & 0xC0) >> 6)) == DISCONNECT_PTYPE;
}

void NFCLinkLayer::buildRRPDU()
{
   targetPayload = (PDU *) PDUoutPtr;
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

boolean NFCLinkLayer::isRRPDU()
{
  return receivedPDU->getPTYPE() == RECEIVE_READY_TYPE;
}
void NFCLinkLayer::buildDMPDU()
{
   targetPayload = (PDU *) PDUoutPtr;  
   targetPayload->setDSAP(DSAP);
   targetPayload->setPTYPE(DISCONNECTED_MODE_PTYPE);
   targetPayload->setSSAP(SSAP);
   targetPayload ->setSeq(0);
   
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

