//
// Created by lima on 12/10/23.
//
#include "MarlinDevice.h"


void MarlinDevice::sendProbe(Stream &serial) {
    serial.print("\n");
    serial.print(GET_FIRMWARE_VER);
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


bool MarlinDevice::canJog(){
    return true; // TODO is it work in general or depodends on machine state
};