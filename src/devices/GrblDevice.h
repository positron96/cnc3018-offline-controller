#pragma once

#include "GCodeDevice.h"


class GrblDevice : public GCodeDevice {
public:

    enum class Status {
        Idle, Run, Hold, Jog, Alarm, Door, Check, Home, Sleep
    };

    GrblDevice(WatchedSerial * s): 
        GCodeDevice(s)
    { 
        sentCounter = &sentQueue; 
        canTimeout = false;
    };
    GrblDevice() : GCodeDevice() { sentCounter = &sentQueue; }

    virtual ~GrblDevice() {}

    bool jog(uint8_t axis, float dist, int feed) override;

    bool canJog() override;

    void begin() override {
        GCodeDevice::begin();
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
        if(panic) return; // grbl does not respond in panic anyway
        char c = '?';
        schedulePriorityCommand(&c, 1);  
    }

    bool schedulePriorityCommand( const char* cmd, size_t len=0) override {
        if(txLocked) return false;
        if(len==0) len = strlen(cmd);
        if(isCmdRealtime(cmd, len) ) {
            printerSerial->write((const uint8_t*)cmd, len);  
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
    Status getStatus() { return status; }
    const char* getStatusStr();
    String & getLastResponse() { return lastResponse; }

    static void sendProbe(Stream &serial);
    static bool checkProbeResponse(const String s);

protected:
    void trySendCommand() override;

    void tryParseResponse( char* cmd, size_t len ) override;
    
private:
    
    SimpleCounter<15,128> sentQueue;
    
    String lastResponse;

    Status status;

    //WPos = MPos - WCO
    float ofsX,ofsY,ofsZ;
    uint32_t feed, spindleVal;

    void parseGrblStatus(char* v);

    bool setStatus(const char* s);

    static bool isCmdRealtime(const char* data, size_t len);

};
