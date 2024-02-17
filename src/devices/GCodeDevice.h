#pragma once

#include <Arduino.h>
#include <etl/vector.h>
#include <etl/observer.h>
#include "WatchedSerial.h"
#include "CommandQueue.h"

class Job;

#include "util.h"

#include "debug.h"

#define KEEPALIVE_INTERVAL 5000    // Marlin defaults to 2 seconds, get a little of margin
#define STATUS_REQUEST_INTERVAL  500

// TODO LIST
// TODO 1 switch to states from flags for protocol state  90%
// TODO 2 make observable events meaningful

constexpr uint8_t MAX_DEVICE_OBSERVERS = 3;

struct DeviceStatusEvent {
};

using DeviceObserver = etl::observer<const DeviceStatusEvent&>;

class GCodeDevice : public etl::observable<DeviceObserver, MAX_DEVICE_OBSERVERS> {
public:
    ///
    /// Device abstraction statuses.
    /// Real Device can have different statuses,
    /// but they can be mapped to this.
    /// Use it for general communication with DRO and Job.
    struct DeviceStatus {
        enum {
            DISCONNECTED,
            OK,
            MSG,
            RESEND,
            // -------- WAIT states
            WAIT = 10,
            BUSY,
            // -------- Error states
            ALARM = 100,
            DEV_ERROR,
        };
    };


    GCodeDevice(WatchedSerial* s, Job* job_) :
            printerSerial(s), job(job_), connected(false) {}

    GCodeDevice() : printerSerial(nullptr), connected(false) {}

    virtual ~GCodeDevice() { clear_observers(); }

    virtual void begin();

    virtual bool scheduleCommand(const char* cmd, size_t len = 0);

    /// Schedule just put it to command var. sendCommands() do work for send.
    ///
    ///  Priority commands are commands for polling status or jogging.
    ///  It has no N<\d> number prefix, if line numbers are used.
    ///
    /// \param cmd
    /// \param len
    /// \return false if fail to schedule.
    virtual bool schedulePriorityCommand(const char* cmd, size_t len = 0);

    virtual bool jog(uint8_t axis, float dist, int feed = 100) = 0;

    virtual void reset() = 0;

    virtual void requestStatusUpdate() = 0;

    virtual const char* getStatusStr() const = 0;

    void enableStatusUpdates(bool v = true);

    virtual void step();

    virtual void receiveResponses();

    virtual bool canJog() { return true; }

    bool supportLineNumber() { return useLineNumber; }

    virtual const etl::ivector<u_int16_t>& getSpindleValues() const = 0;

    float getX() const { return x; }

    float getY() const { return y; }

    float getZ() const { return z; }

    uint32_t getSpindleVal() const { return spindleVal; }

    uint32_t getFeed() const { return feed; }

    bool isConnected() const { return connected; }

    int getLastStatus() const { return lastStatus; }

    bool isLocked() { return printerSerial->isLocked(); }

protected:
    static const size_t MAX_GCODE_LINE = 96;

    WatchedSerial* printerSerial;
    Job* job;

    uint32_t serialRxTimeout;
    bool connected; // TODO use status DISCONNECTED
    bool canTimeout;
    bool xoff;
    bool xoffEnabled = false;
    bool txLocked = false;
    bool useLineNumber = false;

    char curUnsentCmd[MAX_GCODE_LINE + 1], curUnsentPriorityCmd[MAX_GCODE_LINE + 1];
    size_t curUnsentCmdLen;
    size_t curUnsentPriorityCmdLen;

    const char* lastResponse;
    size_t lastStatus = DeviceStatus::DISCONNECTED;

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

    virtual void tryParseResponse(char* cmd, size_t len) = 0;

    void readLockedStatus();
};
