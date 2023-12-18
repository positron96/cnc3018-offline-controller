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

#ifdef ADD_LINECOMMENTS // wierd shit TODO check wat it is
    static size_t nline=0;
     if(curUnsentPriorityCmdLen == 0) {
#ifdef ADD_LINECOMMENTS
             char tmp[MAX_GCODE_LINE+1];
             curUnsentPriorityCmdLen = xMessageBufferReceive(buf0, tmp, MAX_GCODE_LINE, 0);
             if(curUnsentPriorityCmdLen!=0) {
                 tmp[curUnsentPriorityCmdLen] = 0;
                 snprintf(curUnsentPriorityCmd, MAX_GCODE_LINE, "%s ;%d", tmp, nline++);
                 curUnsentPriorityCmdLen = strlen(curUnsentPriorityCmd);
             }
#else
             curUnsentPriorityCmdLen = xMessageBufferReceive(buf0, curUnsentPriorityCmd, MAX_GCODE_LINE, 0);
             curUnsentPriorityCmd[curUnsentPriorityCmdLen]=0;
#endif
     }

     if(curUnsentPriorityCmdLen==0 && curUnsentCmdLen==0) {
#ifdef ADD_LINECOMMENTS
             char tmp[MAX_GCODE_LINE+1];
             curUnsentCmdLen = xMessageBufferReceive(buf1, tmp, MAX_GCODE_LINE, 0);
             if(curUnsentCmdLen!=0) {
                 tmp[curUnsentCmdLen] = 0;
                 snprintf(curUnsentCmd, MAX_GCODE_LINE, "%s ;%d", tmp, nline++);
                 curUnsentCmdLen = strlen(curUnsentCmd);
             }
#else
             curUnsentCmdLen = xMessageBufferReceive(buf1, curUnsentCmd, MAX_GCODE_LINE, 0);
             curUnsentCmd[curUnsentCmdLen] = 0;
#endif
         //loadedNewCmd = true;
     }
#endif

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




