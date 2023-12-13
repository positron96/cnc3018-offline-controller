#pragma once

#include <Arduino.h>
#include <etl/observer.h>
#include "WatchedSerial.h"
#include "CommandQueue.h"

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
            printerSerial(s), connected(false) {

    }

    GCodeDevice() : printerSerial(nullptr), connected(false) {}

    virtual ~GCodeDevice() { clear_observers(); }

    virtual void begin() {
        while (printerSerial->available() > 0) {
            printerSerial->read();
        }
        readLockedStatus();
    };

    virtual bool scheduleCommand(const char *cmd, size_t len = 0) {
        if (panic)
            return false;
        if (len == 0)
            len = strlen(cmd);
        if (len == 0)
            return false;
        if (curUnsentCmdLen != 0)
            return false;

        memcpy(curUnsentCmd, cmd, len);
        curUnsentCmdLen = len;
        return true;
    };

    virtual bool schedulePriorityCommand(const char *cmd, size_t len = 0) {
        if (len == 0)
            len = strlen(cmd);
        if (len == 0)
            return false;
        if (curUnsentPriorityCmdLen != 0)
            return false;

        memcpy(curUnsentPriorityCmd, cmd, len);
        curUnsentPriorityCmdLen = len;
        return true;
    }

    virtual bool canSchedule(size_t len) {
        if (panic)
            return false;
        if (len == 0)
            return false;
        return curUnsentCmdLen == 0;
    }

    virtual bool jog(uint8_t axis, float dist, int feed = 100) = 0;

    virtual bool canJog() { return true; }

    virtual void loop() {
        readLockedStatus();
        sendCommands();
        receiveResponses();
        checkTimeout();

        if (nextStatusRequestTime != 0 && int32_t(millis() - nextStatusRequestTime) > 0) {
            requestStatusUpdate();
            nextStatusRequestTime = millis() + STATUS_REQUEST_INTERVAL;
        }
    }

    virtual void sendCommands();

    virtual void receiveResponses();

    float getX() const { return x; }

    float getY() const { return y; }

    float getZ() const { return z; }

    uint32_t getSpindleVal() const { return spindleVal; }

    uint32_t getFeed() const { return feed; }

    bool isConnected() const { return connected; }

    virtual void reset() = 0;

    bool isInPanic() const { return panic; }

    void enableStatusUpdates(bool v = true) {
        if (v)
            nextStatusRequestTime = millis();
        else
            nextStatusRequestTime = 0;
    }

    virtual void requestStatusUpdate() = 0;

    virtual const char *getStatusStr() const = 0;

    bool isLocked() { return printerSerial->isLocked(); }

protected:
    WatchedSerial *printerSerial;

    uint32_t serialRxTimeout;
    bool connected;
    bool canTimeout;
    bool panic = false;
    bool xoff;
    bool xoffEnabled = false;
    bool txLocked = false;

    static const size_t MAX_GCODE_LINE = 96;

    char curUnsentCmd[MAX_GCODE_LINE + 1], curUnsentPriorityCmd[MAX_GCODE_LINE + 1];
    size_t curUnsentCmdLen, curUnsentPriorityCmdLen;

    float x, y, z;
    uint32_t feed, spindleVal;
    uint32_t nextStatusRequestTime;

    Counter *sentCounter;

    void armRxTimeout() {
        if (!canTimeout) return;

        LOGLN(isRxTimeoutEnabled()
              ?
              "GCodeDevice::resetRxTimeout enable"
              : "GCodeDevice::resetRxTimeout disable");
        serialRxTimeout = millis() + KEEPALIVE_INTERVAL;
    };

    void disarmRxTimeout() {
        if (!canTimeout) return;

        serialRxTimeout = 0;
    };

    void updateRxTimeout(bool waitingMore) {
        if (isRxTimeoutEnabled()) { if (!waitingMore) disarmRxTimeout(); else armRxTimeout(); }
    }

    bool isRxTimeoutEnabled() { return canTimeout && serialRxTimeout != 0; }

    void checkTimeout() {
        if (!isRxTimeoutEnabled()) return;
        if (millis() > serialRxTimeout) {
            LOGLN("GCodeDevice::checkTimeout fired");
            connected = false;
            cleanupQueue();
            disarmRxTimeout();
            notify_observers(DeviceStatusEvent{DeviceStatus::DEV_ERROR});
        }
    }

    void cleanupQueue() {
        sentCounter->clear();
        curUnsentCmdLen = 0;
        curUnsentPriorityCmdLen = 0;
    }

    virtual void trySendCommand() = 0;

    virtual void tryParseResponse(char *cmd, size_t len) = 0;

    void readLockedStatus() {
        bool t = printerSerial->isLocked(true);
        if (t != txLocked)
            notify_observers(DeviceStatusEvent{DeviceStatus::UNLOCKED});
        txLocked = t;
    }

private:

};
// todo utils for string was here

inline bool startsWith(const char *str, const char *pre) {
    return strncmp(pre, str, strlen(pre)) == 0;
}