#pragma once
#include "GCodeDevice.h"
#include "GrblDevice.h"
#include <HardwareSerial.h>

#include "debug.h"
#include "gcode/gcode.h"

using GrbDevice_ptr = GCodeDevice *;

template<class T, T &printerSerial, GrbDevice_ptr(*createDevice)(T *)>
class GrblDetector {
public:

    constexpr static int N_SERIAL_BAUDS = 4;
    constexpr static int N_DEVICES = 2;
    constexpr static uint32_t serialBauds[] = {115200, 250000, 57600, 9600};
    constexpr static const char *deviceNames[] = {"grbl", "marlin"};
    constexpr static uint8_t N_ATTEMPTS = 3;

    static void init() {
        nextProbeTime = 0;
        cResult = 0;
        cAttempt = 0;
        cSpeed = 0;
        cDev = 0;
    }

    static void loop() {
        if (cResult != 0) return;

        if (int32_t(millis() - nextProbeTime) > 0) {
            sendNextProbe();
            nextProbeTime = millis() + 1000;
        } else {
            collectResponse();
        }
    }

    static int getDetectResult() { return cResult; }

    static uint32_t serialBaud;
    static const char *deviceName;

private:

    static uint8_t cSpeed;
    static uint8_t cDev;
    static uint8_t cAttempt;
    static int cResult;
    static uint32_t nextProbeTime;

    static void sendNextProbe() {
        serialBaud = serialBauds[cSpeed];
        deviceName = deviceNames[cDev];

        while (printerSerial.available()) {
            printerSerial.read();
        }
        printerSerial.end();
        printerSerial.begin(serialBaud);

        LOGF("Dev send probe on %d >>..", serialBaud);
        // one of

        GrblDevice::sendProbe(printerSerial);
        // or :
        printerSerial.print("\n");
        printerSerial.print(GET_CURRENT_POS);
        printerSerial.print("\n");

        cAttempt++;
        LOGF("dev:%s speed: %s attempt:%d \n", deviceNames[cDev], serialBauds[cSpeed], cAttempt);
        if (cAttempt == N_ATTEMPTS) {
            cAttempt = 0;
            cSpeed++;
            if(cSpeed==N_SERIAL_BAUDS) {
                cSpeed = 0;
                // ring
                cDev++;
                cDev %= N_DEVICES;
            }
        }
    }

    static void collectResponse() {
        static const size_t MAX_LINE = 200; // M115 is far longer than 100
        static char resp[MAX_LINE + 1];
        static size_t respLen;

        while (printerSerial.available()) {
            char ch = (char) printerSerial.read();
            LOGF("> Read [%s]\n", ch);
            switch (ch) {
                case '\n':
                case '\r':
                    break;
                default:
                    if (respLen < MAX_LINE) resp[respLen++] = ch;
            }
            if (ch == '\n') {
                resp[respLen] = 0;
                LOGF("> Got %s", resp);
                bool ret;
                if (cDev == 0) {
                    ret = GrblDevice::checkProbeResponse(resp);
                } else if (cDev == 1){
                    //todo
                }
                if (ret) {
                    LOGF("> Detected  %s", resp);
                     createDevice(&printerSerial);
                     cResult = 1;
                    return;
                }
                respLen = 0;
            }
        }
    }

};

// friend funcs for Detector Screen
template<class T, T &printerSerial, GrbDevice_ptr(*createDevice)(T *)>
uint32_t GrblDetector<T, printerSerial, createDevice>::serialBaud;

template<class T, T &printerSerial, GrbDevice_ptr(*createDevice)(T *)>
const char *GrblDetector<T, printerSerial, createDevice>::deviceName;

template<class T, T &printerSerial, GrbDevice_ptr(*createDevice)(T *)>
uint8_t GrblDetector<T, printerSerial, createDevice>::cSpeed;

template<class T, T &printerSerial, GrbDevice_ptr(*createDevice)(T *)>
uint8_t  GrblDetector<T, printerSerial, createDevice>::cAttempt;

template<class T, T &printerSerial, GrbDevice_ptr(*createDevice)(T *)>
int  GrblDetector<T, printerSerial, createDevice>::cResult;

template<class T, T &printerSerial, GrbDevice_ptr(*createDevice)(T *)>
uint8_t  GrblDetector<T, printerSerial, createDevice>::cDev;

template<class T, T &printerSerial, GrbDevice_ptr(*createDevice)(T *)>
uint32_t  GrblDetector<T, printerSerial, createDevice>::nextProbeTime;

template<class T, T &printerSerial, GrbDevice_ptr(*createDevice)(T *)>
constexpr uint32_t GrblDetector<T, printerSerial, createDevice>::serialBauds[];

template<class T, T &printerSerial, GrbDevice_ptr(*createDevice)(T *)>
constexpr const char* GrblDetector<T, printerSerial, createDevice>::deviceNames[];
