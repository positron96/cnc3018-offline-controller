#pragma once

#include <Arduino.h>

#include <U8g2lib.h>

#include <etl/vector.h>

#include "../devices/GCodeDevice.h"
#include "../InetServer.h"
#include "../Job.h"

#include "Display.h"

#define S_DEBUGF(...)  { Serial.printf(__VA_ARGS__); }
#define S_DEBUGFI(...)  { log_printf(__VA_ARGS__); }
#define S_DEBUGS(s)  { Serial.println(s); }



class Screen {
public:

    Screen() : firstDisplayedMenuItem(0) {}

    void setDirty(bool fdirty=true) { Display::getDisplay()->setDirty(fdirty); }

    virtual void begin() { setDirty(true); }

    virtual void loop() {}

    void draw();

protected:

    etl::vector<MenuItem, 10> menuItems;

    virtual void drawContents() = 0;

    virtual void onButtonPressed(Button bt, int8_t arg) {};

    virtual void onPotValueChanged(int pot, int val) {};

    //virtual void onMenuItemSelected(MenuItem & item) {};

    virtual void onShow() {};
    virtual void onHide() {};

private:

    int firstDisplayedMenuItem;

    friend class Display;

};