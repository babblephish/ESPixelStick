/*
* wshandler.h
*
* Project: ESPixelStick - An ESP8266 and E1.31 based pixel driver
* Copyright (c) 2016 Shelby Merrick
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

#ifndef WSHANDLER_H_
#define WSHANDLER_H_

#if defined(ESPS_MODE_PIXEL)
#include "PixelDriver.h"
extern PixelDriver  pixels;     // Pixel object
#elif defined(ESPS_MODE_SERIAL)
#include "SerialDriver.h"
extern SerialDriver serial;     // Serial object
#endif

extern EffectEngine effects;    // EffectEngine for test modes

extern ESPAsyncE131 e131;       // ESPAsyncE131 with X buffers
extern config_t     config;     // Current configuration
extern uint32_t     *seqError;  // Sequence error tracking for each universe
extern uint16_t     uniLast;    // Last Universe to listen for
extern bool         reboot;     // Reboot flag

extern const char CONFIG_FILE[];

/*
  Packet Commands
    E1 - Get Elements

    G1 - Get Config
    G2 - Get Config Status
    G3 - Get Current Effect and Effect Config Options

    T0 - Disable Testing
    T1 - Static Testing
    T2 - Blink Test
    T3 - Flash Test
    T4 - Chase Test
    T5 - Rainbow Test
    T6 - Fire flicker
    T7 - Lightning
    T8 - Breathe
    T9 - View Stream

    S1 - Set Network Config
    S2 - Set Device Config
    S3 - Set Effect Startup Config

    XS - Get RSSI:heap:uptime
    X2 - Get E131 Status
    X6 - Reboot
*/

EFUpdate efupdate;
uint8_t * WSframetemp;
uint8_t * confuploadtemp;

void procX(uint8_t *data, AsyncWebSocketClient *client) {
    switch (data[1]) {
        case 'S':
            client->text("XS" +
                     (String)WiFi.RSSI() + ":" +
                     (String)ESP.getFreeHeap() + ":" +
                     (String)millis());
            break;
        case '2': {
            uint32_t seqErrors = 0;
            for (int i = 0; i < ((uniLast + 1) - config.universe); i++)
                seqErrors =+ seqError[i];
            client->text("X2" + (String)config.universe + ":" +
                    (String)uniLast + ":" +
                    (String)e131.stats.num_packets + ":" +
                    (String)seqErrors + ":" +
                    (String)e131.stats.packet_errors + ":" +
                    e131.stats.last_clientIP.toString());
            break;
        }
        case '6':  // Init 6 baby, reboot!
            reboot = true;
    }
}

void procE(uint8_t *data, AsyncWebSocketClient *client) {
    switch (data[1]) {
        case '1':
            // Create buffer and root object
            DynamicJsonBuffer jsonBuffer;
            JsonObject &json = jsonBuffer.createObject();

#if defined (ESPS_MODE_PIXEL)
            // Pixel Types
            JsonObject &p_type = json.createNestedObject("p_type");
            p_type["WS2811 800kHz"] = static_cast<uint8_t>(PixelType::WS2811);
            p_type["GE Color Effects"] = static_cast<uint8_t>(PixelType::GECE);

            // Pixel Colors
            JsonObject &p_color = json.createNestedObject("p_color");
            p_color["RGB"] = static_cast<uint8_t>(PixelColor::RGB);
            p_color["GRB"] = static_cast<uint8_t>(PixelColor::GRB);
            p_color["BRG"] = static_cast<uint8_t>(PixelColor::BRG);
            p_color["RBG"] = static_cast<uint8_t>(PixelColor::RBG);
            p_color["GBR"] = static_cast<uint8_t>(PixelColor::GBR);
            p_color["BGR"] = static_cast<uint8_t>(PixelColor::BGR);

#elif defined (ESPS_MODE_SERIAL)
            // Serial Protocols
            JsonObject &s_proto = json.createNestedObject("s_proto");
            s_proto["DMX512"] = static_cast<uint8_t>(SerialType::DMX512);
            s_proto["Renard"] = static_cast<uint8_t>(SerialType::RENARD);

            // Serial Baudrates
            JsonObject &s_baud = json.createNestedObject("s_baud");
            s_baud["38400"] = static_cast<uint32_t>(BaudRate::BR_38400);
            s_baud["57600"] = static_cast<uint32_t>(BaudRate::BR_57600);
            s_baud["115200"] = static_cast<uint32_t>(BaudRate::BR_115200);
            s_baud["230400"] = static_cast<uint32_t>(BaudRate::BR_230400);
            s_baud["250000"] = static_cast<uint32_t>(BaudRate::BR_250000);
            s_baud["460800"] = static_cast<uint32_t>(BaudRate::BR_460800);
#endif

            String response;
            json.printTo(response);
            client->text("E1" + response);
            break;
    }
}

