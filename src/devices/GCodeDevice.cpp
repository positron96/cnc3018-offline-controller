#include "GCodeDevice.h"

#define XOFF  0x13
#define XON   0x11

#define MAX(a,b)  ( (a)>(b) ? (a) : (b) )
static char deviceBuffer[MAX(sizeof(MarlinDevice), sizeof(GrblDevice))];

const uint32_t DeviceDetector::serialBauds[] = { 115200, 250000, 57600 }; 

uint32_t DeviceDetector::serialBaud = 0;

void DeviceDetector::sendProbe(uint8_t i, Stream &serial) {
    switch(i) {
        case 0: 
            serial.print("\n$I\n");
            break;
        case 1:
            serial.print("\nM115\n");
            break;
    }
}

GCodeDevice* DeviceDetector::checkProbe(uint8_t i, String v, Stream &serial) {
    if(i==0) {
        if(v.indexOf("[VER:")!=-1 ) {
            GD_DEBUGS("Detected GRBL device");
            //devices.grbl = GrblDevice(&serial);
            return new (deviceBuffer) GrblDevice(&serial);
            //return (GCodeDevice*)deviceBuffer;
        }
    }
    if(i==1) {
        if(v.indexOf("MACHINE_TYPE") != -1) {
            GD_DEBUGS("Detected Marlin device");
            //devices.marlin = MarlinDevice(&serial);
            return new (deviceBuffer) MarlinDevice(&serial);
            //return (GCodeDevice*)deviceBuffer;
        }
    }

    return nullptr;

    //return false;
}

