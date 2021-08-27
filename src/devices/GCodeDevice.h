#pragma once

#include <Arduino.h>
#include <etl/observer.h>
//#include <etl/queue.h>
#include "CommandQueue.h"

#include "debug.h"

//#define ADD_LINECOMMENTS

#define GD_DEBUGF  LOGF
#define GD_DEBUGS  LOGLN
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

    GCodeDevice(Stream * s): 
        printerSerial(s), connected(false)
    {
        assert(inst==nullptr);
        inst = this;
    }
    GCodeDevice() : printerSerial(nullptr), connected(false) {}
    virtual ~GCodeDevice() { clear_observers(); }

    virtual void begin() { 
        while(printerSerial->available()>0) printerSerial->read(); 
        connected=true; 
    };


    virtual bool scheduleCommand(const char* cmd, size_t len=0) {
        if(panic) return false;
        if(len==0) len = strlen(cmd);
        if(len==0) return false;
        if(curUnsentCmdLen!=0) return false;
        memcpy(curUnsentCmd, cmd, len);
        curUnsentCmdLen = len;
        return true;
    };

    virtual bool schedulePriorityCommand( const char* cmd, size_t len=0) {
        //if(panic) return false;
        if(len==0) len = strlen(cmd);
        if(len==0) return false;
        if(curUnsentPriorityCmdLen!=0) return false;
        memcpy(curUnsentPriorityCmd, cmd, len);
        curUnsentPriorityCmdLen = len;
        return true;
    }
    virtual bool canSchedule(size_t len) { 
        if(panic) return false;
        if(len==0) return false;
        return curUnsentCmdLen!=0; 
    }

    virtual bool jog(uint8_t axis, float dist, int feed=100)=0;

    virtual bool canJog() { return true; }

    virtual void loop() {
        sendCommands();
        receiveResponses();
        checkTimeout();

        if(nextStatusRequestTime!=0 && int32_t(millis() - nextStatusRequestTime)>0 ) {
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

    String getDescrption() { return desc; }

    size_t getQueueLength() {  
        return 0; 
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
    bool canTimeout;

    static const size_t MAX_GCODE_LINE = 96;
    char curUnsentCmd[MAX_GCODE_LINE+1], curUnsentPriorityCmd[MAX_GCODE_LINE+1];
    size_t curUnsentCmdLen, curUnsentPriorityCmdLen;

    float x,y,z;
    bool panic = false;
    uint32_t nextStatusRequestTime;

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
        //sentCounter->clear();
        curUnsentCmdLen = 0;
        curUnsentPriorityCmdLen = 0;
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

    GrblDevice(Stream * s, int lockedPin): 
        GCodeDevice(s), txLocked(false), pinLocked(lockedPin)
    { 
        sentCounter = &sentQueue; 
        canTimeout = false;
        pinMode(lockedPin, INPUT);
        
    };
    GrblDevice() : GCodeDevice(), pinLocked(0) { sentCounter = &sentQueue; }

    virtual ~GrblDevice() {}

    bool jog(uint8_t axis, float dist, int feed) override;

    bool canJog() override;

    virtual void begin() {
        GCodeDevice::begin();
        readLockedStatus();
        schedulePriorityCommand("$I");
        requestStatusUpdate();
    }

    virtual void reset() {
        panic = false;
        cleanupQueue();
        char c = 0x18;
        schedulePriorityCommand(&c, 1);
    }

    virtual void loop() override {
        readLockedStatus();
        GCodeDevice::loop();
    }

    virtual void requestStatusUpdate() override {   
        char c = '?';
        schedulePriorityCommand(&c, 1);  
    }

    virtual bool schedulePriorityCommand( const char* cmd, size_t len=0) override {
        if(txLocked) return false;
        if(len==0) len = strlen(cmd);
        if(isCmdRealtime(cmd, len) ) {
            printerSerial->write(cmd, len);  
            GD_DEBUGF("<  (f%3d,%3d) '%c' RT\n", sentCounter->getFreeLines(), sentCounter->getFreeBytes(), cmd[0] );
            return true;
        }
        return GCodeDevice::schedulePriorityCommand(cmd, len);
    }

    /// WPos = MPos - WCO
    float getXOfs() { return ofsX; } 
    float getYOfs() { return ofsY; }
    float getZOfs() { return ofsZ; }
    uint32_t getSpindleVal() { return spindleVal; }
    uint32_t getFeed() { return feed; }
    String & getStatus() { return status; }
    String & getLastResponse() { return lastResponse; }

    bool isLocked() { return txLocked; }

    void sendProbe(Stream &serial);
    bool checkProbeResponse(const String s);

protected:
    void trySendCommand() override;

    void tryParseResponse( char* cmd, size_t len ) override;
    
private:
    
    SimpleCounter<15,128> sentQueue;

    const int pinLocked; ///< pin for reading woodpecker locked status
    
    String lastResponse;

    String status;

    bool txLocked; ///< woodpecker board has a pin that indicates if it's connected via USB. If it is, UART is occupied by USB_UART.

    //WPos = MPos - WCO
    float ofsX,ofsY,ofsZ;
    uint32_t feed, spindleVal;

    void parseGrblStatus(char* v);

    bool isCmdRealtime(const char* data, size_t len);

    void readLockedStatus() {
        bool t = digitalRead(pinLocked) == HIGH;
        if(t!=txLocked) notify_observers(DeviceStatusEvent{10});
        txLocked = t;
    }

};




String readStringUntil(Stream &PrinterSerial, char terminator, size_t timeout);
String readString(Stream &PrinterSerial, size_t timeout, size_t timeout2=100);

class DeviceDetector {
public:

    constexpr static int N_TYPES = 1;
    constexpr static int N_SERIAL_BAUDS = 3;
    static constexpr uint32_t serialBauds[] = { 115200, 250000, 57600 };

    static GCodeDevice* detectPrinter(HardwareSerial &PrinterSerial);

    static GCodeDevice* detectPrinterAttempt(HardwareSerial &PrinterSerial, uint32_t speed, uint8_t type);

    static uint32_t serialBaud;    

};


bool startsWith(const char *str, const char *pre);