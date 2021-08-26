#pragma once

#include <Arduino.h>

#include <U8g2lib.h>

#include <etl/vector.h>
#include <functional>


#include "../devices/GCodeDevice.h"
#include "../InetServer.h"
#include "../Job.h"


struct MenuItem {
    int16_t id;
    uint16_t glyph;
    bool togglalbe;
    bool on;
    uint8_t * font;
    using ItemFunc = std::function<void(MenuItem &)>;
    ItemFunc onCmd;
    ItemFunc offCmd;
    static MenuItem simpleItem(int16_t id, uint16_t glyph, ItemFunc func) {
        return MenuItem{id, glyph, false, false, nullptr, func};
    }
};

enum class Button {
    ENC_UP, ENC_DOWN, BT1, BT2, BT3
};

class Screen;

class Display : public JobObserver, public DeviceObserver, public WebServerObserver {
public:
    static U8G2 &u8g2;
    static bool buttonPressed[3];
    static int encVal;
    static int potVal[2];
    static const int STATUS_BAR_HEIGHT = 9;

    Display() { 
        assert(inst==nullptr);
        inst=this; 
    }

    void setDirty(bool fdirty=true) { dirty=fdirty; }

    void notification(JobStatusEvent e) override {
        setDirty();
    }
    void notification(const DeviceStatusEvent &e) override {
        setDirty();
    }
    void notification(const WebServerStatusEvent &e) override {
        setDirty();
    }

    void begin() { dirty=true; }

    void loop();

    void draw();

    void setScreen(Screen *screen) ;    

    static Display *getDisplay();


private:

    static Display *inst;

    Screen *cScreen;

    bool dirty;

    int selMenuItem=0;

    void processInput();

    void processEnc();
    void processButtons();
    void processPot();   

    void drawStatusBar();
    void drawMenu() ;

    void ensureSelMenuVisible();

};