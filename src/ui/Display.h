#pragma once

#include <Arduino.h>

#include <U8g2lib.h>

#include <etl/vector.h>
#include <functional>


#include "../devices/GCodeDevice.h"


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

class Screen;

class Display : public DeviceObserver {
public:
    static U8G2 &u8g2;
    //static bool buttonPressed[3];
    enum {
        BT_ZDOWN = 0,
        BT_ZUP,
        BT_R,
        BT_L,
        BT_CENTER,
        BT_UP,
        BT_DOWN,
        BT_STEP,
        N_BUTTONS
    } _butt;
    //static constexpr int N_BUTTONS = 8;
    static uint16_t buttStates;
    static const int STATUS_BAR_HEIGHT = 16;



    Display(): dirty{true} { 
        assert(inst==nullptr);
        inst=this; 
    }

    void setDirty(bool fdirty=true) { dirty=fdirty; }

    void notification(const DeviceStatusEvent &e) override {
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

    void processButtons();

    void drawStatusBar();
    void drawMenu() ;

    void ensureSelMenuVisible();

};