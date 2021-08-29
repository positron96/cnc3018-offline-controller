#pragma once

#include "GCodeDevice.h"


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

    void begin() override {
        GCodeDevice::begin();
        readLockedStatus();
        schedulePriorityCommand("$I");
        requestStatusUpdate();
    }

    void reset() override {
        panic = false;
        cleanupQueue();
        char c = 0x18;
        schedulePriorityCommand(&c, 1);
    }

    void loop() override {
        readLockedStatus();
        GCodeDevice::loop();
    }

    void requestStatusUpdate() override {   
        char c = '?';
        schedulePriorityCommand(&c, 1);  
    }

    bool schedulePriorityCommand( const char* cmd, size_t len=0) override {
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

    static void sendProbe(Stream &serial);
    static bool checkProbeResponse(const String s);

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
