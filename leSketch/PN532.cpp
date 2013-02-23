// PN532 library by adafruit/ladyada
// MIT license

// authenticateBlock, readMemoryBlock, writeMemoryBlock contributed
// by Seeed Technology Inc (www.seeedstudio.com)

#include "PN532.h"
#include <Wire.h>

#define PN532DEBUG 1



uint8_t pn532ack[] = {0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
uint8_t pn532response_firmwarevers[] = {0x00, 0xFF, 0x06, 0xFA, 0xD5, 0x03};

#define COMMAND_RESPONSE_SIZE 3 
#define TS_GET_DATA_IN_MAX_SIZE  262 + 3
#define CHUNK_OF_DATA 64

//Buffern som allt l�ggs p�, rymmer ett standard PN532 commando
uint8_t pn532_packetbuffer[TS_GET_DATA_IN_MAX_SIZE];

//Pinnarna i argumentet skall defineras av oss i huvudfilen!  
PN532::PN532(uint8_t irq, uint8_t reset) 
{
    //argumenten har reducerats. I2C kr�ver mycket mindre pinnar!
	//Dessa pinnar skall defineras i v�r main
    _irq = irq;
    _reset = reset;

    pinMode(_irq, INPUT);
    pinMode(_reset, OUTPUT);
}

//Hur kommer vi ur lowVbat med I2C som saknar slave select!
// Verkar vara s� att denna metod �r helt on�dig ?
void PN532::initializeReader() 
{
								//Pinnarna som st�ller in vilket interfacelage som skall k�ras �r antagligen konstanta.
	Wire.begin();				// Hoppar med p� I2C bussen som "master"
	digitalWrite(_reset, HIGH); // skriva till reset verar vara �verfl�dig.... ? 
	digitalWrite(_reset, LOW);
	delay(400); 
	digitalWrite(_reset, HIGH);

	/**
    digitalWrite(_ss, LOW);

    delay(1000);

    // not exactly sure why but we have to send a dummy command to get synced up
    pn532_packetbuffer[0] = PN532_FIRMWAREVERSION;
    sendCommandCheckAck(pn532_packetbuffer, 1);

    // ignore response!
	**/
}


uint32_t PN532::getFirmwareVersion(void) 
{
    uint32_t version;

    pn532_packetbuffer[0] = PN532_FIRMWAREVERSION;

    if (IS_ERROR(sendCommandCheckAck(pn532_packetbuffer, 1))) 
    {
        return 0;
    }

    // read response Packet
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    if (IS_ERROR(fetchResponse(PN532_FIRMWAREVERSION, response))) // bytt till fetchResponse, fungerar det?
    {
       return 0;
    }
    
    //response->printResponse();
    
    version = response->data[0];
    version <<= 8;
    version |= response->data[1];
    version <<= 8;
    version |= response->data[2];
    version <<= 8;
    version |= response->data[3];

    return version;
}


// default timeout of one second
uint32_t PN532::sendCommandCheckAck(uint8_t *cmd, 
                                    uint8_t cmdlen, 
                                    uint16_t timeout, 
                                    boolean debug) 
{
    uint16_t timer = 0;
    Serial.println("sendCommandCheckAck");
    
        // send the command frame
    sendFrame(cmd, cmdlen, debug);
    
    // Wait for chip to say it has data to send
   
    
    Serial.print("sendCommandCheckAck: Data available, IRQ: ");
    Serial.println(checkDataAvailable());
    delay(1000);
    
    // read acknowledgement
    if (!fetchCheckAck(debug)) {
        return SEND_COMMAND_RX_ACK_ERROR;
        Serial.println("ACK error"); 
    }
      
    Serial.print("sendCommandCheckAck: Data available???, IRQ: ");
    Serial.println(checkDataAvailable());
    delay(1000);
        
    Serial.println("sendCommandCheckAck: Great success & command is ack'd!");
    return RESULT_SUCCESS; // ack'd command
}

uint32_t PN532::SAMConfig(boolean debugg) 
{
    pn532_packetbuffer[0] = PN532_SAMCONFIGURATION;
    pn532_packetbuffer[1] = 0x01; // normal mode;
    pn532_packetbuffer[2] = 0x14; // timeout 50ms * 20 = 1 second
    pn532_packetbuffer[3] = 0x01; // use IRQ pin!
    
    uint32_t result = sendCommandCheckAck(pn532_packetbuffer, 4, 1000, debugg);

    if (IS_ERROR(result)) 
    {
        Serial.println("SAMConfig: SEND_COMMAND_TX_TIMEOUT_ERROR");
        return result;
    }

    // read data packet
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    return fetchResponse(PN532_SAMCONFIGURATION, response, debugg);
}
/********** LARGE CHUNK CUT OUT, placed at file bottom  **********/

/************** high level functions */


boolean PN532::fetchCheckAck(boolean debug)
{
    uint8_t ackbuff[6];

    //fetchData(ackbuff, 6, debug);
    int timer = 0;
    int timeout = 1000;
    
    while ((checkDataAvailable()) == PN532_I2C_NO_DATA){
      delay(10);
      timer+=10;
      
      if (timer > timeout){
        Serial.println("Timeout");
        return 0;                 
     }
    }
        
    Wire.requestFrom((uint8_t)PN532_I2C_ADDRESS, (uint8_t)7);
    Wire.read();//take away rubbish due to I2C delay
            
    Serial.println("fetchCheckAck: Data given: ");
    for (uint16_t i=0; i<7; i++){
        delay(1);
        ackbuff[i] = Wire.read();
        
        if (debug)
        {
            Serial.print(F(" 0x"));
            Serial.print(ackbuff[i], HEX);
        }
    }
    if((0 == strncmp((char *)ackbuff, (char *)pn532ack, 6)))
      Serial.println("\fetchCheckAck: Ack matched");
      
    return (0 == strncmp((char *)ackbuff, (char *)pn532ack, 6));
}

/************** mid level functions */


//Ers�tter helt readspistatus, i I2C r�cker det med att titta p� IRQ linan f�r att av�gra om PN532 �r upptagen. What? /jonas
uint8_t PN532::checkDataAvailable(void) {
  uint8_t x = digitalRead(_irq);
  
  if (x == 1)
    return PN532_I2C_NO_DATA;
  else
    return PN532_I2C_DATA_TO_FETCH;
}



//Bryter isär svaret fr�n PN532 
uint32_t PN532::fetchResponse(uint8_t cmdCode, PN532_CMD_RESPONSE *response, boolean debug) 
{

    uint8_t calc_checksum = 0;
    uint8_t ret_checksum;
        
    // Wattafack?
    response->header[0] = response->header[1] = 0xAA;
    
    uint32_t retVal = RESULT_SUCCESS;
    
    fetchData((uint8_t *)response, (uint32_t)150, true);
    
    Serial.println(response->preamble, HEX);
    Serial.println(response->header[0], HEX);
    Serial.println(response->header[1], HEX);
    Serial.println(response->len, HEX);
    Serial.println(response->len_chksum, HEX);
    Serial.println(response->direction, HEX);
    Serial.println(response->responseCode, HEX);
    Serial.println(response->data[0], HEX);
    Serial.println(response->data[1], HEX);
    Serial.println(response->data[2], HEX);
    Serial.println("done");
    

    
    // Kontrollerar så att datan given från PN532 faktiskt är svaret på det givna kommandot
    //Detta måste vara det coolaste jag sett när det gäller c. Man kör en metod I en strukt genom att
    //Använa pil operatorn på en metod?
    retVal = response->verifyResponse(cmdCode) ? RESULT_SUCCESS : INVALID_RESPONSE;      
    
    //Vart kommer RESULT_OK från?
    if (RESULT_OK(retVal)){  
        Serial.println("correct Startcode, length and response code");
      
      calc_checksum += response->direction;
      calc_checksum += response->responseCode;
      
      // Add data field to checksum
        uint8_t i = 0;
        uint8_t temp_len =  response -> len ;
        
        Serial.print("Data length: ");
        Serial.println(temp_len );
        //Här är temp_len 0 när vi får svaret, eftersom datalength bara är 2.
        
        for ( i; i < response -> len -2 ; i++){
            calc_checksum +=  response->data[i];
         }
         
         
         Serial.print("Nuvarande I-värde: ");
         Serial.println(i);
         
           // Find Packet Data Checksum. Vi hämtar data[0] vilket är 0x16 = 22, detta bekreftas nedan.
           // Detta är också data Checksum då data 0 är fäst på byten efter direction!... alltså är detta korrekt 
         uint8_t ret_checksum = response->data[i++];
         
         //Debugg
         Serial.print("Nuvarande I-värde: ");
         Serial.println(i);
         
         
         Serial.print("Fetched checksum: ");
         Serial.println(ret_checksum);
        
        // Check if the checksums are correct
         if (((uint8_t)(calc_checksum + ret_checksum)) != 0x00) 
         {
            Serial.println(F("Invalid Checksum recievied."));
            retVal = INVALID_CHECKSUM_RX;
         }
         
         
         Serial.println("Correct checksum");
         //Denna utskrift ges, alltså går vi inte in i if-satsen ovan, slutsatsen är att vi har beräknat checksum korrekt.
         
        // Find Postamble
         uint8_t postamble = response->data[i];
        //I pekar redan på postamble, alltså behövs ingen ökning! 
         
         
         //Debugg
         Serial.print("Nuvarande I-värde: ");
         Serial.println(i);
         
         
         //Denna blir nu 0!!!
         Serial.println(postamble);
         
         
         if (RESULT_OK(retVal) && postamble != 0x00) 
         {
             retVal = INVALID_POSTAMBLE;
             Serial.println(F("Invalid Postamble."));
         }
         
         Serial.println("Svarsmeddelande har tagits emot korrekt!");
       
    }
    
   
    if (debug)
    {
      response->printResponse();
      
      Serial.print(F("Calculated Checksum: 0x"));
      Serial.print(calc_checksum, HEX);
      Serial.print(F(" Returned Checksum: 0x") );
      Serial.println(ret_checksum, HEX); //Helt slumpat blir ret_checksum 0....
      Serial.println();    
    }

    return retVal;
}

void PN532::fetchData(uint8_t* buff, uint32_t n, boolean debug) 
{    
    delay(2000);    
    
    if (debug) 
    {
        Serial.println(F("fetchData"));
    }
    
    //Vänta på att PN532 har något att sända... Kanske skall ha en timeout eller skall denna kanske bort sedan?
    while(checkDataAvailable() == PN532_I2C_NO_DATA){
      Serial.println("fetchData: Nothing to fetch");
      delay(10);
    }
    
    //Hämta en Chunk av datam fortsätt Chunks tills dessa tt PN532 inte har något att ge oss
    while(checkDataAvailable() == PN532_I2C_DATA_TO_FETCH){
      Serial.println("fetchData: Fetching new chunk of data");
      Wire.requestFrom((uint8_t)PN532_I2C_ADDRESS, (uint8_t)(CHUNK_OF_DATA+1));
      delay(8);
      Wire.read();//take away rubbish due to I2C delay
    }
    
    //Dessa utskrifter blir bajs i terminalen, varför? Hade ju varit intressant att se på hur måga bytes som vi faktiskt har läst in. 
        
    Serial.print("fetchData: " + Wire.available());
    Serial.println(" bytes fetched.");
        
    Serial.println("fetchData: Data given: ");
    for (uint16_t i=0; i<n; i++){
        delay(1);
        buff[i] = Wire.read();
        
        if (debug)
        {
            Serial.print(F(" 0x"));
            Serial.print(buff[i], HEX);
        }
    }

    if (debug)
    {
        Serial.println();
    }   
    Serial.println("fetchData: End data given");
}


void PN532::sendFrame(uint8_t* cmd, uint8_t cmdlen, boolean debug)
{
  uint8_t checksum;
  cmdlen++;

  //Skriver ut på terminalen
  if (debug) 
  {
    Serial.print(F("\nsendFrame: Sending: "));
  }

  delay(2);     // or whatever the delay is for waking up the board    
  Wire.beginTransmission(PN532_I2C_ADDRESS);


  checksum = PN532_PREAMBLE + PN532_PREAMBLE + PN532_STARTCODE2;

  //Mycket riktigt är det 00x x00 xFF som är startsekvensen för PN532
  Wire.write(PN532_PREAMBLE);
  Wire.write(PN532_STARTCODE1);
  Wire.write(PN532_STARTCODE2);

  Wire.write(cmdlen); //LEN
  uint8_t cmdlen_1=~cmdlen + 1;
  Wire.write(cmdlen_1);		//tvåkompleterar cmdlen, används som LCS length checksum
  Wire.write(PN532_HOSTTOPN532);	//TFI specificeras som från host till PN532 (0xD4)

  checksum += PN532_HOSTTOPN532; //Checksumen ökas...

  if (debug) 
  {
    Serial.print(F(" 0x")); 
    Serial.print(PN532_PREAMBLE, HEX);
    Serial.print(F(" 0x")); 
    Serial.print(PN532_PREAMBLE, HEX);
    Serial.print(F(" 0x")); 
    Serial.print(PN532_STARTCODE2, HEX);
    Serial.print(F(" 0x")); 
    Serial.print(cmdlen, HEX);
    Serial.print(F(" 0x")); 
    Serial.print(cmdlen_1, HEX);
    Serial.print(F(" 0x")); 
    Serial.print(PN532_HOSTTOPN532, HEX);
  }

    
  //Denna loopen står för skickadet av själva kommandot dvs PD0...PDN
  for (uint8_t i=0; i<cmdlen-1; i++) {
    Wire.write(cmd[i]);
    checksum += cmd[i];  

    if (debug) 
    {
      Serial.print(F(" 0x")); 
      Serial.print(cmd[i], HEX);
    }
  }

  uint8_t checksum_1= ~checksum;
  
  Wire.write(checksum_1); //Varför invereras bara detta? Hur vet man att den verklige uppfyller kravet
  Wire.write(PN532_POSTAMBLE); 


  if (debug) 
  {
    Serial.print(F(" 0x")); 
    Serial.print(checksum_1, HEX);
    Serial.print(F(" 0x")); 
    Serial.print(PN532_POSTAMBLE, HEX);
    Serial.println();
  }
  Serial.println(Wire.endTransmission()); // Nu datan faktiskt sänds iväg
  
  Serial.println("sendFrame: sendCommand done");
}

/******* PN532_CMD_RESPONSE methods */

boolean PN532_CMD_RESPONSE::verifyResponse(uint32_t cmdCode)
{
    return ( header[0] == 0x00 && 
             header[1] == 0xFF &&
            ((uint8_t)(len + len_chksum)) == 0x00 && 
            direction == 0xD5 && 
            (cmdCode + 1) == responseCode);
}

void PN532_CMD_RESPONSE::printResponse() 
{
    Serial.println(F("Response"));
    Serial.print(F("Len: 0x"));
    Serial.println(len, HEX);
    Serial.print(F("Direction: 0x"));
    Serial.println(direction, HEX);
    Serial.print(F("Response Command: 0x"));
    Serial.println(responseCode, HEX);
    
    Serial.println("Data: ");
    
    for (uint8_t i = 0; i < len-2; ++i) 
    {
        Serial.print(F("0x"));
        Serial.print(data[i], HEX);
        Serial.print(F(" "));
    }
    
    Serial.println();
}

/********** CUT OUT **********


uint32_t PN532::initiatorTxRxData(uint8_t *DataOut,
                           uint32_t dataSize,
                           uint8_t *DataIn,
                           boolean debug)
{
    pn532_packetbuffer[0] = PN532_INDATAEXCHANGE;
    pn532_packetbuffer[1] = 0x01; //Target 01

    for(uint8_t iter=(2+0);iter<(2+dataSize);iter++)
    {
        pn532_packetbuffer[iter] = DataOut[iter-2]; //pack the data to send to target
    }
    
    uint32_t result = sendCommandCheckAck(pn532_packetbuffer, dataSize+2);

    if (IS_ERROR(result))
    {
        return result;
    }
    
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    if (!readspicommand(PN532_INDATAEXCHANGE, response))
    {
       return false;
    }
    
    if (debug)
    {
       response->printResponse();
    }
    
    if (response->data[0] != 0x00)
    {
       return (GEN_ERROR | response->data[0]);
    }
    return RESULT_SUCCESS; //No error
}

*/
uint32_t PN532::configurePeerAsTarget(uint8_t type)
{
    static const uint8_t npp_client[44] = { PN532_TGINITASTARGET,
                             0x00,
                             0x00, 0x00, //SENS_RES
                             0x00, 0x00, 0x00, //NFCID1
                             0x00, //SEL_RES

                             0x01, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // POL_RES
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           
                             0x00, 0x00,
                            
                             0x01, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //NFCID3t: Change this to desired value

                             0x06, 0x46, 0x66, 0x6D, 0x01, 0x01, 0x10, 0x00
                             };
    
    static const uint8_t npp_server[44] = { PN532_TGINITASTARGET,
                             0x01,
                             0x00, 0x00, //SENS_RES
                             0x00, 0x00, 0x00, //NFCID1
                             0x40, //SEL_RES

                             0x01, 0xFE, 0x0F, 0xBB, 0xBA, 0xA6, 0xC9, 0x89, // POL_RES
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                           
                             0xFF, 0xFF,
                            
                             0x01, 0xFE, 0x0F, 0xBB, 0xBA, 0xA6, 0xC9, 0x89, 0x00, 0x00, //NFCID3t: Change this to desired value

                             0x06, 0x46, 0x66, 0x6D, 0x01, 0x01, 0x10, 0x00
                             };
                             
    if (type == NPP_CLIENT)
    {
       for(uint8_t iter = 0;iter < 44;iter++)
       {
          pn532_packetbuffer[iter] = npp_client[iter];
       }
    }
    else if (type == NPP_SERVER)
    {
       for(uint8_t iter = 0;iter < 44;iter++)
       {
          pn532_packetbuffer[iter] = npp_server[iter];
       }
    }
    
    uint32_t result;
    result = sendCommandCheckAck(pn532_packetbuffer, 44);
   
    if (IS_ERROR(result))
    {
        return result;
    }
    
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    return fetchResponse(PN532_TGINITASTARGET, response);
}

/*
uint32_t PN532::getTargetStatus(uint8_t *DataIn)
{
    pn532_packetbuffer[0] = PN532_TGTARGETSTATUS;
    
    if (IS_ERROR(sendCommandCheckAck(pn532_packetbuffer, 1))) {
        return 0;
    }
    
    // read data packet
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    if (RESULT_OK(readspicommand(PN532_TGTARGETSTATUS, response)))
    {
       memcpy(DataIn, response->data, response->data_len);
       return response->data_len;
    }
    
    return 0;
}

uint32_t PN532::targetRxData(uint8_t *DataIn, boolean debug)
{
    ///////////////////////////////////// Receiving from Initiator ///////////////////////////
    pn532_packetbuffer[0] = PN532_TGGETDATA;
    uint32_t result = sendCommandCheckAck(pn532_packetbuffer, 1, 1000, debug);
    if (IS_ERROR(result)) {
        //Serial.println(F("SendCommandCheck Ack Failed"));
        return NFC_READER_COMMAND_FAILURE;
    }
    
    // read data packet
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    
    result = readspicommand(PN532_TGGETDATA, response, debug);
    
    if (IS_ERROR(result))
    {
       return NFC_READER_RESPONSE_FAILURE;
    }
 
    if (response->data[0] == 0x00)
    {
       uint32_t ret_len = response->data_len - 1;
       memcpy(DataIn, &(response->data[1]), ret_len);
       return ret_len;
    }
    
    return (GEN_ERROR | response->data[0]);
}



uint32_t PN532::targetTxData(uint8_t *DataOut, uint32_t dataSize, boolean debug)
{
    ///////////////////////////////////// Sending to Initiator ///////////////////////////
    pn532_packetbuffer[0] = PN532_TGSETDATA;
    uint8_t commandBufferSize = (1 + dataSize);
    for(uint8_t iter=(1+0);iter < commandBufferSize; ++iter)
    {
        pn532_packetbuffer[iter] = DataOut[iter-1]; //pack the data to send to target
    }
    
    uint32_t result = sendCommandCheckAck(pn532_packetbuffer, commandBufferSize, 1000, debug);
    if (IS_ERROR(result)) {
        Serial.println(F("TX_Target Command Failed."));
        return result;
    }
    
    
    // read data packet
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    
    result = readspicommand(PN532_TGSETDATA, response);
    if (IS_ERROR(result))
    {
       return result;
    }
    
    if (debug)
    {
       response->printResponse();
    }
    
    if (response->data[0] != 0x00)
    {
       return (GEN_ERROR | response->data[0]);
    }
    return RESULT_SUCCESS; //No error
}
    

uint32_t PN532::authenticateBlock(uint8_t cardnumber  ,//1 or 2
                                  uint32_t cid , //Card NUID
                                  uint8_t blockaddress, //0 to 63
                                  uint8_t authtype, //Either KEY_A or KEY_B
                                  uint8_t * keys)
{
    pn532_packetbuffer[0] = PN532_INDATAEXCHANGE;
    pn532_packetbuffer[1] = cardnumber; // either card 1 or 2 (tested for card 1)
    if(authtype == KEY_A)
    {
        pn532_packetbuffer[2] = PN532_AUTH_WITH_KEYA;
    }
    else
    {
        pn532_packetbuffer[2] = PN532_AUTH_WITH_KEYB;
    }
    pn532_packetbuffer[3] = blockaddress; //This address can be 0-63 for MIFARE 1K card

    pn532_packetbuffer[4] = keys[0];
    pn532_packetbuffer[5] = keys[1];
    pn532_packetbuffer[6] = keys[2];
    pn532_packetbuffer[7] = keys[3];
    pn532_packetbuffer[8] = keys[4];
    pn532_packetbuffer[9] = keys[5];

    pn532_packetbuffer[10] = ((cid >> 24) & 0xFF);
    pn532_packetbuffer[11] = ((cid >> 16) & 0xFF);
    pn532_packetbuffer[12] = ((cid >> 8) & 0xFF);
    pn532_packetbuffer[13] = ((cid >> 0) & 0xFF);
    
    uint32_t result = sendCommandCheckAck(pn532_packetbuffer, 14);

    if (IS_ERROR(result))
    {
        return result;
    }
    
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    result = readspicommand(PN532_INDATAEXCHANGE, response);
    if (IS_ERROR(result))
    {
       return result;
    }


    if((response->data[0] == 0x41) && (response->data[1] == 0x00))
    {
   return RESULT_SUCCESS;
    }

    return (GEN_ERROR | response->data[1]);
}

uint32_t PN532::readMemoryBlock(uint8_t cardnumber, //1 or 2
                                uint8_t blockaddress, //0 to 63
                                uint8_t * block)
{
    pn532_packetbuffer[0] = PN532_INDATAEXCHANGE;
    pn532_packetbuffer[1] = cardnumber; // either card 1 or 2 (tested for card 1)
    pn532_packetbuffer[2] = PN532_MIFARE_READ;
    pn532_packetbuffer[3] = blockaddress; //This address can be 0-63 for MIFARE 1K card
    
    uint32_t result = sendCommandCheckAck(pn532_packetbuffer, 4);

    if (IS_ERROR(result)) {
        return result;
    }

    // read data packet
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    result = readspicommand(PN532_INDATAEXCHANGE, response);
    if (IS_ERROR(result))
    {
       return result;
    }

    if((response->data[0] == 0x41) && (response->data[1] == 0x00))
    {
   return RESULT_SUCCESS; //read successful
    }

    return (GEN_ERROR | response->data[1]);
}

//Do not write to Sector Trailer Block unless you know what you are doing.
uint32_t PN532::writeMemoryBlock(uint8_t cardnumber, //1 or 2
                                 uint8_t blockaddress, //0 to 63
                                 uint8_t * block)
{
    pn532_packetbuffer[0] = PN532_INDATAEXCHANGE;
    pn532_packetbuffer[1] = cardnumber; // either card 1 or 2 (tested for card 1)
    pn532_packetbuffer[2] = PN532_MIFARE_WRITE;
    pn532_packetbuffer[3] = blockaddress;

    for(uint8_t i =0; i <16; i++)
    {
        pn532_packetbuffer[4+i] = block[i];
    }
    
    uint32_t result = sendCommandCheckAck(pn532_packetbuffer, 20);

    if (IS_ERROR(result))
    {
        return result;
    }
    
    // read data packet
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    result = readspicommand(PN532_INDATAEXCHANGE, response);
    if (IS_ERROR(result))
    {
       return result;
    }

    if((response->data[0] == 0x41) && (response->data[1] == 0x00))
    {
   return RESULT_SUCCESS; //read successful
    }

    return (GEN_ERROR | response->data[1]);
}

uint32_t PN532::readPassiveTargetID(uint8_t cardbaudrate)
{
    uint32_t cid;

    pn532_packetbuffer[0] = PN532_INLISTPASSIVETARGET;
    pn532_packetbuffer[1] = 1; // max 1 cards at once (we can set this to 2 later)
    pn532_packetbuffer[2] = cardbaudrate;

    if (IS_ERROR(sendCommandCheckAck(pn532_packetbuffer, 3)))
    {
        return 0; // no cards read
    }

    // read data packet
    PN532_CMD_RESPONSE *response = (PN532_CMD_RESPONSE *) pn532_packetbuffer;
    if (IS_ERROR(readspicommand(PN532_INDATAEXCHANGE, response)))
    {
       return 0;
    }
    
    // check some basic stuff
    Serial.print(F("Found "));
    Serial.print(response->data[2], DEC);
    Serial.println(F(" tags"));
    
    if (response->data[2] != 1)
    {
        return 0;
    }
    
    uint16_t sens_res = response->data[4];
    sens_res <<= 8;
    sens_res |= response->data[5];
    
    Serial.print(F("Sens Response: 0x"));
    Serial.println(sens_res, HEX);
    Serial.print(F("Sel Response: 0x"));
    Serial.println(response->data[6], HEX);
    
    cid = 0;
    for (uint8_t i = 0; i < response->data[7]; i++)
    {
        cid <<= 8;
        cid |= response->data[8 + i];
        Serial.print(F(" 0x"));
        Serial.print(response->data[8 + i], HEX);
    }

    return cid;
}


inline boolean PN532::isTargetReleasedError(uint32_t result)
{
   return result == (GEN_ERROR | TARGET_RELEASED_ERROR);
}
*/
