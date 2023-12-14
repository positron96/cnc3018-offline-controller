#include "GrblDevice.h"
#include "printfloat.h"


void GrblDevice::sendProbe(Stream &serial) {
    serial.print("\n$I\n");
}

bool GrblDevice::checkProbeResponse(const String v) {
    if (v.indexOf("[VER:") != -1) {
        return true;
    }
    return false;
}


bool GrblDevice::jog(uint8_t axis, float dist, int feed) {
    constexpr static char AXIS[] = {'X', 'Y', 'Z'};
    constexpr size_t LN = 25;
    char msg[LN];
    // "$J=G91 G20 X0.5" will move +0.5 inches (12.7mm) to X=22.7mm (WPos).
    // Note that G91 and G20 are only applied to this jog command
    int l = snprintf(msg, LN, "$J=G91 F%d %c", feed, AXIS[axis]);
    snprintfloat(msg + l, LN - l, dist, 3);
    return scheduleCommand(msg, strlen(msg));
}

bool GrblDevice::canJog() {
    return status == Status::Idle || status == Status::Jog;
}

bool GrblDevice::isCmdRealtime(const char *data, size_t len) {
    if (len != 1)
        return false;

    char c = data[0];
    switch (c) {
        case '?': // status
        case '~': // cycle start/stop
        case '!': // feedhold
        case 0x18: // ^x, reset
        case 0x84: // door
        case 0x85: // jog cancel
        case 0x9E: // toggle spindle
        case 0xA0: // toggle flood coolant
        case 0xA1: // toggle mist coolant
        case 0x90 ... 0x9D: // feed override, rapid override, spindle override
            return true;
        default:
            return false;
    }
}

void GrblDevice::trySendCommand() {

#ifdef ADD_LINECOMMENTS
    if (isCmdRealtime(curUnsentPriorityCmd, curUnsentPriorityCmdLen)) {
        printerSerial->write((const uint8_t *) curUnsentPriorityCmd, curUnsentPriorityCmdLen);

        LOGF("<  (f%3d,%3d) '%c' RT\n", sentCounter->getFreeLines(), sentCounter->getFreeBytes(),
                  curUnsentPriorityCmd[0]);

        curUnsentPriorityCmdLen = 0;
        return;
    }
#endif // ADD_LINECOMMENTS

    char *cmd = curUnsentPriorityCmdLen != 0 ? &curUnsentPriorityCmd[0] : &curUnsentCmd[0];
    size_t &len = curUnsentPriorityCmdLen != 0 ? curUnsentPriorityCmdLen : curUnsentCmdLen;
    cmd[len] = 0;
    if (sentCounter->canPush(len)) { //todo how it work on resend
        sentCounter->push(cmd, len);
        printerSerial->write((const uint8_t *) cmd, len);
        printerSerial->write('\n');
        LOGF("<  (f%3d,%3d) '%s'\n", sentCounter->getFreeLines(), sentCounter->getFreeBytes(), cmd, len);
        len = 0;
    } else {
        LOGF("<  (f%3d,%3d) NO SPACE: '%s'(len %d)\n", sentQueue.getFreeLines(), sentQueue.getFreeBytes(), cmd, len);
        LOGF(".");
    }

}

void GrblDevice::tryParseResponse(char *resp, size_t len) {
    if (startsWith(resp, "ok")) {
        sentQueue.pop();
        connected = true;
        panic = false;
        lastResponse = resp;
        notify_observers(DeviceStatusEvent{DeviceStatus::OK});
    } else if (startsWith(resp, "<")) {
        parseStatus(resp + 1);
        panic = false;
    } else if (startsWith(resp, "error")) {
        sentQueue.pop();
        LOGF("ERR '%s'\n", resp);
        notify_observers(DeviceStatusEvent{DeviceStatus::DEV_ERROR});
        lastResponse = resp;
    } else if (startsWith(resp, "ALARM:")) {
        panic = true;
        LOGF("ALARM '%s'\n", resp);
        lastResponse = resp;
        // no mor status updates will come in, so update status.
        status = Status::Alarm;
        notify_observers(DeviceStatusEvent{DeviceStatus::ALARM});
    } else if (startsWith(resp, "[MSG:")) {
        LOGF("Msg '%s'\n", resp);
        resp[len - 1] = 0; // strip last ']'
        lastResponse = resp + 5;
        // this is the first message after reset
        notify_observers(DeviceStatusEvent{DeviceStatus::MSG});
    }
    LOGF("> (f%3d,%3d) '%s'\n", sentQueue.getFreeLines(), sentQueue.getFreeBytes(), resp);
}

