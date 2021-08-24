#pragma once

#include <Arduino.h>
#include <etl/observer.h>
//#include <etl/queue.h>
#include "CommandQueue.h"
#include <message_buffer.h>

//#define ADD_LINECOMMENTS

#define GD_DEBUGF(...) // { Serial.printf(__VA_ARGS__); }
#define GD_DEBUGS(s)   // { Serial.println(s); }
#define GD_DEBUGLN GD_DEBUGS

#define KEEPALIVE_INTERVAL 5000    // Marlin defaults to 2 seconds, get a little of margin

#define STATUS_REQUEST_INTERVAL  500


const int MAX_DEVICE_OBSERVERS = 3;
struct DeviceStatusEvent { int statusField; };
using DeviceObserver = etl::observer<const DeviceStatusEvent&> ;

using ReceivedLineHandler = std::function< void(const char* str, size_t len) >;

class GCodeDevice : public etl::observable<DeviceObserver, MAX_DEVICE_OBSERVERS> {
public:

    static GCodeDevice *getDevice();
    //static void setDevice(GCodeDevice *dev);

    GCodeDevice(Stream * s, size_t priorityBufSize=0, size_t bufSize=0): printerSerial(s), connected(false)  {
        if(priorityBufSize!=0) buf0 = xMessageBufferCreate(priorityBufSize);
        if(bufSize!=0) buf1 = xMessageBufferCreate(bufSize);
        buf0Len = bufSize;
        buf1Len = priorityBufSize;

        assert(inst==nullptr);
        inst = this;
    }
    GCodeDevice() : printerSerial(nullptr), connected(false) {}
    virtual ~GCodeDevice() { clear_observers(); }

    virtual void begin() { 
        while(printerSerial->available()>0) printerSerial->read(); 
        connected=true; 
    };

    virtual bool scheduleCommand(String cmd) {
        return scheduleCommand(cmd.c_str(), cmd.length() );
    };
    virtual bool scheduleCommand(const char* cmd, size_t len) {
        if(panic) return false;
        if(!buf1) return false;
        if(len==0) return false;
        return xMessageBufferSend(buf1, cmd, len, 0) != 0;
    };
    virtual bool schedulePriorityCommand(String cmd) { 
        return schedulePriorityCommand(cmd.c_str(), cmd.length() );
    };
    virtual bool schedulePriorityCommand( const char* cmd, size_t len) {
        //if(panic) return false;
        if(!buf0) return false;
        if(len==0) return false;
        return xMessageBufferSend(buf0, cmd, len, 0) != 0;
    }
    virtual bool canSchedule(size_t len) { 
        if(panic) return false;
        if(!buf1) return false; 
        if(len==0) return false;
        return xMessageBufferSpaceAvailable(buf1) > len + 2; 
    }

    virtual bool jog(uint8_t axis, float dist, int feed=100)=0;

    virtual bool canJog() { return true; }

    virtual void loop() {
        sendCommands();
        receiveResponses();
        checkTimeout();

        if(nextStatusRequestTime!=0 && millis() > nextStatusRequestTime) {
            requestStatusUpdate();
            nextStatusRequestTime = millis() + STATUS_REQUEST_INTERVAL;
        }
    }
    virtual void sendCommands();
    virtual void receiveResponses();

    float getX() { return x; }
    float getY() { return y; }
    float getZ() { return z; }

    bool isConnected() { return connected; }

    virtual void reset()=0;

    bool isInPanic() { return panic; }

    virtual void enableStatusUpdates(bool v=true) {
        if(v) nextStatusRequestTime = millis();
        else nextStatusRequestTime = 0;
    }

    String getType() { return typeStr; }

    String getDescrption() { return desc; }

    size_t getQueueLength() {  
        return (  buf0Len - xMessageBufferSpaceAvailable(buf0)  ) + 
            (  buf1Len - xMessageBufferSpaceAvailable(buf1)  ); 
    }

    size_t getSentQueueLength()  {
        return sentCounter->bytes();
    }

    virtual void requestStatusUpdate() = 0;

    void addReceivedLineHandler( ReceivedLineHandler h) { receivedLineHandlers.push_back(h); }

protected:
    Stream * printerSerial;

