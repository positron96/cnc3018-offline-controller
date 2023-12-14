#pragma once

#include "GCodeDevice.h"


class GrblDevice : public GCodeDevice {
public:

    enum class Status {
        Idle, Run, Hold, Jog, Alarm, Door, Check, Home, Sleep
    };

    GrblDevice(WatchedSerial *s) :
            GCodeDevice(s) {
        sentCounter = &sentQueue;
        canTimeout = false;
    };

    GrblDevice() : GCodeDevice() { sentCounter = &sentQueue; }

    virtual ~GrblDevice() {}

    bool jog(uint8_t axis, float dist, int feed) override;

    bool canJog() override;

    void begin() override {
        GCodeDevice::begin();
        schedulePriorityCommand("$I"); // TODO actual depends on realisation see inside fn
        requestStatusUpdate(); //TODO how this all begin works in real GRBL ???
        // TODO why it has 2 tracts 1: send with serial 2: schedule Priority ???
    }

    void reset() override {
        panic = false;
        cleanupQueue();
        // ^x, reset
        schedulePriorityCommand("\x18", 1);
    }


    void requestStatusUpdate() override {
        if (panic)
            return; // grbl does not respond in panic anyway
        schedulePriorityCommand("?", 1);
    }

    bool schedulePriorityCommand(const char *cmd, size_t len = 0) override {
        if (txLocked) return false;
        if (len == 0) {
            len = strlen(cmd);
        }
        if (isCmdRealtime(cmd, len)) {
            printerSerial->write((const uint8_t *) cmd, len);
//            LOGF("<  (f%3d,%3d) '%c' RT\n", sentCounter->getFreeLines(), sentCounter->getFreeBytes(), cmd[0]);
            return true;
        } else {
            return GCodeDevice::schedulePriorityCommand(cmd, len);
        }
    }

    /// WPos = MPos - WCO
    float getXOfs() const { return ofsX; }

    float getYOfs() const { return ofsY; }

    float getZOfs() const { return ofsZ; }

    const char *getStatusStr() const override;

    static void sendProbe(Stream &serial);

    static bool checkProbeResponse(String s);

protected:
    void trySendCommand() override;

    void tryParseResponse(char *cmd, size_t len) override;

private:

    SimpleCounter<15, 128> sentQueue;

    Status status;

    //WPos = MPos - WCO
    float ofsX, ofsY, ofsZ;

    void parseStatus(char *v);

    bool setStatus(const char *s);

    static bool isCmdRealtime(const char *data, size_t len);

};
