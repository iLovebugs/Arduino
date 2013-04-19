// PN532 library by adafruit/ladyada
// MIT license

// authenticateBlock, readMemoryBlock, writeMemoryBlock contributed
// by Seeed Technology Inc (www.seeedstudio.com)

#include "Arduino.h"
#include "MemoryFree.h"
#include "NFCReader.h"
#include <avr/sleep.h>

#define PN532_PREAMBLE 0x00
#define PN532_STARTCODE1 0x00
#define PN532_STARTCODE2 0xFF
#define PN532_POSTAMBLE 0x00

#define PN532_HOSTTOPN532 0xD4

#define PN532_FIRMWAREVERSION 0x02
#define PN532_GETGENERALSTATUS 0x04
#define PN532_SAMCONFIGURATION  0x14
#define PN532_INLISTPASSIVETARGET 0x4A
#define PN532_INDATAEXCHANGE 0x40
#define PN532_INJUMPFORDEP 0x56
#define PN532_TGINITASTARGET 0x8C
#define PN532_TGGETDATA 0x86
#define PN532_TGSETDATA 0x8E
#define PN532_TGTARGETSTATUS   0x8A

#define PN532_MIFARE_READ 0x30
#define PN532_MIFARE_WRITE 0xA0

#define PN532_AUTH_WITH_KEYA 0x60
#define PN532_AUTH_WITH_KEYB 0x61


#define PN532_WAKEUP 0x55

#define  PN532_SPI_STATREAD 0x02
#define  PN532_SPI_DATAWRITE 0x01
#define  PN532_SPI_DATAREAD 0x03
#define  PN532_SPI_READY 0x01

#define PN532_I2C_ADDRESS (0x48 >> 1)
#define PN532_I2C_READBIT (0x01)
#define PN532_I2C_DATA_TO_FETCH (0x00)
#define PN532_I2C_NO_DATA (0x01)
#define TIMEOUT (20)

#define PN532_MIFARE_ISO14443A 0x0

#define KEY_A	1
#define KEY_B	2

#define PN532_SEL_RES_DEP    0x40
#define PN532_SEL_RES_PICC   0x20
#define PN532_SEL_RES_BOTH   0x60

#define PN532_MODE_PICC_ONLY  0x04
#define PN532_MODE_DEP_ONLY     0x02
#define PN532_MODE_PASSIVE_ONLY 0x01

#define PN532_MODE_PICC_ONLY_OFF  0x00
#define PN532_MODE_DEP_ONLY_OFF     0x00
#define PN532_MODE_PASSIVE_ONLY_OFF 0x00

#define PN532_MODE_AS_TARGET (PN532_MODE_PICC_ONLY_OFF | PN532_MODE_DEP_ONLY_OFF | PN532_MODE_PASSIVE_ONLY_OFF)

enum READER_ERRORS 
{
    TIME_OUT_ERROR = 0x01,
    CRC_ERROR,
    PARITY_ERROR,
    ANTI_COLLISION_BIT_COUNT_ERROR,
    MIFARE_FRAMING_ERROR,
    ANTI_COLLISION_BIT_COLLISION_ERROR,
    INSUFFICIENT_BUFFER_ERROR,
    RF_BUFFER_OVERFLOW_ERROR,
    RF_FIELD_MISSING_ERROR,
    RF_PROTOCOL_ERROR,
    TEMPERATURE_ERROR = 0x0D,
    INTERNAL_BUFFER_ERROR,
    INVALID_PARAM_ERROR = 0x10,
    DEP_UNSUPPORTED_COMMAND_ERROR = 0x12,
    DATA_FORMAT_ERROR,
    MIFARE_AUTH_ERROR,
    UID_CHECK_BYTE_ERROR = 0X23,
    DEP_INVALID_DEVICE_STATE_ERROR = 0x25,
    OPERATION_NOT_ALLOWED_ERROR,
    TARGET_RELEASED_ERROR = 0x29,
    CARD_ID_ERROR,
    CARD_INACTIVE_ERROR,
    NFCID3_MISMATCH_ERROR,
    OVER_CURRENT_ERROR,
    DEP_NAD_MISSING,
    CONNECTION_ERROR,   
};

struct PN532_CMD_RESPONSE {
   uint8_t preamble;
   uint8_t header[2];   // 0x00 0xFF
   uint8_t len;         
   uint8_t len_chksum;  // len + len_chksum = 0x00 
   uint8_t direction;
   uint8_t responseCode;
   uint8_t data[0];
   
   boolean verifyResponse(uint32_t cmdCode);
   void printResponse();
   
};


    void wakeUpFunction();
    void sleepTight();



class PN532 : public NFCReader{
public:
    PN532(uint8_t irq, uint8_t reset);

    void initializeReader();
    uint32_t SAMConfig();    
    uint32_t getFirmwareVersion();
     
    uint32_t configurePeerAsTarget(boolean sleep = false); 
 /* 
    uint32_t getTargetStatus(uint8_t *response);
*/
    uint32_t sendCommandCheckAck(uint8_t *cmd, 
                                 uint8_t cmdlen, 
                                 uint16_t timeout = 1000,
                                 boolean debug = true);
                                 
uint32_t getGeneralStatus();                               

    uint32_t targetTxData(uint8_t *DataOut, 
                          uint32_t dataSize);
                                                         
    uint32_t targetRxData(uint8_t *response);  
    
    boolean isTargetReleasedError(uint32_t result);
    
    void clearBuffer();

private:
    uint8_t _irq, _reset;
    void sleepArduino();
    boolean fetchCheckAck(uint16_t timeout = 1000);
    uint8_t checkDataAvailable(void);
    uint32_t fetchResponse(uint8_t cmdCode, PN532_CMD_RESPONSE *reponse);
    void fetchData(uint8_t* buff, uint16_t timeout = 1000); //debugg=?
    void sendFrame(uint8_t* cmd, uint8_t cmdlen);
    void wiresend(uint8_t c);
    uint8_t wirerecv(void);


};