GCodeDevice* DeviceDetector::detectPrinterAttempt(HardwareSerial &printerSerial, uint32_t speed, uint8_t type) {
    serialBaud = speed;
    for(uint8_t retry=0; retry<2; retry++) {
        GD_DEBUGF("attempt %d, speed %d, type %d\n", retry, speed, type);
        //PrinterSerial.end();
        //PrinterSerial.begin(speed);
        printerSerial.updateBaudRate(speed);
        while(printerSerial.available()) printerSerial.read();
        DeviceDetector::sendProbe(type, printerSerial);
        //String v = readStringUntil(printerSerial, '\n', 1000); v.trim();
        String v = readString(printerSerial, 1000);
        GD_DEBUGF("Got response '%s'\n", v.c_str() );
        if(v) {
            //int t = v.indexOf('\n');
            GCodeDevice * dev = DeviceDetector::checkProbe(type, v, printerSerial);
            if(dev!=nullptr) return dev;
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

    if(curUnsentPriorityCmdLen == 0) {
        #ifdef ADD_LINECOMMENTS
            char tmp[MAX_GCODE_LINE+1];
            curUnsentPriorityCmdLen = xMessageBufferReceive(buf0, tmp, MAX_GCODE_LINE, 0);
            if(curUnsentPriorityCmdLen!=0) {
                tmp[curUnsentPriorityCmdLen] = 0;
                snprintf(curUnsentPriorityCmd, MAX_GCODE_LINE, "%s ;%d", tmp, nline++);
                curUnsentPriorityCmdLen = strlen(curUnsentPriorityCmd);
            }
        #else
            curUnsentPriorityCmdLen = xMessageBufferReceive(buf0, curUnsentPriorityCmd, MAX_GCODE_LINE, 0);
            curUnsentPriorityCmd[curUnsentPriorityCmdLen]=0;
        #endif
    }

    if(curUnsentPriorityCmdLen==0 && curUnsentCmdLen==0) {
        #ifdef ADD_LINECOMMENTS
            char tmp[MAX_GCODE_LINE+1];
            curUnsentCmdLen = xMessageBufferReceive(buf1, tmp, MAX_GCODE_LINE, 0);
            if(curUnsentCmdLen!=0) {
                tmp[curUnsentCmdLen] = 0;
                snprintf(curUnsentCmd, MAX_GCODE_LINE, "%s ;%d", tmp, nline++);
                curUnsentCmdLen = strlen(curUnsentCmd);
            }
        #else
            curUnsentCmdLen = xMessageBufferReceive(buf1, curUnsentCmd, MAX_GCODE_LINE, 0);
            curUnsentCmd[curUnsentCmdLen] = 0; 
        #endif
        //loadedNewCmd = true;
    }

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





#define TEMP_COMMAND      "M105"
#define AUTOTEMP_COMMAND  "M155 S"



bool startsWith(const char *str, const char *pre) {
    return strncmp(pre, str, strlen(pre)) == 0;
}

void MarlinDevice::trySendCommand() {
    char* cmd  = curUnsentPriorityCmdLen!=0 ? &curUnsentPriorityCmd[0] :  &curUnsentCmd[0]; 
    size_t * len = curUnsentPriorityCmdLen!=0 ? &curUnsentPriorityCmdLen : &curUnsentCmdLen ;

    if( sentCounter->canPush(*len) ) {
        sentCounter->push( cmd, *len );
        printerSerial->write(cmd, *len);  
        printerSerial->print('\n');
        armRxTimeout();
        GD_DEBUGF("<  (f%3d,%3d) '%s' (%d)\n", sentCounter->getFreeLines(), sentCounter->getFreeBytes(), cmd, *len );
        *len = 0;
    } else {
        //if(loadedNewCmd) GD_DEBUGF("<  Not sent, free lines: %d, free space: %d\n", sentQueue.getFreeLines() , sentQueue.getFreeBytes()  );
    }
}

void MarlinDevice::tryParseResponse( char* resp, size_t len ) {

    char tmp = 0;
    char* curCmd;
    size_t curCmdLen = sentQueue.peek(curCmd);
    if(curCmdLen==0) curCmd = &tmp;

    //GD_DEBUGF(" > '%s'; current cmd %s\n", resp, curCmd );

    if ( startsWith(resp, "ok") ) {

        if (startsWith(curCmd, TEMP_COMMAND))
            parseTemperatures(String(resp) );
        else if (fwAutoreportTempCap && startsWith(curCmd, AUTOTEMP_COMMAND))
            autoreportTempEnabled = (curCmd[6] != '0');
        else if(startsWith(curCmd, "G0") || startsWith(curCmd, "G1")) {
            parseG0G1(curCmd); // artificial position from G0/G1 command
        }
        
        //sentQueue.markAcknowledged();     // Go on with next command
        sentQueue.pop();
        
        //curCmdLen = 0; // need to fetch another sent command from queue

        connected = true;
    } else {
        if (connected) {
            if(startsWith(curCmd,"M115") ) {
                parseM115(String(resp) ); 
            } else if (parseTemperatures(String(resp) ) ) {
                // do nothing
                //sprintf(responseDetail, "autotemp");
            } else if (parsePosition(resp) ) {
                // do nothing
                //sprintf(responseDetail, "position");
            /*} else if (startsWith(resp, "Resend: ")) {
                nline = (int)extractFloat(resp, "Resend:");
                sprintf(responseDetail, "sync line=%d", nline);
            }*/
            } else if (startsWith(resp, "echo: cold extrusion prevented")) {
                // To do: Pause sending gcode, or do something similar
                lastResponse = "cold extrusion prevented";
                notify_observers(DeviceStatusEvent{1}); 
            }
            else if (startsWith(resp, "Error:") ) {
                lastResponse = resp;

                cleanupQueue();
                panic = true;

                
                notify_observers(DeviceStatusEvent{1}); 
            } else {
                //incompleteResponse = true;
            }
        } else {
            //incompleteResponse = true;
            lastResponse = "discovering";
        }
    }

    GD_DEBUGF(" > (f%3d,%3d) '%s' current cmd '%s'\n", sentQueue.getFreeLines(), sentQueue.getFreeBytes(), 
        resp, curCmd );
    //GD_DEBUGF("   free slots: %d type:'%s' \n", sentQueue.getFreeLines(),  responseDetail  );
    updateRxTimeout( sentQueue.size()>0 );
        

};



// Parse temperatures from printer responses like
// ok T:32.8 /0.0 B:31.8 /0.0 T0:32.8 /0.0 @:0 B@:0
bool MarlinDevice::parseTemperatures(const String &response) {
    bool ret;

    if (fwExtruders == 1)
        ret = parseTemp(response, "T", &toolTemperatures[0]);
    else {
        ret = false;
        for (int t = 0; t < fwExtruders; t++)
            ret |= parseTemp(response, "T" + String(t), &toolTemperatures[t]);
    }
    ret |= parseTemp(response, "B", &bedTemperature);
    if (!ret) {
        // Parse Prusa heating temperatures
        int e = extractPrusaHeatingExtruder(response);
        ret = e >= 0 && e < MAX_SUPPORTED_EXTRUDERS && extractPrusaHeatingTemp(response, "T", toolTemperatures[e].actual);
        ret |= extractPrusaHeatingTemp(response, "B", bedTemperature.actual);
    }

    if(ret) GD_DEBUGF("Parsed temp E:%d->%d  B:%d->%d\n", 
        (int)toolTemperatures[0].actual, (int)toolTemperatures[0].target,  
        (int)bedTemperature.actual, (int)bedTemperature.target );

    notify_observers(DeviceStatusEvent{0});

    return ret;
}

// parses line T:aaa.aa /ttt.tt
bool MarlinDevice::parseTemp(const String &response, const String whichTemp, Temperature *temperature) {
    int tpos = response.indexOf(whichTemp + ":");
    if (tpos != -1) { // This response contains a temperature
        int slashpos = response.indexOf(" /", tpos);
        int spacepos = response.indexOf(" ", slashpos + 1);
        // if match mask T:xxx.xx /xxx.xx
        if (slashpos != -1 && spacepos != -1) {
            String actual = response.substring(tpos + whichTemp.length() + 1, slashpos);
            String target = response.substring(slashpos + 2, spacepos);
            if (isFloat(actual) && isFloat(target)) {
                temperature->actual = actual.toFloat();
                temperature->target = target.toFloat();

                return true;
            }
        }
    }

    return false;
}

// Parse position responses from printer like
// X:-33.00 Y:-10.00 Z:5.00 E:37.95 Count X:-3300 Y:-1000 Z:2000
bool MarlinDevice::parsePosition(const char * str) {
    float t = extractFloat(str, "X:");
    if(!isnan(t) ) x = t; else return false;
    t = extractFloat(str, "Y:");
    if(!isnan(t) ) y = t; else return false;
    t = extractFloat(str, "Z:");
    if(!isnan(t) ) z = t; else return false;
    t = extractFloat(str, "E:");
    if(!isnan(t) ) ePos = t; else return false;
    GD_DEBUGF("Parsed pos: X: %f, Y: %f, Z: %f, E: %f\n", x,y,z,ePos);
    notify_observers(DeviceStatusEvent{0});
    return true;
}

// Parse position responses from printer like
// X:-33.00 Y:-10.00 Z:5.00 E:37.95 Count X:-3300 Y:-1000 Z:2000
bool MarlinDevice::parseG0G1(const char *str) {
    float t = extractFloat(str, "X");
    if(!isnan(t) ) x = t; else return false;
    t = extractFloat(str, "Y");
    if(!isnan(t) ) y = t; else return false;
    t = extractFloat(str, "Z");
    if(!isnan(t) ) z = t; else return false;
    t = extractFloat(str, "E");
    if(!isnan(t) ) ePos = t; else return false;
    GD_DEBUGF("Parsed pos: X: %f, Y: %f, Z: %f, E: %f\n", x,y,z,ePos);
    notify_observers(DeviceStatusEvent{0});
    return true;
}

bool MarlinDevice::parseM115(const String &str) {
    desc = extractM115String(str, "FIRMWARE_NAME") + " " + extractM115String(str, "MACHINE_TYPE");
    String value = extractM115String(str, "EXTRUDER_COUNT");
    fwExtruders = value == "" ? 1 : min(value.toInt(), (long)MAX_SUPPORTED_EXTRUDERS);
    fwAutoreportTempCap = extractM115Bool(str, "Cap:AUTOREPORT_TEMP");
    fwProgressCap = extractM115Bool(str, "Cap:PROGRESS");
    fwBuildPercentCap = extractM115Bool(str, "Cap:BUILD_PERCENT");
    GD_DEBUGF("Parsed M115: desc=%s, extruders:%d, autotemp:%d, progress:%d, buildPercent:%d\n", 
        desc.c_str(), fwExtruders, fwAutoreportTempCap, fwProgressCap, fwBuildPercentCap );
    notify_observers(DeviceStatusEvent{0});
    return true;
}


bool MarlinDevice::isFloat(const String value) {
    for (int i = 0; i < value.length(); i++) {
        char ch = value[i];
        if (ch != ' ' && ch != '.' && ch != '-' && !isDigit(ch)) return false;
    }
    return true;
}

inline float MarlinDevice::extractFloat(const char *str, const char * key) {
    char* s = strstr(str, key);
    if(s==NULL) return NAN; 
    s += strlen(key);
    return atof(s);
}

inline float MarlinDevice::extractFloat(const String &str, const String key) {
    int s = str.indexOf(key);
    if(s==-1) return NAN; 
    s += key.length();
    int e = str.indexOf(' ', s);
    if(e==-1) e=str.length();
    return str.substring(s,e).toFloat();
}

// Parse temperatures from prusa firmare (sent when heating)
// ok T:32.8 E:0 B:31.8
bool MarlinDevice::extractPrusaHeatingTemp(const String &response, const String whichTemp, float &temperature) {
    int tpos = response.indexOf(whichTemp + ":");
    if (tpos != -1) { // This response contains a temperature
        int spacepos = response.indexOf(" ", tpos);
        if (spacepos == -1)
        spacepos = response.length();
        String actual = response.substring(tpos + whichTemp.length() + 1, spacepos);
        if (isFloat(actual)) {
            temperature = actual.toFloat();
            return true;
        }
    }

    return false;
}


inline int MarlinDevice::extractPrusaHeatingExtruder(const String &response) {
    float tmp;
    return extractPrusaHeatingTemp(response, "E", tmp) ? (int)tmp : -1;
}

String MarlinDevice::extractM115String(const String &response, const String key) {
    int spos = response.indexOf(key + ":");
    if (spos != -1) {
        spos += key.length() + 1;
        int epos = response.indexOf(':', spos);
        if (epos == -1) return response.substring(spos);
        else {
            while (epos >= spos && response[epos] != ' ' && response[epos] != '\n') --epos;
            return response.substring(spos, epos);
        }
    }

    return "";
}

inline bool MarlinDevice::extractM115Bool(const String &response, const String key, const bool onErrorValue) {
    String result = extractM115String(response, key);
    return result == "" ? onErrorValue : (result == "1" ? true : false);
}
