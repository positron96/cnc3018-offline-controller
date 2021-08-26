#pragma once

#include <ArduinoJson.h> 
#include <etl/map.h>

#include "Screen.h"

#include "../devices/GCodeDevice.h"
#include "../Job.h"


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
            nextRefresh = millis() + 500;
            GCodeDevice *dev = GCodeDevice::getDevice();
            if (dev!=nullptr) {
                dev->requestStatusUpdate();
                //setDirty();
            }
        }
    }

    void enableRefresh(bool r) { nextRefresh = r?millis() : 0;  }
    bool isRefreshEnabled() { return nextRefresh!=0; }

/*
    void config(JsonObjectConst cfg) {
        for (JsonPairConst kv : cfg) {
            etl::map<String, String, 10> devMenu{};
            S_DEBUGF("Device menu %s\n", kv.key().c_str() );
            for (JsonPairConst cmd : kv.value().as<JsonObject>() ) {
                devMenu[ String(cmd.key().c_str()) ] = cmd.value().as<String>();
                S_DEBUGF(" %s=%s\n", cmd.key().c_str(), cmd.value().as<char*>()); 
                allMenuItems[ String( kv.key().c_str() )] = devMenu;
                if( devMenu.available()==0) break;
            }
            if( allMenuItems.available()==0) break;
        }

    }

    void setDevice(GCodeDevice *dev) {
        S_DEBUGF("setDevice %s\n", dev->getType() );
        String t = dev->getType();
        if( allMenuItems.find(t) == allMenuItems.end() ) {
            S_DEBUGF("Could not find type %s\n", t.c_str() );
            return;
        }
        devMenu = &allMenuItems[t];
        for(auto r: *devMenu) {
            S_DEBUGF("Adding menu %s\n", r.first.c_str() );
            menuItems.push_back(r.first);
            if(menuItems.available()==0) break;
        }
    } */

private:
    //etl::imap<String,String> * devMenu;

    //etl::map<String, etl::map<String,String, 10>, 3> allMenuItems;

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
        log_printf("Unknown axis\n");
        return 0;
    }


    static float distVal(const JogDist &a) {
        switch(a) {
            case 0: return 0.1;
            case 1: return 1;
            case 2: return 10;
        }
        log_printf("Unknown multiplier\n");
        return 1;
    }

    void drawAxis(char axis, float v, int y) {
        const int LEN=20;
        char str[LEN];
        
        snprintf(str, LEN, "%c% 8.3f", axis, v );
        Display::u8g2.drawStr(1, y, str );
        //u8g2.drawGlyph();
    }

    void drawContents() override;

    void onPotValueChanged(int pot, int v) override;

    void onButtonPressed(Button bt, int8_t arg) override;


};