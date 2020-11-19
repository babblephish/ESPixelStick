#pragma once
/*
* c_FPPDiscovery.h
*
* Project: ESPixelStick - An ESP8266 / ESP32 and E1.31 based pixel driver
* Copyright (c) 2018 Shelby Merrick
* http://www.forkineye.com
*
*  This program is provided free for you to use in any way that you wish,
*  subject to the laws and regulations where you are using it.  Due diligence
*  is strongly suggested before using this code.  Please give credit where due.
*
*  The Author makes no warranty of any kind, express or implied, with regard
*  to this program or the documentation contained in this document.  The
*  Author shall not be liable in any event for incidental or consequential
*  damages in connection with, or arising out of, the furnishing, performance
*  or use of these programs.
*
*/

#include "../ESPixelStick.h"

#ifdef ESP32
#include <WiFi.h>
#include <AsyncUDP.h>
#elif defined (ESP8266)
#include <ESPAsyncUDP.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#else
#error Platform not supported
#endif


#include <ESPAsyncWebServer.h>

class c_FPPDiscovery
{
public:

#   define SD_CARD_MISO_PIN    19
#   define SD_CARD_MOSI_PIN    23 
#   define SD_CARD_CLK_PIN     18
#   define SD_CARD_CS_PIN      4
#   define Stop_FPP_RemotePlay "..."
private:

    AsyncUDP udp;
    void ProcessReceivedUdpPacket (AsyncUDPPacket _packet);
    void ProcessSyncPacket (uint8_t action, String filename, uint32_t frame);
    void ProcessBlankPacket ();

    bool isRemoteRunning = false;
    File fseqFile;
    String fseqName = "";
    String failedFseqName = "";
    String AutoPlayFileName = Stop_FPP_RemotePlay;
    unsigned long fseqStartMillis = 0;
    int fseqCurrentFrameId = 0;
    uint32_t dataOffset = 0;
    uint32_t channelsPerFrame = 0;
    uint8_t  frameStepTime = 0;
    uint32_t TotalNumberOfFramesInSequence = 0;
    uint8_t  miso_pin = SD_CARD_MISO_PIN;
    uint8_t  mosi_pin = SD_CARD_MOSI_PIN;
    uint8_t  clk_pin  = SD_CARD_CLK_PIN;
    uint8_t  cs_pin   = SD_CARD_CS_PIN;
    uint8_t* outputBuffer;
    uint16_t outputBufferSize;
    bool hasSDStorage = false;
    bool inFileUpload = false;
    bool hasBeenInitialized = false;
    uint8_t* buffer = nullptr;
    int bufCurPos = 0;

    void GetSysInfoJSON (JsonObject& jsonResponse);
    void DescribeSdCardToUser ();
    void BuildFseqResponse (String fname, File fseq, String & resp);
    void StopPlaying ();
    void StartPlaying (String & filename, uint32_t frameId);
    void printDirectory (File dir, int numTabs);
    bool AllowedToRemotePlayFiles ();

public:
    c_FPPDiscovery ();

    void begin ();

    void ProcessFPPJson (AsyncWebServerRequest* request);
    void ProcessGET     (AsyncWebServerRequest* request);
    void ProcessPOST    (AsyncWebServerRequest* request);
    void ProcessFile    (AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final);
    void ProcessBody    (AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total);
    void ReadNextFrame  (uint8_t* outputBuffer, uint16_t outputBufferSize);
    void GetListOfFiles (char * ResponseBuffer);
    void DeleteFseqFile (String & FileNameToDelete);
    void sendPingPacket ();
    void SetSpiIoPins   (uint8_t miso, uint8_t mosi, uint8_t clock, uint8_t cs);
    void PlayFile       (String & FileToPlay);

    bool SdcardIsInstalled () { return hasSDStorage; }
};

extern c_FPPDiscovery FPPDiscovery;