void mystrcpy(char *dst, const char *start, const char *end) {
    while (start != end) {
        *(dst++) = *(start++);
    }
    *dst = 0;
}

void GrblDevice::parseStatus(char *v) {
    //<Idle|MPos:9.800,0.000,0.000|FS:0,0|WCO:0.000,0.000,0.000>
    //                                        override values in percent of programmed values for
    //                                    v-- feed, rapids, and spindle speed
    //<Idle|MPos:9.800,0.000,0.000|FS:0,0|Ov:100,100,100>
    char buf[10];
    bool mpos;
    char cpy[100];
    strncpy(cpy, v, 100);
    v = cpy;

    // idle/jogging
    char *fromGrbl = strtok(v, "|");
    if (fromGrbl == nullptr) return;
    setStatus(fromGrbl);

    // MPos:0.000,0.000,0.000
    fromGrbl = strtok(nullptr, "|");
    if (fromGrbl == nullptr) return;
    // ===========++++
    char *st, *fi;
    st = fromGrbl + 5;
    fi = strchr(st, ',');
    if (fi == nullptr)
        return;
    mystrcpy(buf, st, fi);
    x = _atod(buf);
    //==============
    st = fi + 1;
    fi = strchr(st, ',');
    if (fi == nullptr)
        return;
    mystrcpy(buf, st, fi);
    y = _atod(buf);
    //==============
    st = fi + 1;
    z = _atod(st);

    mpos = startsWith(fromGrbl, "MPos");
    LOGF("Parsed Pos: %f %f %f\n", x, y, z);
    //    +--  feed
    //    v   v-- spindle v-- feed
    // FS:500,8000     or F:500
    fromGrbl = strtok(nullptr, "|");
    while (fromGrbl != nullptr) {
        if (startsWith(fromGrbl, "FS:") || startsWith(fromGrbl, "F:")) {
            if (fromGrbl[1] == 'S') {
                st = fromGrbl + 3;
                fi = strchr(st, ',');
                if (fi == nullptr)return;
                mystrcpy(buf, st, fi);
                feed = atoi(buf);
                st = fi + 1;
                spindleVal = atoi(st);
            } else {
                feed = atoi(fromGrbl + 2);
            }
        } else if (startsWith(fromGrbl, "WCO:")) {
            st = fromGrbl + 4;
            fi = strchr(st, ',');
            if (fi == nullptr)return;
            mystrcpy(buf, st, fi);
            ofsX = _atod(buf);
            st = fi + 1;
            fi = strchr(st, ',');
            if (fi == nullptr)return;
            mystrcpy(buf, st, fi);
            ofsY = _atod(buf);
            st = fi + 1;
            ofsZ = _atod(st);
            LOGF("Parsed WCO: %f %f %f\n", ofsX, ofsY, ofsZ);
        }
        fromGrbl = strtok(nullptr, "|");
    }

    if (!mpos) {
        x -= ofsX;
        y -= ofsY;
        z -= ofsZ;
    }

    notify_observers(DeviceStatusEvent{DeviceStatus::OK});
}


bool GrblDevice::setStatus(const char *pch) {
    if (startsWith(pch, "Hold")) status = Status::Hold;
    else if (startsWith(pch, "Door")) status = Status::Door;
    else if (strcmp(pch, "Idle") == 0) status = Status::Idle;
    else if (strcmp(pch, "Run") == 0) status = Status::Run;
    else if (strcmp(pch, "Jog") == 0) status = Status::Jog;
    else if (strcmp(pch, "Alarm") == 0) status = Status::Alarm;
    else if (strcmp(pch, "Check") == 0) status = Status::Check;
    else if (strcmp(pch, "Home") == 0) status = Status::Home;
    else if (strcmp(pch, "Sleep") == 0) status = Status::Sleep;
    else {
        return false;
    }
    LOGF("Parsed Status: %d\n", status);
    return true;
}

const char *GrblDevice::getStatusStr() const {
    switch (status) {
        case Status::Idle:
            return "Idle";
        case Status::Run:
            return "Run";
        case Status::Jog:
            return "Jog";
        case Status::Alarm:
            return "Alarm";
        case Status::Hold:
            return "Hold";
        case Status::Door:
            return "Door";
        case Status::Check:
            return "Check";
        case Status::Home:
            return "Home";
        case Status::Sleep:
            return "Sleep";
        default:
            return "?";
    }
}
