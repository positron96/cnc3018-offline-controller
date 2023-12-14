#include "MarlinDevice.h"
#include "util.h"

void MarlinDevice::sendProbe(Stream &serial) {
    serial.print("\n");
    serial.print(M115_GET_FIRMWARE_VER);
    serial.print("\n");
}

// FIRMWARE_NAME:Marlin
// todo for position
//  "ok C: X:0.00 Y:0.00 Z:0.00 E:0.00"
// "X:0.00 Y:0.00 RZ:0.00 LZ:0.00 Count X:0.00 Y:0.00 RZ:41.02 LZ:41.02"
//
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

void MarlinDevice::tryParseResponse(char *resp, size_t len) {
    LOGF("> : %s\n", resp);
    if (startsWith(resp, "!!")) {
        sentQueue.pop();
        panic = true;
        lastResponse = resp + 2;
    } else {
        if (startsWith(resp, "ok")) {
            sentQueue.pop();
            lastResponse = "ok";
            notify_observers(DeviceStatusEvent{DeviceStatus::OK});
        } else if (startsWith(resp, "error ")) {
            sentQueue.pop();
            /// TODO
        } else if (startsWith(resp, "busy: ")) {
            sentQueue.pop();
            lastResponse = resp + 6; // marlin do space after :
            // TODO busy state
        } else if (startsWith(resp, "Resend: ")) {
            lastResponse = resp + 8;
            // no pop. resend
        } else if (startsWith(resp, "DEBUG:")) {
            lastResponse = resp;
        }
        connected = true;
        panic = false;
    }

}

const char *MarlinDevice::getStatusStr() const {
    return lastResponse.c_str(); //todo
}


bool MarlinDevice::canJog() {
    return true; // TODO is it work in general or depodends on machine state
}

void parseStatus(char *resp){


}
