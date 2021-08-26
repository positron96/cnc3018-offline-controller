#include "DRO.h"


    void DRO::onButton(int bt, int8_t arg) {
        GCodeDevice *dev = GCodeDevice::getDevice();
        if(dev==nullptr) {
            S_DEBUGF("device is null\n");
            return;
        }
        if(!arg) return;
        switch(bt) {          
            case Display::BT_CENTER: dev->schedulePriorityCommand("?"); break;
            case Display::BT_STEP: dev->scheduleCommand("$X"); break;
            case Display::BT_L: dev->jog(0, -1, 500); break;
            case Display::BT_R: dev->jog(0, 1, 500); break;
            case Display::BT_UP: dev->jog(1, 1, 500); break;
            case Display::BT_DOWN: dev->jog(1, -1, 500); break;
            case Display::BT_ZUP: dev->jog(2, 1, 500); break;
            case Display::BT_ZDOWN: dev->jog(2, -1, 500); break;    
            default: break;
        }
    };
