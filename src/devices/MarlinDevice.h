#pragma once

#include "GCodeDevice.h"
#include "gcode/gcode.h"


class MarlinDevice : public GCodeDevice {
public:
    constexpr static uint32_t BUFFER_LEN = 255;

    MarlinDevice(WatchedSerial *s) :
            GCodeDevice(s) {
        sentCounter = &sentQueue;
        canTimeout = false;
        panic = false;
    }

    MarlinDevice() : GCodeDevice() { sentCounter = &sentQueue; }

    virtual ~MarlinDevice() {}

    bool jog(uint8_t axis, float dist, int feed) override;

    bool canJog() override;

    void begin() override {
        GCodeDevice::begin();
    }

    void reset() override {
        panic = false;
        busy = false;
        cleanupQueue();
        // TODO Marlin has panic at all ??
    }


    void requestStatusUpdate() override {
        if (panic || busy)
            return;
        schedulePriorityCommand(M114_GET_CURRENT_POS);
    }

    bool schedulePriorityCommand(const char *cmd, size_t len = 0) override {
        if (txLocked) return false;
        return GCodeDevice::schedulePriorityCommand(cmd, len);
    }

    bool scheduleCommand(const char *cmd, size_t len) override;

    const char *getStatusStr() const override;

    bool isRelative() const {
        return relative;
    }

    void toggleRelative() {
        relative = !relative;
    }

    float getE() const { return e; }

    void tempChange(uint8_t temp) {
        constexpr size_t LN = 11;
        char msg[LN];
        int l = snprintf(msg, LN, "%s S%d", M104_SET_EXTRUDER_TEMP, temp);
        scheduleCommand(msg, l);
        //todo must return
    }

    float getTemp() const { return hotendTemp; }

    float getBedTemp() const { return bedTemp; }

    static void sendProbe(Stream &serial);

    static bool checkProbeResponse(String s);

protected:
    void trySendCommand() override;

    void tryParseResponse(char *cmd, size_t len) override;

private:
    SimpleCounter<15, 128> sentQueue;

    float hotendTemp = 0.0, bedTemp = 0.0;
    size_t hotendPower = 0;
    size_t bedPower = 0;
    float hotendRequestedTemp = 0.0, bedRequestedTemp = 0.0;
    float e = 0.0;
    bool busy = false;

    bool relative = false;

    int resendLine = -1;

    void parseError(const char *input);

    void parseStatus(const char *v);

    void parseOk(const char *v, size_t len);
};