void procG(uint8_t *data, AsyncWebSocketClient *client) {
    switch (data[1]) {
        case '1': {
            String response;
            serializeConfig(response, false, true);
            client->text("G1" + response);
            break;
        }

        case '2': {
            // Create buffer and root object
            DynamicJsonBuffer jsonBuffer;
            JsonObject &json = jsonBuffer.createObject();

            json["ssid"] = (String)WiFi.SSID();
            json["hostname"] = (String)WiFi.hostname();
            json["ip"] = WiFi.localIP().toString();
            json["mac"] = WiFi.macAddress();
            json["version"] = (String)VERSION;
            json["built"] = (String)BUILD_DATE;
            json["flashchipid"] = String(ESP.getFlashChipId(), HEX);
            json["usedflashsize"] = (String)ESP.getFlashChipSize();
            json["realflashsize"] = (String)ESP.getFlashChipRealSize();
            json["freeheap"] = (String)ESP.getFreeHeap();

            String response;
            json.printTo(response);
            client->text("G2" + response);
            break;
        }

        case '3': {
            String response;
            DynamicJsonBuffer jsonBuffer;
            JsonObject &json = jsonBuffer.createObject();

// dump the current running effect options
            JsonObject &effect = json.createNestedObject("currentEffect");
            effect["name"] = (String)effects.getEffect() ? effects.getEffect() : "";
            effect["brightness"] = effects.getBrightness();
            effect["speed"] = effects.getSpeed();
            effect["r"] = effects.getColor().r;
            effect["g"] = effects.getColor().g;
            effect["b"] = effects.getColor().b;
            effect["reverse"] = effects.getReverse();
            effect["mirror"] = effects.getMirror();
            effect["allleds"] = effects.getAllLeds();
            effect["startenabled"] = config.effect_startenabled;
            effect["idleenabled"] = config.effect_idleenabled;
            effect["idletimeout"] = config.effect_idletimeout;


// dump all the known effect and options
            JsonObject &effectList = json.createNestedObject("effectList");
            for(int i=0; i < effects.getEffectCount(); i++){
                JsonObject &effect = effectList.createNestedObject( effects.getEffectInfo(i)->htmlid );
                effect["name"] = effects.getEffectInfo(i)->name;
                effect["htmlid"] = effects.getEffectInfo(i)->htmlid;
                effect["hasColor"] = effects.getEffectInfo(i)->hasColor;
                effect["hasMirror"] = effects.getEffectInfo(i)->hasMirror;
                effect["hasReverse"] = effects.getEffectInfo(i)->hasReverse;
                effect["hasAllLeds"] = effects.getEffectInfo(i)->hasAllLeds;
                effect["wsTCode"] = effects.getEffectInfo(i)->wsTCode;
//            effect["brightness"] = effects.getBrightness();
//            effect["speed"] = effects.getSpeed();
            }

            json.printTo(response);
            client->text("G3" + response);
//LOG_PORT.print(response);
            break;
        }
    }
}

void procS(uint8_t *data, AsyncWebSocketClient *client) {
    DynamicJsonBuffer jsonBuffer;
    JsonObject &json = jsonBuffer.parseObject(reinterpret_cast<char*>(data + 2));
    if (!json.success()) {
        LOG_PORT.println(F("*** procS(): Parse Error ***"));
        LOG_PORT.println(reinterpret_cast<char*>(data));
        return;
    }

    bool reboot = false;
    switch (data[1]) {
        case '1':   // Set Network Config
            dsNetworkConfig(json);
            saveConfig();
            client->text("S1");
            break;
        case '2':   // Set Device Config
            // Reboot if MQTT changed
            if (config.mqtt != json["mqtt"]["enabled"])
                reboot = true;

            dsDeviceConfig(json);
            saveConfig();

            if (reboot)
                client->text("S1");
            else
                client->text("S2");
            break;
        case '3':   // Set Effect Startup Config
            dsEffectConfig(json);
            saveConfig();
            client->text("S3");
            break;
    }
}

