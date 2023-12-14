#pragma once

#include "GCodeDevice.h"
#include "gcode/gcode.h"


class MarlinDevice : public GCodeDevice {
public:

    enum class Status {

    };

    explicit MarlinDevice(WatchedSerial *s) :
            GCodeDevice(s) {}

    virtual ~MarlinDevice() {}

    bool jog(uint8_t axis, float dist, int feed) override;

    bool canJog() override;

    void begin() override {
        GCodeDevice::begin();
        requestStatusUpdate();
    }

    void reset() override {
        panic = false;
        cleanupQueue();

        // TODO Marlin has panic at all ??
//        char c = 0x18; // ^x, reset
//        schedulePriorityCommand(&c, 1);
    }


    void requestStatusUpdate() override {
        if (panic) return; // todo
        schedulePriorityCommand(M114_GET_CURRENT_POS);
    }

    bool schedulePriorityCommand(const char *cmd, size_t len = 0) override {
        if (txLocked) return false;
        return GCodeDevice::schedulePriorityCommand(cmd, len);
    }

    const char *getStatusStr() const override;

    static void sendProbe(Stream &serial);

    static bool checkProbeResponse(const String s);

protected:
    void trySendCommand() override;

    void tryParseResponse(char *cmd, size_t len) override;

    bool scheduleCommand(const char *cmd, size_t len) override;

private:
    SimpleCounter<15, 128> sentQueue;

    float hotendTemp, bedTemp = 0.0;
    float e = 0;
    int resendLine = -1;

    void parseError(const char *input);

    void parseStatus(const char *v);

    void parseOk(const char *v);
};
