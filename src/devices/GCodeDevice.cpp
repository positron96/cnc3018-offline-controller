#include "GCodeDevice.h"


#define XOFF  0x13
#define XON   0x11

#define MAX(a,b)  ( (a)>(b) ? (a) : (b) )
static char deviceBuffer[sizeof(GrblDevice)];

const uint32_t DeviceDetector::serialBauds[] = { 115200, 250000, 57600 }; 

uint32_t DeviceDetector::serialBaud = 0;

GCodeDevice* DeviceDetector::detectPrinterAttempt(HardwareSerial &printerSerial, uint32_t speed, uint8_t type) {
    serialBaud = speed;
    for(uint8_t retry=0; retry<2; retry++) {
        GD_DEBUGF("attempt %d, speed %d, type %d\n", retry, speed, type);
        
        printerSerial.end();
        printerSerial.begin(speed);
        while(printerSerial.available()) printerSerial.read();
        GrblDevice::sendProbe(printerSerial);
        //String v = readStringUntil(printerSerial, '\n', 1000); v.trim();
        String v = readString(printerSerial, 1000);
        GD_DEBUGF("Got response '%s'\n", v.c_str() );
        if(v) {
            bool ret = GrblDevice::checkProbe(v);
            if(ret) return type;
        }
    }
    return nullptr;
}


GCodeDevice* DeviceDetector::detectPrinter(HardwareSerial &printerSerial) {
    while(true) {
        for(uint32_t speed: serialBauds) {
            for(int type=0; type<DeviceDetector::N_TYPES; type++) {
                GCodeDevice *dev = detectPrinterAttempt(printerSerial, speed, type);
                if(dev!=nullptr) return dev;
            }
        }
    }    
    
}

String readStringUntil(Stream &serial, char terminator, size_t timeout) {
    String ret;
    timeout += millis();
    char c;
    int len = serial.readBytes(&c, 1);
    while(len>0 && c != terminator && millis()<timeout) {
        ret += (char) c;
        len = serial.readBytes(&c, 1);
    }
    return ret;
}

String readString(Stream &serial, size_t timeout, size_t earlyTimeout) {
    String ret; ret.reserve(40);
    timeout += millis();
    earlyTimeout += millis();
    while(millis()<timeout) {
        if(serial.available()>0) {
            ret += (char)serial.read();
        }
        if(millis()>earlyTimeout && ret.length()>0 ) break; // break early if something was read at all
    }
    return ret;
}

bool startsWith(const char *str, const char *pre) {
    return strncmp(pre, str, strlen(pre)) == 0;
}





GCodeDevice *GCodeDevice::inst = nullptr;

GCodeDevice *GCodeDevice::getDevice() {
    return inst;
}
/*
void GCodeDevice::setDevice(GCodeDevice *dev) {
    device = dev;
}
*/


void GCodeDevice::sendCommands() {

    if(panic) return;
    //bool loadedNewCmd=false;

    if(xoffEnabled && xoff) return;

    #ifdef ADD_LINECOMMENTS
    static size_t nline=0;
    #endif

    // if(curUnsentPriorityCmdLen == 0) {
    //     #ifdef ADD_LINECOMMENTS
    //         char tmp[MAX_GCODE_LINE+1];
    //         curUnsentPriorityCmdLen = xMessageBufferReceive(buf0, tmp, MAX_GCODE_LINE, 0);
    //         if(curUnsentPriorityCmdLen!=0) {
    //             tmp[curUnsentPriorityCmdLen] = 0;
    //             snprintf(curUnsentPriorityCmd, MAX_GCODE_LINE, "%s ;%d", tmp, nline++);
    //             curUnsentPriorityCmdLen = strlen(curUnsentPriorityCmd);
    //         }
    //     #else
    //         curUnsentPriorityCmdLen = xMessageBufferReceive(buf0, curUnsentPriorityCmd, MAX_GCODE_LINE, 0);
    //         curUnsentPriorityCmd[curUnsentPriorityCmdLen]=0;
    //     #endif
    // }

    // if(curUnsentPriorityCmdLen==0 && curUnsentCmdLen==0) {
    //     #ifdef ADD_LINECOMMENTS
    //         char tmp[MAX_GCODE_LINE+1];
    //         curUnsentCmdLen = xMessageBufferReceive(buf1, tmp, MAX_GCODE_LINE, 0);
    //         if(curUnsentCmdLen!=0) {
    //             tmp[curUnsentCmdLen] = 0;
    //             snprintf(curUnsentCmd, MAX_GCODE_LINE, "%s ;%d", tmp, nline++);
    //             curUnsentCmdLen = strlen(curUnsentCmd);
    //         }
    //     #else
    //         curUnsentCmdLen = xMessageBufferReceive(buf1, curUnsentCmd, MAX_GCODE_LINE, 0);
    //         curUnsentCmd[curUnsentCmdLen] = 0; 
    //     #endif
    //     //loadedNewCmd = true;
    // }

    if(curUnsentCmdLen==0 && curUnsentPriorityCmdLen==0) return;

    trySendCommand();

}


void GCodeDevice::receiveResponses() {


    static const size_t MAX_LINE = 200; // M115 is far longer than 100
    static char resp[MAX_LINE+1];
    static size_t respLen;

    while (printerSerial->available()) {
        char ch = (char)printerSerial->read();
        switch(ch) {
            case '\n':
            case '\r': break;
            case XOFF: if(xoffEnabled) { xoff=true; break; }
            case XON: if(xoffEnabled) {xoff=false; break; }
            default: if(respLen<MAX_LINE) resp[respLen++] = ch;
        }
        if(ch=='\n') {
            resp[respLen]=0;
            for(const auto &r: receivedLineHandlers) if(r) r(resp, respLen);
            tryParseResponse(resp, respLen);
            respLen = 0;
        }
    }
    
}




