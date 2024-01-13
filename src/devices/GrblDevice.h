#pragma once

#include "GCodeDevice.h"
#include <etl/vector.h>

class GrblDevice : public GCodeDevice {
public:
    const etl::vector<u_int16_t, sizeof(u_int16_t) * 5> SPINDLE_VALS{0, 1, 10, 100, 1000};

    enum class Status {
        Idle, Run, Hold, Jog, Alarm, Door, Check, Home, Sleep
    };

    GrblDevice(WatchedSerial *s, Job* job) :
            GCodeDevice(s,job) {
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
            LOGF("<RT (f%3d,%3d) '%c'\n", sentCounter->getFreeLines(), sentCounter->getFreeBytes(), cmd[0]);
            return true;
        } else {
            return GCodeDevice::schedulePriorityCommand(cmd, len);
        }
    }

    const etl::ivector<u_int16_t> &getSpindleValues() const override;

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