    uint32_t serialRxTimeout;
    bool connected;
    String desc;
    String typeStr;
    size_t buf0Len, buf1Len;
    bool canTimeout;

    static const size_t MAX_GCODE_LINE = 96;
    char curUnsentCmd[MAX_GCODE_LINE+1], curUnsentPriorityCmd[MAX_GCODE_LINE+1];
    size_t curUnsentCmdLen, curUnsentPriorityCmdLen;

    float x,y,z;
    bool panic = false;
    uint32_t nextStatusRequestTime;
    MessageBufferHandle_t  buf0;
    MessageBufferHandle_t  buf1;

    bool xoff;
    bool xoffEnabled = false;

    Counter * sentCounter;

    void armRxTimeout() {
        if(!canTimeout) return;
        //GD_DEBUGLN(enable ? "GCodeDevice::resetRxTimeout enable" : "GCodeDevice::resetRxTimeout disable");
        serialRxTimeout = millis() + KEEPALIVE_INTERVAL;
    };
    void disarmRxTimeout() { 
        if(!canTimeout) return;  
        serialRxTimeout=0; 
    };
    void updateRxTimeout(bool waitingMore) {
        if(isRxTimeoutEnabled() ) { if(!waitingMore) disarmRxTimeout(); else armRxTimeout(); }
    }

    bool isRxTimeoutEnabled() { return canTimeout && serialRxTimeout!=0; }

    void checkTimeout() {
        if( !isRxTimeoutEnabled() ) return;
        if (millis() > serialRxTimeout) { 
            GD_DEBUGLN("GCodeDevice::checkTimeout fired"); 
            connected = false; 
            cleanupQueue();
            disarmRxTimeout(); 
            notify_observers(DeviceStatusEvent{1});
        }
    }

    void cleanupQueue() { 
        if(buf1) xMessageBufferReset(buf1); 
        if(buf0) xMessageBufferReset(buf0); 
        sentCounter->clear();
        curUnsentCmdLen = 0;
    }

    virtual void trySendCommand() = 0;

    virtual void tryParseResponse( char* cmd, size_t len ) = 0;

private:
    static GCodeDevice *inst;

    etl::vector<ReceivedLineHandler, 3> receivedLineHandlers;
    //friend void loop();

};



class GrblDevice : public GCodeDevice {
public:

    GrblDevice(Stream * s): GCodeDevice(s, 20, 100) { 
        typeStr = "grbl";
        sentCounter = &sentQueue; 
        canTimeout = false;
    };
    GrblDevice() : GCodeDevice() {typeStr = "grbl"; sentCounter = &sentQueue; }

    virtual ~GrblDevice() {}

    bool jog(uint8_t axis, float dist, int feed) override;

    bool canJog() override;

    virtual void begin() {
        GCodeDevice::begin();
        schedulePriorityCommand("$I");
        schedulePriorityCommand("?");
    }

    virtual void reset() {
        panic = false;
        cleanupQueue();
        char c = 0x18;
        schedulePriorityCommand(&c, 1);
    }

    virtual void requestStatusUpdate() override {        
        schedulePriorityCommand("?");
    }

    /// WPos = MPos - WCO
    float getXOfs() { return ofsX; } 
    float getYOfs() { return ofsY; }
    float getZOfs() { return ofsZ; }
    uint getSpindleVal() { return spindleVal; }
    uint getFeed() { return feed; }
    String & getStatus() { return status; }
    String & getLastResponse() { return lastResponse; }

protected:
    void trySendCommand() override;

    void tryParseResponse( char* cmd, size_t len ) override;
    
private:
    
    SimpleCounter<15,128> sentQueue;
    
    String lastResponse;

    String status;

    //WPos = MPos - WCO
    float ofsX,ofsY,ofsZ;
    uint feed, spindleVal;

    void parseGrblStatus(char* v);

    bool isCmdRealtime(char* data, size_t len);

};




class MarlinDevice: public GCodeDevice {

public:

    MarlinDevice(Stream * s): GCodeDevice(s, 100, 200) { 
        typeStr = "marlin";
        sentCounter = &sentQueue;
        canTimeout = true;
    }
    MarlinDevice() : GCodeDevice() {typeStr = "marlin";; sentCounter = &sentQueue;}

    virtual ~MarlinDevice() {}

