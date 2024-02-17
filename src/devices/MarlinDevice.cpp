#include <vector>
#include "constants.h"
#include "MarlinDevice.h"

#include "Job.h"
#include "printfloat.h"
#include "util.h"
#include "debug.h"


void MarlinDevice::sendProbe(Stream& serial) {
    serial.print("\n");
    serial.print(M115_GET_FIRMWARE_VER);
    serial.print("\n");
}

// FIRMWARE_NAME:Marlin
//   In Marlin first 3 numbers is the position for the planner.
//   The other positions are the positions from the stepper function.
//   This helps for debugging a previous stepper function bug.
// todo for position
bool MarlinDevice::checkProbeResponse(const String v) {
    // TODO  check: is v copied each invocation.
    if (v.indexOf("Marlin") != -1) {
        LOGLN(">> Detected Marlin device <<");
        return true;
    }
    return false;
}


MarlinDevice::MarlinDevice(WatchedSerial* s, Job* job) : GCodeDevice(s, job) {
    canTimeout = false;
    useLineNumber = true;
    lastResponse = nullptr;
}

bool MarlinDevice::jog(uint8_t axis, float dist, int feed) {
    constexpr size_t LN = 25;
    char msg[LN];
    // adding G91_SET_RELATIVE_COORDINATES to command
    // does not work. 
    int l = snprintf(msg, LN, "G0 F%d %c", feed, AXIS[axis]);
    snprintfloat(msg + l, LN - l, dist, 3);
    return scheduleCommand(msg, strlen(msg));
}

void MarlinDevice::trySendCommand() {
    if (lastStatus == DeviceStatus::WAIT) {
        return;
    }
    if (printerSerial->availableForWrite() && !outQueue.empty()) {
        String& front = outQueue.front();
        const char* cmd = front.c_str();
        auto size = (size_t) front.length();
        LOGF("[%s]\n", cmd);
        printerSerial->write((const unsigned char*) cmd, size);
        printerSerial->write('\n');
        //delete
        outQueue.pop_front();
        lastResponse = nullptr;
        lastStatus = DeviceStatus::WAIT;
    }
}

bool MarlinDevice::scheduleCommand(const char* cmd, size_t len = 0) {
    if (resendLine > 0) {
        return false;
    }
    if (!outQueue.full()) {
        outQueue.push_back(String(cmd));
        return true;
    }
    return false;
}

bool MarlinDevice::schedulePriorityCommand(const char* cmd, size_t len = 0) {
    if (txLocked)
        return false;
    return scheduleCommand(cmd, len);
}

void MarlinDevice::begin() {
    GCodeDevice::begin();
    constexpr size_t LN = 11;
    char msg[LN];
    int l = snprintf(msg, LN, "%s S%d", M154_AUTO_REPORT_POSITION, 1);
    scheduleCommand(msg, l);
    l = snprintf(msg, LN, "%s S%d", M155_AUTO_REPORT_TEMP, 1);
    scheduleCommand(msg, l);
    scheduleCommand(RESET_LINE_NUMBER);
}

void MarlinDevice::requestStatusUpdate() {
}

void MarlinDevice::reset() {
    lastStatus = DeviceStatus::OK;
}

