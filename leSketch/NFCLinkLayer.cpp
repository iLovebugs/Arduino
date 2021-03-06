#include "NFCLinkLayer.h"

NFCLinkLayer::NFCLinkLayer(NFCReader *nfcReader) 
    : _nfcReader(nfcReader)
{
}

NFCLinkLayer::~NFCLinkLayer() 
{

}


uint32_t NFCLinkLayer::openNPPClientLink()
{
   PDU *targetPayload;
   PDU *recievedPDU;
   uint8_t PDUBuffer[20];
   uint8_t DataIn[64];
        
   Serial.println(F("Opening NPP CLIENT Link."));
   
   
   uint32_t result = _nfcReader->configurePeerAsTarget(NPP_CLIENT);

   if (IS_ERROR(result))
   {
       //#ifdef PN532DEBUG
       Serial.println(F("Is Error"));
       //#endif
       return result;
   }
   recievedPDU = ( PDU *) DataIn; 
   if (!_nfcReader->targetRxData(DataIn))  //We get SYMM PDU, WHY?
   {     
      
         Serial.println(F("Connection Failed."));
      
      return CONNECT_RX_FAILURE;   
   }
   
   targetPayload = (PDU *) PDUBuffer;
   targetPayload->setDSAP(0x01); //Calls the Service Discovery Protocol. Lets the Target choose appropriate DSAP?
   targetPayload->setPTYPE(CONNECT_PTYPE); //Connection Request.
   targetPayload->setSSAP(0x20); // We emulate an application request.
   
   //Sets the parameter fields of the LLC PDU
   targetPayload->params.type = SERVICE_NAME_PARAM_TYPE; // Sets the message as a Service Name, SN  
   targetPayload->params.length = CONNECT_SERVICE_NAME_LEN; // "com.android.npp", can we change this to something more applicaton specific?
                                                             //The DSAP MUSH be 0x01 for the reciever to handle the PDU
   memcpy(targetPayload->params.data, CONNECT_SERVICE_NAME, CONNECT_SERVICE_NAME_LEN);
   
   do
   {
       if (IS_ERROR(_nfcReader->targetTxData((uint8_t *)targetPayload, CONNECT_SERVER_PDU_LEN))) {
          Serial.println(F("Error!")); // Här blir det fel!
          return CONNECT_TX_FAILURE;   
       }
   } while(IS_ERROR(_nfcReader->targetRxData(DataIn)));
    
   if (recievedPDU->getPTYPE() != CONNECTION_COMPLETE_PTYPE) //LLC connection complete!!
   {
      //#ifdef PN532DEBUG
      
         Serial.println(F("Connection Complete Failed."));
      //#endif
      return UNEXPECTED_PDU_FAILURE;
   }
   
   DSAP = recievedPDU->getSSAP(); //we now know the destination service accsess point
   SSAP = recievedPDU->getDSAP(); //we now know our? access point 0x20 i guess?
   
   Serial.println(F("Connection OK!"));
   return RESULT_SUCCESS;
}


// Stängt härifrån
uint32_t NFCLinkLayer::closeNPPClientLink() 
{

}


uint32_t NFCLinkLayer::openNPPServerLink() 
{
   uint8_t status[2];
   uint8_t DataIn[64];
   PDU *recievedPDU;
   PDU targetPayload;
   uint32_t result;

   Serial.println(F("Opening SERVER Link."));

   
   result = _nfcReader->configurePeerAsTarget(NPP_CLIENT);
   if (IS_ERROR(result))
   {
       return result;
   }
   
   recievedPDU = (PDU *)DataIn;
   do 
   {
     result = _nfcReader->targetRxData(DataIn);
     
     #ifdef PN532DEBUG
     
        Serial.print(F("Configured as Peer: "));
        Serial.print(F("0x"));
        Serial.println(result, HEX);
     #endif
     
     if (IS_ERROR(result)) 
     {
        return result;   
     }
   } while (result < CONNECT_SERVER_PDU_LEN || !recievedPDU->isConnectClientRequest());
   
   targetPayload.setDSAP(recievedPDU->getSSAP());
   targetPayload.setPTYPE(CONNECTION_COMPLETE_PTYPE);
   targetPayload.setSSAP(recievedPDU->getDSAP());

   if (IS_ERROR(_nfcReader->targetTxData((uint8_t *)&targetPayload, 2))) 
   {
      #ifdef PN532DEBUG
      
         Serial.println(F("Connection Complete Failed."));
      #endif
      
      return CONNECT_COMPLETE_TX_FAILURE;   
   }

   return RESULT_SUCCESS;
}


