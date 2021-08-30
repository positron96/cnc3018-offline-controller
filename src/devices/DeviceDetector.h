#pragma once

#include "GCodeDevice.h"
#include <HardwareSerial.h>
//#include <etl/delegate.h>
#include <functional>

struct DeviceDescription {
    std::function< void(Stream &) > sendProbe;
    std::function< bool(const String &) > checkProbeResponse;
    std::function< void(const int type, Stream &s, size_t baud) > createDevice;
};

template<HardwareSerial &printerSerial, DeviceDescription &... DEVs>
class DeviceDetector {
public:

    constexpr static DeviceDescription desc[] = { DEVs... };
    constexpr static size_t N_TYPES = sizeof...(DEVs);

    constexpr static int N_SERIAL_BAUDS = 3;
    constexpr static uint32_t serialBauds[] = { 115200, 250000, 57600 };

    constexpr static uint8_t N_ATTEMPTS=2;

    static void begin() {
        nextProbeTime = 0;
        cResult = -1;
        cAttempt = 0;
        cType = 0;
        cSpeed = 0;
    }

    static void loop() {
        if(cResult!=-1) return;

        if( int32_t(millis()-nextProbeTime) > 0) {
            sendNextProbe();
            nextProbeTime = millis() + 1000;
        } else {
            collectResponse();
        }
    }

    static int getDetectResult();

    static uint32_t serialBaud; 

private:

    static uint8_t cSpeed;
    static uint8_t cType;
    static uint8_t cAttempt;
    static int cResult;
    static uint32_t nextProbeTime;

    void sendNextProbe() {
        serialBaud = serialBauds[cSpeed];
        while(printerSerial.available()) printerSerial.read();
        printerSerial.end();
        printerSerial.begin(serialBaud);
        desc[cType].sendProbe(printerSerial);

        cAttempt++;
        if(cAttempt==N_ATTEMPTS) {
            cAttempt = 0;
            cSpeed++;
            if(cSpeed==N_SERIAL_BAUDS) {
                cSpeed = 0;
                cType++;
                if(cType==N_TYPES) {
                    cType = 0;
                }
            }
        }
    }

    void collectResponse() {
        static const size_t MAX_LINE = 200; // M115 is far longer than 100
        static char resp[MAX_LINE+1];
        static size_t respLen;

        while(printerSerial.available())  {
            char ch = (char)printerSerial.read();
            switch(ch) {
                case '\n':
                case '\r': break;
                default: if(respLen<MAX_LINE) resp[respLen++] = ch;
            }
            if(ch=='\n') {
                resp[respLen]=0;
                bool ret = desc[cType].checkProbeResponse(resp);
                if(ret) {
                    desc[cType].createDevice(cType);
                    cResult = cType;
                    return;
                }                   
                respLen = 0;
            }
        }
    }

};

template<HardwareSerial &printerSerial, DeviceDescription &... DEVs>
constexpr DeviceDescription DeviceDetector<printerSerial, DEVs...>::desc[];