void MarlinDevice::toggleRelative() {
    relative = !relative;
}
// TODO optimize all this silly ifs
void MarlinDevice::tryParseResponse(char* resp, size_t len) {
    LOGF("> [%s],%d\n", resp, len);
    if (startsWith(resp, "Error") || startsWith(resp, "!!")) {
        if (startsWith(resp, "Error")) {
            lastResponse = resp + 5;
            parseError(lastResponse); // TODO
        } else {
            lastResponse = resp + 2;
        }
        lastStatus = DeviceStatus::DEV_ERROR;
        outQueue.clear();
    } else {
        connected = true;
        if (startsWith(resp, "ok")) {
            if (len > 2) {
                parseOk(resp + 2, len - 2);
            }
            if (resendLine > 0) { // was resend before
                resendLine = -1;
            }
            lastResponse = resp;
            lastStatus = DeviceStatus::OK;
        } else {
            if (strstr(resp, "busy:") != nullptr) {
                lastResponse = resp + 5; // marlin do space after :
                lastStatus = DeviceStatus::BUSY;
                LOGLN("SET busy");
            }
            if (startsWith(resp, "echo:")) {
                // echo: busy must be before this
                lastResponse = resp + 5;
            } else if (startsWith(resp, "Resend: ")) {
                // MAY hae "Resend:Error
                lastResponse = resp + 7;
                resendLine = atoi(resp);
                lastStatus = DeviceStatus::RESEND;
                // no pop. resend
            } else {
                if (startsWith(resp, "DEBUG:")) {
                    lastResponse = resp;
                } else {
                    // M154 Snn or  M155 Snn
                    parseOk(resp, len);
                }
                lastStatus = DeviceStatus::OK;
            }
        }
        notify_observers(DeviceStatusEvent{});
    }
}

bool MarlinDevice::tempChange(uint8_t temp) {
    constexpr size_t LN = 11;
    char msg[LN];
    int l = snprintf(msg, LN, "%s S%d", M104_SET_EXTRUDER_TEMP, temp);
    return scheduleCommand(msg, l);
}

const char* MarlinDevice::getStatusStr() const {
    return ""/*lastResponse*/; //todo
}

///  marlin dont jog, just do G0
/// \return
bool MarlinDevice::canJog() {
    return lastStatus == DeviceStatus::OK;
}
/// ok T:201 /202 B:117 /120 @:255
///
///  "ok C: X:0.00 Y:0.00 Z:0.00 E:0.00"
///  " X:0.00 Y:0.00 RZ:0.00 LZ:0.00 Count X:0.00 Y:0.00 RZ:41.02 LZ:41.02"

/// \param input
/// \param len
void MarlinDevice::parseOk(const char* input, size_t len) {
    char cpy[BUFFER_LEN];
    strncpy(cpy, input, MIN(len, BUFFER_LEN));
    cpy[MIN(len, BUFFER_LEN)] = 0;

    bool nextTemp = false,
            nextBedTemp = false;
//    char *v = cpy;
    char* fromMachine = strtok(cpy, " ");
#define ATOF _atod
    while (fromMachine != nullptr) {
        switch (fromMachine[0]) {
            case 'T':
                hotendTemp = ATOF((fromMachine + 2));
                nextTemp = true;
                break;
            case 'B':
                if (fromMachine[1] == '@') {
                    bedPower = atoi((fromMachine + 3));
                } else {
                    bedTemp = ATOF((fromMachine + 2));
                    nextBedTemp = true;
                }
                break;
            case 'X':
                x = ATOF((fromMachine + 2));
                break;
            case 'Y':
                y = ATOF((fromMachine + 2));
                break;
            case 'Z':
                z = ATOF((fromMachine + 2));
                break;
            case 'E':
                e = ATOF((fromMachine + 2));
                break;
            case '@':
                hotendPower = atoi(fromMachine + 2);
                break;
            case 'C': // next is coords
                if (fromMachine[1] == 'o') {
                    goto end;
                }
            case '/':
                if (nextTemp) {
                    hotendRequestedTemp = ATOF((fromMachine + 1));
                } else if (nextBedTemp) {
                    bedRequestedTemp = ATOF((fromMachine + 1));
                }
                nextTemp = false;
                nextBedTemp = false;
            default:
                break;
        }
        fromMachine = strtok(nullptr, " ");
    }
    end:;// noop
#undef ATOF
}

void MarlinDevice::parseError(const char* input) {
    char cpy[SHORT_BUFFER_LEN];
    strncpy(cpy, input, SHORT_BUFFER_LEN);
    if (strstr(cpy, "Last Line") != nullptr) {
//        int lastResponse = atoi((cpy + 10));
    }
}

const etl::ivector<u_int16_t>& MarlinDevice::getSpindleValues() const {
    return MarlinDevice::SPINDLE_VALS;
}
