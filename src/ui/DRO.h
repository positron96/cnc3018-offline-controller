#pragma once

#include <etl/map.h>

#include "Screen.h"

#include "../devices/GCodeDevice.h"


/*
enum class JogAxis {
    X,Y,Z
};
enum class JogDist {
    _01, _1, _10
};
*/
using JogAxis = int;
using JogDist = int;



class DRO: public Screen {
public:
    static constexpr uint16_t REFRESH_INTL = 500;

    DRO(): nextRefresh(1) {}
    
    void begin() override {
        /*
        menuItems.push_back("oFile...");
        menuItems.push_back("xReset");
        menuItems.push_back("uUpdate");
        */
    };

    void loop() override {
        Screen::loop();
        if(nextRefresh!=0 && millis()>nextRefresh) {
            nextRefresh = millis() + REFRESH_INTL;
            GCodeDevice *dev = GCodeDevice::getDevice();
            if (dev!=nullptr) {
                dev->requestStatusUpdate();
                //setDirty();
            }
        }
    }

    void enableRefresh(bool r) { nextRefresh = r ? millis() : 0;  }
    bool isRefreshEnabled() { return nextRefresh!=0; }

private:

protected:

    JogAxis cAxis;
    JogDist cDist;
    uint32_t nextRefresh;
    uint32_t lastJogTime;
    
    static char axisChar(const JogAxis &a) {
        switch(a) {
            /*case JogAxis::X : return 'X';
            case JogAxis::Y : return 'Y';
            case JogAxis::Z : return 'Z';*/
            case 0 : return 'X';
            case 1 : return 'Y';
            case 2 : return 'Z';
        }
        S_DEBUGF("Unknown axis\n");
        return 0;
    }


    static float distVal(const JogDist &a) {
        switch(a) {
            case 0: return 0.1;
            case 1: return 1;
            case 2: return 10;
        }
        S_DEBUGF("Unknown multiplier\n");
        return 1;
    }

    void drawAxis(char axis, float v, int x, int y) {
        static const int LEN=13;
        static char str[LEN];
        
        snprintf(str, LEN, "%c% 8.3f", axis, v );
        Display::u8g2.drawStr(x, y, str );
        //u8g2.drawGlyph();
    }

    void drawContents() = 0;

    void onButton(int bt, int8_t arg) = 0;


};