void procT(uint8_t *data, AsyncWebSocketClient *client) {
    // T9 is view stream, DONT change source when we get this
    if ( (data[1] >= '1') && (data[1] <= '8') ) config.ds = DataSource::WEB;

    switch (data[1]) {
        case '0': { // Clear whole string
            //TODO: Store previous data source when effect is selected so we can switch back to it
            config.ds = DataSource::E131;
            effects.clearAll();
            break;
        }
        case '1': {  // Static color
            DynamicJsonBuffer jsonBuffer;
            JsonObject &json = jsonBuffer.parseObject(reinterpret_cast<char*>(data + 2));

            if (json.containsKey("r") && json.containsKey("g") && json.containsKey("b")) {
                effects.setColor({json["r"], json["g"], json["b"]});
            }

            effects.setEffect("Solid");
            break;
        }
        case '2': {  // Blink
            DynamicJsonBuffer jsonBuffer;
            JsonObject &json = jsonBuffer.parseObject(reinterpret_cast<char*>(data + 2));

            if (json.containsKey("r") && json.containsKey("g") && json.containsKey("b")) {
                effects.setColor({json["r"], json["g"], json["b"]});
            }

            effects.setEffect("Blink");
            break;
        }
        case '3': {  // Flash
            DynamicJsonBuffer jsonBuffer;
            JsonObject &json = jsonBuffer.parseObject(reinterpret_cast<char*>(data + 2));

            if (json.containsKey("r") && json.containsKey("g") && json.containsKey("b")) {
                effects.setColor({json["r"], json["g"], json["b"]});
            }

            effects.setEffect("Flash");
            break;
        }
        case '4': {  // Chase
            DynamicJsonBuffer jsonBuffer;
            JsonObject &json = jsonBuffer.parseObject(reinterpret_cast<char*>(data + 2));

            if (json.containsKey("r") && json.containsKey("g") && json.containsKey("b")) {
                effects.setColor({json["r"], json["g"], json["b"]});
            }

            if (json.containsKey("reverse")) {
                effects.setReverse(json["reverse"]);
            }

            if (json.containsKey("mirror")) {
                effects.setMirror(json["mirror"]);
            }

            effects.setEffect("Chase");
            break;
        }
        case '5': { // Rainbow
            DynamicJsonBuffer jsonBuffer;
            JsonObject &json = jsonBuffer.parseObject(reinterpret_cast<char*>(data + 2));

            if (json.containsKey("reverse")) {
                effects.setReverse(json["reverse"]);
            }

            if (json.containsKey("mirror")) {
                effects.setMirror(json["mirror"]);
            }

            if (json.containsKey("allleds")) {
                effects.setAllLeds(json["allleds"]);
            }

            effects.setEffect("Rainbow");
            break;
        }
        case '6': { // Fire flicker
            DynamicJsonBuffer jsonBuffer;
            JsonObject &json = jsonBuffer.parseObject(reinterpret_cast<char*>(data + 2));

            if (json.containsKey("r") && json.containsKey("g") && json.containsKey("b")) {
                effects.setColor({json["r"], json["g"], json["b"]});
            }

            effects.setEffect("Fire flicker");
            break;
        }
        case '7': { // Lightning
            DynamicJsonBuffer jsonBuffer;
            JsonObject &json = jsonBuffer.parseObject(reinterpret_cast<char*>(data + 2));

            if (json.containsKey("r") && json.containsKey("g") && json.containsKey("b")) {
                effects.setColor({json["r"], json["g"], json["b"]});
            }

            effects.setEffect("Lightning");
            break;
        }
        case '8': { // Breathe
            DynamicJsonBuffer jsonBuffer;
            JsonObject &json = jsonBuffer.parseObject(reinterpret_cast<char*>(data + 2));

            if (json.containsKey("r") && json.containsKey("g") && json.containsKey("b")) {
                effects.setColor({json["r"], json["g"], json["b"]});
            }

            effects.setEffect("Breathe");
            break;
        }

        case '9': {  // View stream
#if defined(ESPS_MODE_PIXEL)
            client->binary(pixels.getData(), config.channel_count);
#elif defined(ESPS_MODE_SERIAL)
            if (config.serial_type == SerialType::DMX512)
                client->binary(&serial.getData()[1], config.channel_count);
            else
                client->binary(&serial.getData()[2], config.channel_count);
#endif
            break;
        }
    }
}

