#include "constants.h"
#include "GCodeDevice.h"

// utils for string was here

bool GCodeDevice::scheduleCommand(const char* cmd, size_t len) {
    if (lastStatus >= DeviceStatus::ALARM)
        return false;
    if (len == 0)
        len = strlen(cmd);
    if (len == 0)
        return false;
    if (curUnsentCmdLen != 0)
        return false;
    LOGF("< '%s' \n",cmd);
    memcpy(curUnsentCmd, cmd, len);
    curUnsentCmdLen = len;
    return true;
};

bool GCodeDevice::schedulePriorityCommand(const char* cmd, size_t len) {
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

void GCodeDevice::step() {
    readLockedStatus();
    if(lastStatus == DeviceStatus::ALARM){
        cleanupQueue();
    } else if (
            (!xoffEnabled || !xoff)
            && !txLocked ) {
        // no check for queued commands.
        // do it inside trySend
        trySendCommand();
    }
    receiveResponses();
    checkTimeout();

    if (nextStatusRequestTime != 0 && int32_t(millis() - nextStatusRequestTime) > 0) {
        requestStatusUpdate();
        nextStatusRequestTime = millis() + STATUS_REQUEST_INTERVAL;
    }
}

void GCodeDevice::begin() {
    while (printerSerial->available() > 0) {
        printerSerial->read();
    }
    readLockedStatus();
}

void GCodeDevice::readLockedStatus() {
    bool t = printerSerial->isLocked(true);
    if (t != txLocked)
        notify_observers(DeviceStatusEvent{});
    txLocked = t;
}

void GCodeDevice::cleanupQueue() {
    curUnsentCmdLen = 0;
    curUnsentPriorityCmdLen = 0;
}

void GCodeDevice::checkTimeout() {
    if (!isRxTimeoutEnabled()) return;
    if (millis() > serialRxTimeout) {
        LOGLN("GCodeDevice::checkTimeout fired");
        connected = false;
        cleanupQueue();
        disarmRxTimeout();
        notify_observers(DeviceStatusEvent{});
    }
}

void GCodeDevice::armRxTimeout() {
    if (!canTimeout) return;

    LOGLN(isRxTimeoutEnabled()
          ?
          "GCodeDevice::resetRxTimeout enable"
          : "GCodeDevice::resetRxTimeout disable");
    serialRxTimeout = millis() + KEEPALIVE_INTERVAL;
};

void GCodeDevice::disarmRxTimeout() {
    if (!canTimeout)
        return;
    serialRxTimeout = 0;
};

void GCodeDevice::updateRxTimeout(bool waitingMore) {
    if (isRxTimeoutEnabled()) {
        if (!waitingMore)
            disarmRxTimeout();
        else
            armRxTimeout();
    }
}

bool GCodeDevice::isRxTimeoutEnabled() {
    return canTimeout && serialRxTimeout != 0;
}

void GCodeDevice::enableStatusUpdates(bool v) {
    if (v)
        nextStatusRequestTime = millis();
    else
        nextStatusRequestTime = 0;
}

void GCodeDevice::receiveResponses() {
    static const size_t MAX_LINE = 200; // M115 is far longer than 100
    static char resp[MAX_LINE + 1];
    static size_t respLen;

    while (printerSerial->available()) {
        char ch = (char) printerSerial->read();
        switch (ch) {
            case '\n':
            case '\r':
                break;
            case XOFF:
                if (xoffEnabled) {
                    xoff = true;
                    break;
                }
            case XON:
                if (xoffEnabled) {
                    xoff = false;
                    break;
                }
            default:
                if (respLen < MAX_LINE) resp[respLen++] = ch;
        }
        if (ch == '\n') {
            resp[respLen] = 0;
            tryParseResponse(resp, respLen);
            respLen = 0;
        }
    }

}