uint32_t NFCLinkLayer::closeNPPServerLink() 
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


uint32_t NFCLinkLayer::serverLinkRxData(uint8_t *&Data)
{
  Serial.println(F("<serverLinkRxData>"));
   uint32_t result = _nfcReader->targetRxData(Data);
   uint8_t len;
   PDU *recievedPDU;
   PDU ackPDU;
   
   if (IS_ERROR(result)) 
   {
      //#ifdef PN532DEBUG
      
         Serial.println(F("Failed to Recieve NDEF Message."));
      //#endif
      
      return NDEF_MESSAGE_RX_FAILURE;
   }
   
   len = (uint8_t) result;
   
   recievedPDU = (PDU *) Data;   
   
   if (recievedPDU->getPTYPE() != INFORMATION_PTYPE)
   {
      #ifdef PN532DEBUG
      
         Serial.println(F("Unexpected PDU"));
      #endif
      
      return UNEXPECTED_PDU_FAILURE;
   }
   
   // Acknowledge reciept of Information PDU
   ackPDU.setDSAP(recievedPDU->getSSAP());
   ackPDU.setPTYPE(RECEIVE_READY_TYPE);
   ackPDU.setSSAP(recievedPDU->getDSAP());
   
   ackPDU.params.sequence = recievedPDU->params.sequence & 0x0F;
   
   result = _nfcReader->targetTxData((uint8_t *)&ackPDU, 3);
   if (IS_ERROR(result)) 
   {
      #ifdef PN532DEBUG
      
         Serial.println(F("Ack Failed."));
         
      #endif
      
      return result;   
   }
   
   
   Data = &Data[3];
   
   return len - 2;
}
// Stängt hit

uint32_t NFCLinkLayer::clientLinkTxData(uint8_t *nppMessage, uint32_t len)
{
   PDU *infoPDU = (PDU *) ALLOCATE_HEADER_SPACE(nppMessage, 3);
   infoPDU->setDSAP(DSAP);
   infoPDU->setSSAP(SSAP);
   infoPDU->setPTYPE(INFORMATION_PTYPE);
   
   infoPDU->params.sequence = 0;
   
   /*
   uint8_t *buf = (uint8_t *) infoPDU;
   Serial.println("PDU + NPP + NDEF Message"); 
   for (uint16_t i = 0; i < len + 3; ++i)
   {
       Serial.print(F("0x")); 
       Serial.print(buf[i], HEX);
       Serial.print(F(" "));
   }*/
   
   Serial.println(F("Sending NDEF Message")); 
   if (IS_ERROR(_nfcReader->targetTxData((uint8_t *)infoPDU, len + 3))) 
   {
     #ifdef PN532DEBUG
     
        Serial.println(F("Sending NDEF Message Failed."));
     #endif
     
     return NDEF_MESSAGE_TX_FAILURE;   
   }
   
   PDU disconnect;
   disconnect.setDSAP(DSAP);
   disconnect.setSSAP(SSAP);
   disconnect.setPTYPE(DISCONNECT_PTYPE);
   
   if (!_nfcReader->targetTxData((uint8_t *)&disconnect, 2)) 
   {
     Serial.println(F("Disconnect Failed."));
     return false;   
   }
   
   #ifdef PN532DEBUG
   
      Serial.println(F("Sent NDEF Message"));
   #endif
   
   return RESULT_SUCCESS;
}


inline bool PDU::isConnectClientRequest()
{
    return ((getPTYPE() == CONNECT_PTYPE)                     && 
             (params.length == CONNECT_SERVICE_NAME_LEN)       &&
             (strncmp((char *)params.data, CONNECT_SERVICE_NAME, CONNECT_SERVICE_NAME_LEN) == 0));
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
