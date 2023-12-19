#include "GCodeDevice.h"

#define XOFF  0x13
#define XON   0x11

// utils for string was here

void GCodeDevice::sendCommands() {
    if (panic) {
        // drain queue
        curUnsentCmdLen = 0;
        return;
    }

    if (xoffEnabled && xoff)
        return;

    if (txLocked)
        return;

    if (curUnsentCmdLen == 0 && curUnsentPriorityCmdLen == 0)
        return;

    trySendCommand();
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




