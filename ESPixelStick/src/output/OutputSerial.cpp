/*
* OutputSerial.cpp - Pixel driver code for ESPixelStick UART
*
* Project: ESPixelStick - An ESP8266 / ESP32 and E1.31 based pixel driver
* Copyright (c) 2015 Shelby Merrick
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
#if defined(SUPPORT_OutputType_DMX) || defined(SUPPORT_OutputType_Serial) || defined(SUPPORT_OutputType_Renard)

#include "OutputSerial.hpp"
#define ADJUST_INTENSITY_AT_ISR

//----------------------------------------------------------------------------
c_OutputSerial::c_OutputSerial (c_OutputMgr::e_OutputChannelIds OutputChannelId,
    gpio_num_t outputGpio,
    uart_port_t uart,
    c_OutputMgr::e_OutputType outputType) :
    c_OutputCommon (OutputChannelId, outputGpio, uart, outputType)
{
    // DEBUG_START;
#if defined(SUPPORT_OutputType_DMX)
    if (outputType == c_OutputMgr::e_OutputType::OutputType_DMX)
    {
        CurrentBaudrate = uint32_t(BaudRate::BR_DMX);
    }
#endif // defined(SUPPORT_OutputType_DMX)
    // DEBUG_END;
} // c_OutputSerial

//----------------------------------------------------------------------------
c_OutputSerial::~c_OutputSerial ()
{
    // DEBUG_START;

    // DEBUG_END;
} // ~c_OutputSerial

//----------------------------------------------------------------------------
void c_OutputSerial::Begin()
{
    // DEBUG_START;
#if defined(SUPPORT_OutputType_DMX)
    if (OutputType == c_OutputMgr::e_OutputType::OutputType_DMX)
    {
        CurrentBaudrate = uint32_t(BaudRate::BR_DMX);
    }
#endif // defined(SUPPORT_OutputType_DMX)
    // DEBUG_END;
} // Begin

//----------------------------------------------------------------------------
void c_OutputSerial::GetConfig(ArduinoJson::JsonObject &jsonConfig)
{
    // DEBUG_START;

    jsonConfig[CN_gen_ser_hdr] = GenericSerialHeader;
    jsonConfig[CN_gen_ser_ftr] = GenericSerialFooter;
    jsonConfig[CN_num_chan]    = Num_Channels;
    jsonConfig[CN_baudrate]    = CurrentBaudrate;

    c_OutputCommon::GetConfig (jsonConfig);

    // DEBUG_END;
} // GetConfig

//----------------------------------------------------------------------------
void c_OutputSerial::GetStatus (ArduinoJson::JsonObject& jsonStatus)
{
    // DEBUG_START;

    c_OutputCommon::GetStatus (jsonStatus);

#ifdef USE_SERIAL_DEBUG_COUNTERS
    JsonObject debugStatus = jsonStatus.createNestedObject("Serial Debug");
    debugStatus["Num_Channels"]                = Num_Channels;
    debugStatus["NextIntensityToSend"]         = String(int(NextIntensityToSend), HEX);
    debugStatus["IntensityBytesSent"]          = IntensityBytesSent;
    debugStatus["IntensityBytesSentLastFrame"] = IntensityBytesSentLastFrame;
    debugStatus["FrameStartCounter"]           = FrameStartCounter;
    debugStatus["FrameEndCounter"]             = FrameEndCounter;
    debugStatus["AbortFrameCounter"]           = AbortFrameCounter;
#endif // def USE_SERIAL_DEBUG_COUNTERS

    // DEBUG_END;
} // GetStatus

//----------------------------------------------------------------------------
void c_OutputSerial::GetDriverName(String &sDriverName)
{
    switch (OutputType)
    {
#ifdef SUPPORT_OutputType_Serial
    case c_OutputMgr::e_OutputType::OutputType_Serial:
    {
        sDriverName = F("Serial");
        break;
    }
#endif // def SUPPORT_OutputType_Serial

#ifdef SUPPORT_OutputType_DMX
    case c_OutputMgr::e_OutputType::OutputType_DMX:
    {
        sDriverName = F("DMX");
        break;
    }
#endif // def SUPPORT_OutputType_DMX

#ifdef SUPPORT_OutputType_Renard
    case c_OutputMgr::e_OutputType::OutputType_Renard:
    {
        sDriverName = F("Renard");
        break;
    }
#endif // def SUPPORT_OutputType_Renard

    default:
    {
        sDriverName = F("Default");
        break;
    }
    } // switch (OutputType)

} // GetDriverName

//----------------------------------------------------------------------------
void c_OutputSerial::SetOutputBufferSize(size_t NumChannelsAvailable)
{
    // DEBUG_START;
    // DEBUG_V (String ("NumChannelsAvailable: ") + String (NumChannelsAvailable));
    // DEBUG_V (String ("   GetBufferUsedSize: ") + String (c_OutputCommon::GetBufferUsedSize ()));
    // DEBUG_V (String ("    OutputBufferSize: ") + String (OutputBufferSize));
    // DEBUG_V (String ("       BufferAddress: ") + String ((uint32_t)(c_OutputCommon::GetBufferAddress ())));

    do // once
    {
        // are we changing size?
        if (NumChannelsAvailable == OutputBufferSize)
        {
            // DEBUG_V ("NO Need to change anything");
            break;
        }

        c_OutputCommon::SetOutputBufferSize(NumChannelsAvailable);

        // Calculate our refresh time
        SetFrameDurration();

    } while (false);

    // DEBUG_END;
} // SetBufferSize

//----------------------------------------------------------------------------
bool c_OutputSerial::SetConfig (ArduinoJson::JsonObject& jsonConfig)
{
    // DEBUG_START;

#if defined(SUPPORT_OutputType_DMX)
    if (OutputType == c_OutputMgr::e_OutputType::OutputType_DMX)
    {
        jsonConfig[CN_baudrate] = uint32_t(BaudRate::BR_DMX);
    }
#endif // defined(SUPPORT_OutputType_DMX)

    setFromJSON(GenericSerialHeader, jsonConfig, CN_gen_ser_hdr);
    setFromJSON(GenericSerialFooter, jsonConfig, CN_gen_ser_ftr);
    setFromJSON(Num_Channels,        jsonConfig, CN_num_chan);
    setFromJSON(CurrentBaudrate,     jsonConfig, CN_baudrate);

    c_OutputCommon::SetConfig(jsonConfig);
    bool response = validate();

    SerialHeaderSize = GenericSerialHeader.length();
    SerialFooterSize = GenericSerialFooter.length();
    SetFrameDurration();

    // Update the config fields in case the validator changed them
    GetConfig(jsonConfig);

    // DEBUG_END;
    return response;

} // SetConfig

//----------------------------------------------------------------------------
bool c_OutputSerial::validate ()
{
    // DEBUG_START;
    bool response = true;

    if ((Num_Channels > MAX_CHANNELS) || (Num_Channels < 1))
    {
        logcon(CN_stars + String(F(" Requested channel count was not valid. Setting to ")) + MAX_CHANNELS + " " + CN_stars);
        Num_Channels = DEFAULT_NUM_CHANNELS;
        response = false;
    }
    SetOutputBufferSize(Num_Channels);

    if ((CurrentBaudrate < uint32_t(BaudRate::BR_MIN)) || (CurrentBaudrate > uint32_t(BaudRate::BR_MAX)))
    {
        logcon(CN_stars + String(F(" Requested baudrate is not valid. Setting to Default ")) + CN_stars);
        CurrentBaudrate = uint32_t(BaudRate::BR_DEF);
        response = false;
    }

#if defined(SUPPORT_OutputType_DMX)
    if (OutputType == c_OutputMgr::e_OutputType::OutputType_DMX)
    {
        CurrentBaudrate = uint32_t(BaudRate::BR_DMX);
    }
#endif // defined(SUPPORT_OutputType_DMX)

    if (GenericSerialHeader.length() > MAX_HDR_SIZE)
    {
        logcon(CN_stars + String(F(" Requested header is too long. Setting to Default ")) + CN_stars);
        GenericSerialHeader = "";
    }

    if (GenericSerialFooter.length() > MAX_FOOTER_SIZE)
    {
        logcon(CN_stars + String(F(" Requested footer is too long. Setting to Default ")) + CN_stars);
        GenericSerialFooter = "";
    }

    // DEBUG_END;
    return response;

} // validate

//----------------------------------------------------------------------------
void c_OutputSerial::SetFrameDurration ()
{
    // DEBUG_START;
    float IntensityBitTimeInUs     = (1.0 / float(CurrentBaudrate)) * 1000000.0;
    float TotalIntensitiesPerFrame = float(Num_Channels + 1) + SerialHeaderSize + SerialFooterSize;
    float TotalBitsPerFrame        = float(NumBitsPerIntensity) * TotalIntensitiesPerFrame;
    uint32_t TotalFrameTimeInUs    = uint32_t(IntensityBitTimeInUs * TotalBitsPerFrame) + InterFrameGapInMicroSec;
    FrameMinDurationInMicroSec     = max(uint32_t(25000), TotalFrameTimeInUs);

    // DEBUG_V (String ("           CurrentBaudrate: ") + String (CurrentBaudrate));
    // DEBUG_V (String ("      IntensityBitTimeInUs: ") + String (IntensityBitTimeInUs));
    // DEBUG_V (String ("          SerialHeaderSize: ") + String (SerialHeaderSize));
    // DEBUG_V (String ("          SerialFooterSize: ") + String (SerialFooterSize));
    // DEBUG_V (String ("  TotalIntensitiesPerFrame: ") + String (TotalIntensitiesPerFrame));
    // DEBUG_V (String ("         TotalBitsPerFrame: ") + String (TotalBitsPerFrame));
    // DEBUG_V (String ("   InterFrameGapInMicroSec: ") + String (InterFrameGapInMicroSec));
    // DEBUG_V (String ("        TotalFrameTimeInUs: ") + String (TotalFrameTimeInUs));
    // DEBUG_V (String ("   InterFrameGapInMicroSec: ") + String (InterFrameGapInMicroSec));
    // DEBUG_V (String ("FrameMinDurationInMicroSec: ") + String (FrameMinDurationInMicroSec));

    // DEBUG_END;

} // SetFrameDurration

//----------------------------------------------------------------------------
void c_OutputSerial::StartNewFrame ()
{
    // DEBUG_START;

#ifdef USE_SERIAL_DEBUG_COUNTERS
    if (ISR_MoreDataToSend ())
    {
        AbortFrameCounter++;
    }
    FrameStartCounter++;
#endif // def USE_SERIAL_DEBUG_COUNTERS

    NextIntensityToSend = GetBufferAddress();
    intensity_count     = Num_Channels;
    SentIntensityCount  = 0;
    SerialHeaderIndex   = 0;
    SerialFooterIndex   = 0;

#ifdef USE_SERIAL_DEBUG_COUNTERS
    IntensityBytesSentLastFrame = IntensityBytesSent;
    IntensityBytesSent = 0;
    // IntensityBytesSentLastFrame = 0;
#endif // def USE_SERIAL_DEBUG_COUNTERS

    // start the next frame
    switch (OutputType)
    {
#ifdef SUPPORT_OutputType_DMX
        case c_OutputMgr::e_OutputType::OutputType_DMX:
        {
            SerialFrameState = SerialFrameState_t::DMXSendFrameStart;
            break;
        }  // DMX512
#endif // def SUPPORT_OutputType_DMX

#ifdef SUPPORT_OutputType_Renard
        case c_OutputMgr::e_OutputType::OutputType_Renard:
        {
            SerialFrameState = SerialFrameState_t::RenardFrameStart;
            break;
        }  // RENARD
#endif // def SUPPORT_OutputType_Renard

#ifdef SUPPORT_OutputType_Serial
        case c_OutputMgr::e_OutputType::OutputType_Serial:
        {
            SerialFrameState = (SerialHeaderSize) ? SerialFrameState_t::GenSerSendHeader : SerialFrameState_t::GenSerSendData;
        }  // GENERIC
#endif // def SUPPORT_OutputType_Serial

        default:
        {
            break;
        } // this is not possible but the language needs it here

    } // end switch (OutputType)

    // DEBUG_END;

} // StartNewFrame

//----------------------------------------------------------------------------
uint8_t IRAM_ATTR c_OutputSerial::ISR_GetNextIntensityToSend ()
{
    uint8_t data = 0x00;

#ifdef USE_SERIAL_DEBUG_COUNTERS
    IntensityBytesSent++;
#endif // def USE_SERIAL_DEBUG_COUNTERS

    switch (SerialFrameState)
    {
        case SerialFrameState_t::RenardFrameStart:
        {
            data = RenardFrameDefinitions_t::FRAME_START_CHAR;
            SerialFrameState = SerialFrameState_t::RenardDataStart;
            break;
        }

        case SerialFrameState_t::RenardDataStart:
        {
            data = RenardFrameDefinitions_t::CMD_DATA_START;
            SerialFrameState = SerialFrameState_t::RenardSendData;
            break;
        }

        case SerialFrameState_t::RenardSendData:
        {
            data = *NextIntensityToSend;
            // do we have to adjust the renard data stream?
            if ((data >= RenardFrameDefinitions_t::MIN_VAL_TO_ESC) &&
                (data <= RenardFrameDefinitions_t::MAX_VAL_TO_ESC))
            {
                // Send a two byte substitute for the value
                data = RenardFrameDefinitions_t::ESC_CHAR;
                SerialFrameState = SerialFrameState_t::RenardSendEscapedData;

            } // end modified data
            else
            {
                ++NextIntensityToSend;
                if (0 == --intensity_count)
                {
                    SerialFrameState = SerialFrameState_t::SerialIdle;
#ifdef USE_SERIAL_DEBUG_COUNTERS
                    ++FrameEndCounter;
#endif // def USE_SERIAL_DEBUG_COUNTERS
                }
            }

            break;
        }

        case SerialFrameState_t::RenardSendEscapedData:
        {
            data = *NextIntensityToSend - uint8_t(RenardFrameDefinitions_t::ESCAPED_OFFSET);
            ++NextIntensityToSend;           
            if (0 == --intensity_count)
            {
                SerialFrameState = SerialFrameState_t::SerialIdle;
#ifdef USE_SERIAL_DEBUG_COUNTERS
                ++FrameEndCounter;
#endif // def USE_SERIAL_DEBUG_COUNTERS
            }
            else
            {
                SerialFrameState = SerialFrameState_t::RenardSendData;
            }
            break;
        }

        case SerialFrameState_t::DMXSendFrameStart:
        {
            data = 0x00; // DMX Lighting frame start
            SerialFrameState = SerialFrameState_t::DMXSendData;
            break;
        }

        case SerialFrameState_t::DMXSendData:
        {
            data = *NextIntensityToSend++;
            if (0 == --intensity_count)
            {
                SerialFrameState = SerialFrameState_t::SerialIdle;
#ifdef USE_SERIAL_DEBUG_COUNTERS
                ++FrameEndCounter;
#endif // def USE_SERIAL_DEBUG_COUNTERS
            }
            break;
        }

        case SerialFrameState_t::GenSerSendHeader:
        {
            data = GenericSerialHeader[SerialHeaderIndex++];
            if (SerialHeaderSize <= SerialHeaderIndex)
            {
                SerialFrameState = SerialFrameState_t::GenSerSendData;
            }
            break;
        }

        case SerialFrameState_t::GenSerSendData:
        {
            data = *NextIntensityToSend++;
            if (0 == --intensity_count)
            {
                if (SerialFooterSize)
                {
                    SerialFrameState = SerialFrameState_t::GenSerSendFooter;
                }
                else
                {
                    SerialFrameState = SerialFrameState_t::SerialIdle;
#ifdef USE_SERIAL_DEBUG_COUNTERS
                    ++FrameEndCounter;
#endif // def USE_SERIAL_DEBUG_COUNTERS
                }
            }
            break;
        }

        case SerialFrameState_t::GenSerSendFooter:
        {
            data = GenericSerialFooter[SerialFooterIndex++];
            if (SerialFooterSize <= SerialFooterIndex)
            {
                SerialFrameState = SerialFrameState_t::SerialIdle;
#ifdef USE_SERIAL_DEBUG_COUNTERS
                ++FrameEndCounter;
#endif // def USE_SERIAL_DEBUG_COUNTERS
            }
            break;
        }

        case SerialFrameState_t::SerialIdle:
        default:
        {
            break;
        }
    } // switch SerialFrameState

/*
    if (InvertData)
    {
        response = ~response;
    }
*/
    return data;
} // NextIntensityToSend

#endif // defined(SUPPORT_OutputType_DMX) || defined(SUPPORT_OutputType_Serial) || defined(SUPPORT_OutputType_Renard)
