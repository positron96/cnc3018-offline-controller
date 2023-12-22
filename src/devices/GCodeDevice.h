#pragma once

#include <Arduino.h>
#include <etl/observer.h>
#include "WatchedSerial.h"
#include "CommandQueue.h"
#include "util.h"

#include "debug.h"

#define KEEPALIVE_INTERVAL 5000    // Marlin defaults to 2 seconds, get a little of margin
#define STATUS_REQUEST_INTERVAL  500

constexpr uint8_t MAX_DEVICE_OBSERVERS = 3;

struct DeviceStatusEvent {
    int statusField;
};

using DeviceObserver = etl::observer<const DeviceStatusEvent &>;

class GCodeDevice : public etl::observable<DeviceObserver, MAX_DEVICE_OBSERVERS> {
public:
    enum DeviceStatus {
        OK = 0,
        DEV_ERROR,
        ALARM,
        MSG,
        UNLOCKED = 10
    };

    GCodeDevice(WatchedSerial *s) :
            printerSerial(s), connected(false) {}

    GCodeDevice() : printerSerial(nullptr), connected(false) {}

    virtual ~GCodeDevice() { clear_observers(); }

    virtual void begin();

    virtual bool scheduleCommand(const char *cmd, size_t len = 0);

    virtual bool schedulePriorityCommand(const char *cmd, size_t len = 0);

    virtual bool canSchedule(size_t len);

    virtual bool jog(uint8_t axis, float dist, int feed = 100) = 0;

    virtual void reset() = 0;

    virtual void requestStatusUpdate() = 0;

    virtual const char *getStatusStr() const = 0;

    void enableStatusUpdates(bool v = true);

    virtual void loop();

    virtual void sendCommands();

    virtual void receiveResponses();

    virtual bool canJog() { return true; }

    float getX() const { return x; }

    float getY() const { return y; }

    float getZ() const { return z; }

    uint32_t getSpindleVal() const { return spindleVal; }

    uint32_t getFeed() const { return feed; }

    bool isConnected() const { return connected; }

    bool isInPanic() const { return panic; }

    const String &getLastResponse() const { return lastResponse; }

    bool isLocked() { return printerSerial->isLocked(); }

protected:
    static const size_t MAX_GCODE_LINE = 96;

    WatchedSerial *printerSerial;
    Counter *sentCounter;

    uint32_t serialRxTimeout;
    bool connected;
    bool canTimeout;
    bool panic = false;
    bool xoff;
    bool xoffEnabled = false;
    bool txLocked = false;

    char curUnsentCmd[MAX_GCODE_LINE + 1], curUnsentPriorityCmd[MAX_GCODE_LINE + 1];
    size_t curUnsentCmdLen;
    size_t curUnsentPriorityCmdLen;
    const char *lastResponse;

    float x, y, z;
    uint32_t feed, spindleVal;
    uint32_t nextStatusRequestTime;

    void armRxTimeout();

    void disarmRxTimeout();

    void updateRxTimeout(bool waitingMore);

    bool isRxTimeoutEnabled();

    void checkTimeout();

    void cleanupQueue();

    virtual void trySendCommand() = 0;

    virtual void tryParseResponse(char *cmd, size_t len) = 0;

    void readLockedStatus();
};
