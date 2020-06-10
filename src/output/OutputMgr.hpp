// only include this file once
#pragma once
/*
* OutputMgr.hpp - Output Management class
*
* Project: ESPixelStick - An ESP8266 / ESP32 and E1.31 based pixel driver
* Copyright (c) 2019 Shelby Merrick
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
*   This is a factory class used to manage the output port. It creates and deletes
*   the output channel functionality as needed to support any new configurations 
*   that get sent from from the WebPage.
*
*/

#include <Arduino.h>
#include <ArduinoJson.h>

#include "..\memdebug.h"
#include "..\FileIO.h"

class c_OutputCommon; ///< forward declaration to the pure virtual output class that will be defined later.

class c_OutputMgr
{
public:
    c_OutputMgr ();
    virtual ~c_OutputMgr ();

    void    Begin ();                            ///< set up the operating environment based on the current config (or defaults)
    void    LoadConfig ();                       ///< Read the current configuration data from nvram
    void    SaveConfig ();                       ///< Save the current configuration data to nvram
    void    GetConfig (JsonObject & jsonConfig); ///< Get the current config used by the driver
    String  GetConfig ();
    bool    SetConfig (JsonObject & jsonConfig); ///< Set a new config in the driver

    // handles to determine which output channel we are dealing with
    enum e_OutputChannelIds
    {
        OutputChannelId_1 = 0,
#ifdef ARDUINO_ARCH_ESP32
        OutputChannelId_2,
        OutputChannelId_3,
#endif // def ARDUINO_ARCH_ESP32
        OutputChannelId_End,
        OutputChannelId_Start = OutputChannelId_1
    };

    uint8_t * GetBufferAddress (e_OutputChannelIds ChannelId = OutputChannelId_1); ///< Get the address of the buffer into which the E1.31 handler will stuff data
    uint16_t  GetBufferSize    (e_OutputChannelIds ChannelId = OutputChannelId_1); ///< Get the size (in intensities) of the buffer into which the E1.31 handler will stuff data
    void      Render ();                                                           ///< Call from loop(),  renders output data

    enum e_OutputType
    {
        OutputType_WS2811 = 0,
        OutputType_GECE,
        OutputType_Serial,
        OutputType_Renard,
        OutputType_DMX,
#ifdef ARDUINO_ARCH_ESP32
        OutputType_SPI,
#endif // def ARDUINO_ARCH_ESP32
        OutputType_Disabled, // must be last
        OutputType_End,
        OutputType_Start = OutputType_WS2811,
    };

private:

    void InstantiateNewOutputChannel (e_OutputChannelIds ChannelIndex, e_OutputType NewChannelType);
    void CreateNewConfig ();
    void merge (JsonVariant dst, JsonVariantConst src);

    c_OutputCommon * pOutputChannelDrivers[e_OutputChannelIds::OutputChannelId_End]; ///< pointer(s) to the current active output driver

    // configuration parameter names for the channel manager within the config file
#   define OM_SECTION_NAME         F("om_config")
#   define OM_CHANNEL_SECTION_NAME F("om_channels")
#   define OM_CHANNEL_TYPE_NAME    F("om_channel_type")
#   define OM_CHANNEL_DATA_NAME    F("om_channel_data")

    bool HasBeenInitialized = false;

    bool DeserializeConfig (JsonObject & jsonConfig);
    void SerializeConfig   (JsonObject & jsonConfig);
    // JsonObject JsonConfig = JsonConfigDoc.as<JsonObject> ();

    File ConfigFile;
    String ConfigFileName;

protected:

}; // c_OutputMgr

extern c_OutputMgr OutputMgr;
