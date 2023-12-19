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
    snprintfloat(msg + l, LN - l, dist, 3); //todo why + l -l  ???
    return scheduleCommand(msg, strlen(msg));
}

void MarlinDevice::trySendCommand() {
    char *cmd = curUnsentPriorityCmdLen != 0 ? &curUnsentPriorityCmd[0] : &curUnsentCmd[0];
    size_t &len = curUnsentPriorityCmdLen != 0 ? curUnsentPriorityCmdLen : curUnsentCmdLen;
    cmd[len] = 0;
    LOGF("Try [%s]\n", cmd);
    if (sentCounter->canPush(len)) { //todo how it work on resend
        sentCounter->push(cmd, len);
        if (printerSerial->availableForWrite()) {
            printerSerial->write((const uint8_t *) cmd, len);
            printerSerial->write('\n');
            len = 0; // todo this how it works on resend
        }
    }
}

bool MarlinDevice::scheduleCommand(const char *cmd, size_t len = 0) {
    if (resendLine > 0) {
        return false;
    }
    return GCodeDevice::scheduleCommand(cmd, len);
}

void MarlinDevice::tryParseResponse(char *resp, size_t len) {
    LOGF("> [%s]\n", resp);
    if (startsWith(resp, "Error") || startsWith(resp, "!!")) {
        if (startsWith(resp, "Error")) {
            sentQueue.pop();
            lastResponse = resp + 5;
            parseError(lastResponse); // TODO
        } else {
            cleanupQueue();
            sentQueue.pop();
            lastResponse = resp + 2;
        }
        panic = true;
        notify_observers(DeviceStatusEvent{DeviceStatus::DEV_ERROR});
    } else {
        if (startsWith(resp, "ok")) {
            if (len > 2) {
                parseOk(resp + 2, len - 2);
            }
            if (resendLine > 0) { // was resend before
                resendLine = -1;
            }
            sentQueue.pop();
            lastResponse = resp;

        }
            // stupid C substring index
        else if (strcspn(resp, "busy:") != len) {
            sentQueue.pop();
            lastResponse = resp + 5; // marlin do space after :
            // TODO busy state
        } else if (startsWith(resp, "Resend: ")) {
            lastResponse = resp + 7;
            resendLine = atoi(resp);
            // no pop. resend
        } else if (startsWith(resp, "DEBUG:")) {
            lastResponse = resp;
        } else {
            // M114 
            parseOk(resp, len);
        }
        connected = true;
        panic = false;
        if (curUnsentPriorityCmdLen) {
            curUnsentPriorityCmdLen = 0;
        } else {
            curUnsentCmdLen = 0;
        }
        notify_observers(DeviceStatusEvent{DeviceStatus::OK});
    }
}

const char *MarlinDevice::getStatusStr() const {
    return lastResponse; //todo
}

///  marlin dont jog, just do G0
/// \return
bool MarlinDevice::canJog() {
    // return !panic;
    return true; // todo depend on state remove
}
/// ok T:201 /202 B:117 /120 @:255
///
///  "ok C: X:0.00 Y:0.00 Z:0.00 E:0.00"
///  "X:0.00 Y:0.00 RZ:0.00 LZ:0.00 Count X:0.00 Y:0.00 RZ:41.02 LZ:41.02"
//

/// \param input
/// \param len
void MarlinDevice::parseOk(const char *input, size_t len) {
    char cpy[BUFFER_LEN];
    strncpy(cpy, input, MIN(len, BUFFER_LEN));
    cpy[MIN(len, BUFFER_LEN)] = 0;
    bool nextTemp = false, nextBedTemp = false;

//    char *v = cpy;
    char *fromMachine = strtok(cpy, " ");
    while (fromMachine != nullptr) {
        switch (fromMachine[0]) {
            case 'T':
                hotendTemp = atof((fromMachine + 2));
                nextTemp = true;
                break;
            case 'B':
                if (fromMachine[1] == '@') {
                    bedPower = atoi((fromMachine + 1));
                } else {
                    bedTemp = atof((fromMachine + 2));
                    nextBedTemp = true;
                }
                break;
            case 'X':
                x = atof((fromMachine + 2));
                break;
            case 'Y':
                y = atof((fromMachine + 2));
                break;
            case 'Z':
                z = atof((fromMachine + 2));
                break;
            case 'E':
                e = atof((fromMachine + 2));
                break;
            case '@':
                hotendPower = atoi(fromMachine + 1);
                break;
            case 'C': // next is coords
                if (fromMachine[1] == 'o') {
                    goto end;
                }
            case '/':
                if (nextTemp) {
                    hotendRequestedTemp = atof((fromMachine + 1));
                } else if (nextBedTemp) {
                    bedRequestedTemp = atof((fromMachine + 1));
                }
                nextTemp = false;
                nextBedTemp = false;
            default:
                break;
        }
        fromMachine = strtok(nullptr, " ");
    }
    end:;// noop
}

void MarlinDevice::parseError(const char *input) {
    char cpy[100]; // TODO inst lenth
    strncpy(cpy, input, 100);
    if (strncmp(cpy, "Last Line", 100) != -1) {
        int lastLine = atoi((cpy + 10));
    } else if (startsWith(cpy, "")) {

    }
}

void MarlinDevice::parseStatus(const char *input) {}