void handle_fw_upload(AsyncWebServerRequest *request, String filename,
        size_t index, uint8_t *data, size_t len, bool final) {
    if (!index) {
        WiFiUDP::stopAll();
        LOG_PORT.print(F("* Upload Started: "));
        LOG_PORT.println(filename.c_str());
        efupdate.begin();
    }

    if (!efupdate.process(data, len)) {
        LOG_PORT.print(F("*** UPDATE ERROR: "));
        LOG_PORT.println(String(efupdate.getError()));
    }

    if (efupdate.hasError())
        request->send(200, "text/plain", "Update Error: " +
                String(efupdate.getError()));

    if (final) {
        request->send(200, "text/plain", "Update Finished: " +
                String(efupdate.getError()));
        LOG_PORT.println(F("* Upload Finished."));
        efupdate.end();
        SPIFFS.begin();
        saveConfig();
        reboot = true;
    }
}

void handle_config_upload(AsyncWebServerRequest *request, String filename,
        size_t index, uint8_t *data, size_t len, bool final) {
    static File file;
    if (!index) {
        WiFiUDP::stopAll();
        LOG_PORT.print(F("* Config Upload Started: "));
        LOG_PORT.println(filename.c_str());

        if (confuploadtemp) {
          free (confuploadtemp);
          confuploadtemp = nullptr;
        }
        confuploadtemp = (uint8_t*) malloc(CONFIG_MAX_SIZE);
    }

    LOG_PORT.printf("index %d len %d\n", index, len);
    memcpy(confuploadtemp + index, data, len);
    confuploadtemp[index + len] = 0;

    if (final) {
        int filesize = index+len;
        LOG_PORT.print(F("* Config Upload Finished:"));
        LOG_PORT.printf(" %d bytes", filesize);

        DynamicJsonBuffer jsonBuffer;
        JsonObject &json = jsonBuffer.parseObject(reinterpret_cast<char*>(confuploadtemp));
        if (!json.success()) {
            LOG_PORT.println(F("*** Parse Error ***"));
            LOG_PORT.println(reinterpret_cast<char*>(confuploadtemp));
            request->send(500, "text/plain", "Config Update Error." );
        } else {
            dsNetworkConfig(json);
//          dsDeviceConfig(json);
            dsEffectConfig(json);
            saveConfig();
            request->send(200, "text/plain", "Config Update Finished: " );
//          reboot = true;
        }

        if (confuploadtemp) {
            free (confuploadtemp);
            confuploadtemp = nullptr;
        }
    }
}

void wsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
        AwsEventType type, void * arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_DATA: {
            AwsFrameInfo *info = static_cast<AwsFrameInfo*>(arg);
            if (info->opcode == WS_TEXT) {
                switch (data[0]) {
                    case 'X':
                        procX(data, client);
                        break;
                    case 'E':
                        procE(data, client);
                        break;
                    case 'G':
                        procG(data, client);
                        break;
                    case 'S':
                        procS(data, client);
                        break;
                    case 'T':
                        procT(data, client);
                        break;
                }
            } else {
                LOG_PORT.println(F("-- binary message --"));
            }
            break;
        }
        case WS_EVT_CONNECT:
            LOG_PORT.print(F("* WS Connect - "));
            LOG_PORT.println(client->id());
            break;
        case WS_EVT_DISCONNECT:
            LOG_PORT.print(F("* WS Disconnect - "));
            LOG_PORT.println(client->id());
            break;
        case WS_EVT_PONG:
            LOG_PORT.println(F("* WS PONG *"));
            break;
        case WS_EVT_ERROR:
            LOG_PORT.println(F("** WS ERROR **"));
            break;
    }
}

#endif /* ESPIXELSTICK_H_ */