    virtual bool jog(uint8_t axis, float dist, int feed) override {
        constexpr const char AXIS[] = {'X', 'Y', 'Z', 'E'};
        char msg[81]; snprintf(msg, 81, "G0 F%d %c%04f", feed, AXIS[axis], dist);
        if(  xMessageBufferSpacesAvailable(buf1)>strlen(msg)+3+3+6 ) return false;
        schedulePriorityCommand("G91");
        schedulePriorityCommand(msg);
        schedulePriorityCommand("G90");
        return true;
    }

    virtual void begin() {
        GCodeDevice::begin();
        if(! schedulePriorityCommand("M115") ) GD_DEBUGS("could not schedule M115");
        if(! schedulePriorityCommand("M114") ) GD_DEBUGS("could not schedule M114");
        if(! schedulePriorityCommand("M105") ) GD_DEBUGS("could not schedule M105");
    }

    virtual void reset() {        
        cleanupQueue();
        panic = false;
        schedulePriorityCommand("M112");
        //schedulePriorityCommand("M999");
    }

    //virtual void receiveResponses() ;

    void requestStatusUpdate() override {
        schedulePriorityCommand("M114"); // temp
        schedulePriorityCommand("M105"); // pos
    }

    struct Temperature {
        float actual;
        float target;
    };

    const Temperature & getBedTemp() const { return bedTemperature; }
    const Temperature & getExtruderTemp(uint8_t e) const { return toolTemperatures[e]; }
    uint8_t getExtruderCount() const { return fwExtruders; }

protected:

    void trySendCommand() override;

    void tryParseResponse( char* cmd, size_t len ) override;

private:

    static const int MAX_SUPPORTED_EXTRUDERS = 3;

    static const size_t MAX_SENT_BYTES = 128;
    static const size_t MAX_SENT_LINES = 400;

    SizedQueue<MAX_SENT_LINES, MAX_SENT_BYTES, MAX_GCODE_LINE> sentQueue;

    int fwExtruders = 1;
    bool fwAutoreportTempCap, fwProgressCap, fwBuildPercentCap;
    bool autoreportTempEnabled;

    Temperature toolTemperatures[MAX_SUPPORTED_EXTRUDERS];
    Temperature bedTemperature;
    String lastResponse;
    float ePos; ///< extruder pos

    bool parseTemperatures(const String &response);

    // Parse temperatures from printer responses like
    // ok T:32.8 /0.0 B:31.8 /0.0 T0:32.8 /0.0 @:0 B@:0
    bool parseTemp(const String &response, const String whichTemp, Temperature *temperature);
    
    // Parse position responses from printer like
    // X:-33.00 Y:-10.00 Z:5.00 E:37.95 Count X:-3300 Y:-1000 Z:2000
    bool parsePosition(const char *str);

    bool parseM115(const String &str);
    bool parseG0G1(const char * str);


    static float extractFloat(const String &str, const String key) ;
    static float extractFloat(const char * str, const char* key) ;
    
    static bool isFloat(const String value);

    // Parse temperatures from prusa firmare (sent when heating)
    // ok T:32.8 E:0 B:31.8
    static bool extractPrusaHeatingTemp(const String &response, const String whichTemp, float &temperature) ;

    static int extractPrusaHeatingExtruder(const String &response) ;

    static String extractM115String(const String &response, const String field) ;

    static bool extractM115Bool(const String &response, const String field, const bool onErrorValue = false);

};

String readStringUntil(Stream &PrinterSerial, char terminator, size_t timeout);
String readString(Stream &PrinterSerial, size_t timeout, size_t timeout2=100);

class DeviceDetector {
public:

    constexpr static int N_TYPES = 2;

    constexpr static int N_SERIAL_BAUDS = 3;

    static const uint32_t serialBauds[];   // Marlin valid bauds (removed very low bauds; roughly ordered by popularity to speed things up)

    static GCodeDevice* detectPrinter(HardwareSerial &PrinterSerial);

    static GCodeDevice* detectPrinterAttempt(HardwareSerial &PrinterSerial, uint32_t speed, uint8_t type);

    static uint32_t serialBaud;    

private:
    static void sendProbe(uint8_t i, Stream &serial);

    static GCodeDevice* checkProbe(uint8_t i, String v, Stream &serial) ;

};


bool startsWith(const char *str, const char *pre);