#include "constants.h"
#include "MarlinDevice.h"
#include "printfloat.h"
#include "util.h"
#include "debug.h"


void MarlinDevice::sendProbe(Stream &serial) {
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
    if (v.indexOf("Marlin") != -1) {
        LOGLN(">> Detected Marlin device <<");
        return true;
    }
    return false;
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
    char *cmd = curUnsentPriorityCmdLen != 0 ? &curUnsentPriorityCmd[0] : &curUnsentCmd[0];
    size_t &len = curUnsentPriorityCmdLen != 0 ? curUnsentPriorityCmdLen : curUnsentCmdLen;
    cmd[len] = 0;
    LOGF("Try [%s]\n", cmd);
    if (sentCounter->canPush(len)) {
        sentCounter->push(cmd, len);
        if (printerSerial->availableForWrite()) {
            printerSerial->write((const uint8_t *) cmd, len);
            printerSerial->write('\n');
            len = 0;
        }
    }
}

bool MarlinDevice::scheduleCommand(const char *cmd, size_t len = 0) {
    if (busy || resendLine > 0) {
        return false;
    }
    return GCodeDevice::scheduleCommand(cmd, len);
}

bool MarlinDevice::schedulePriorityCommand(const char *cmd, size_t len = 0) {
    if (txLocked) return false;
    return GCodeDevice::schedulePriorityCommand(cmd, len);
}

void MarlinDevice::begin() {
    GCodeDevice::begin();
    constexpr size_t LN = 11;
    char msg[LN];
//todo refactor
    int l = snprintf(msg, LN, "%s S%d", M154_AUTO_REPORT_POSITION, 3);
    schedulePriorityCommand(msg, l
    );
    l = snprintf(msg, LN, "%s S%d", M155_AUTO_REPORT_TEMP, 3);
    scheduleCommand(msg, l
    );
}

void MarlinDevice::requestStatusUpdate() {
}

void MarlinDevice::reset() {
    panic = false;
    busy = false;
}

void MarlinDevice::toggleRelative() {
    relative = !relative;
}

void MarlinDevice::tryParseResponse(char *resp, size_t len) {
    LOGF("> [%s],%d\n", resp, len);
    if (startsWith(resp, "Error") || startsWith(resp, "!!")) {
        if (startsWith(resp, "Error")) {
            lastResponse = resp + 5;
            parseError(lastResponse); // TODO
        } else {
            lastResponse = resp + 2;
        }
        panic = true;
        cleanupQueue();
        notify_observers(DeviceStatusEvent{DeviceStatus::DEV_ERROR});
    } else {
        connected = true;
        panic = false;
        if (startsWith(resp, "ok")) {
            if (len > 2) {
                parseOk(resp + 2, len - 2);
            }
            if (resendLine > 0) { // was resend before
                resendLine = -1;
            }
            sentQueue.pop();
            lastResponse = resp;
            busy = false;
        } else {
            // stupid C substring index
            if (strcspn(resp, "busy:") != 1) {
                sentQueue.pop();
                lastResponse = resp + 5; // marlin do space after :
                busy = true;
            } else if (startsWith(resp, "Resend: ")) {
                lastResponse = resp + 7;
                resendLine = atoi(resp);
                // no pop. resend
            } else if (startsWith(resp, "DEBUG:")) {
                lastResponse = resp;
            } else {
                // M154 Snn or  M155 Snn
                parseOk(resp, len);
            }
        }
        notify_observers(DeviceStatusEvent{DeviceStatus::OK});
    }
}

bool MarlinDevice::tempChange(uint8_t temp) {
    constexpr size_t LN = 11;
    char msg[LN];
    int l = snprintf(msg, LN, "%s S%d", M104_SET_EXTRUDER_TEMP, temp);
    return scheduleCommand(msg, l);
}

const char *MarlinDevice::getStatusStr() const {
    return lastResponse; //todo
}

///  marlin dont jog, just do G0
/// \return
bool MarlinDevice::canJog() {
    return !busy && !panic;
}
/// ok T:201 /202 B:117 /120 @:255
///
///  "ok C: X:0.00 Y:0.00 Z:0.00 E:0.00"
///  " X:0.00 Y:0.00 RZ:0.00 LZ:0.00 Count X:0.00 Y:0.00 RZ:41.02 LZ:41.02"

/// \param input
/// \param len
void MarlinDevice::parseOk(const char *input, size_t len) {
    char cpy[BUFFER_LEN];
    strncpy(cpy, input, MIN(len, BUFFER_LEN));
    cpy[MIN(len, BUFFER_LEN)] = 0;

    bool nextTemp = false,
            nextBedTemp = false;
//    char *v = cpy;
    char *fromMachine = strtok(cpy, " ");
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

void MarlinDevice::parseError(const char *input) {
    char cpy[100]; // TODO inst lenth
    strncpy(cpy, input, 100);
    if (strncmp(cpy, "Last Line", 100) != -1) {
        int lastLine = atoi((cpy + 10));
    } else if (startsWith(cpy, "")) {

    }
}
