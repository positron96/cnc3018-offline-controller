#pragma once

#include <Arduino.h>

#include <U8g2lib.h>

#include <etl/vector.h>

#include "../devices/GCodeDevice.h"

#include "Display.h"

#include "../debug.h"

#define S_DEBUGF   LOGF
#define S_DEBUGS   LOGLN



class Screen {
public:

    Screen() : firstDisplayedMenuItem(0) {}

    void setDirty(bool fdirty=true) { Display::getDisplay()->setDirty(fdirty); }

    virtual void begin() { setDirty(true); }

    virtual void loop() {}

    void draw();

protected:

    using Evt = Display::ButtonEvent;

    etl::vector<MenuItem, 10> menuItems;

    virtual void drawContents() = 0;

    virtual void onButton(int bt, Display::ButtonEvent arg) {};

    //virtual void onMenuItemSelected(MenuItem & item) {};

    virtual void onShow() {};
    virtual void onHide() {};

private:

    int firstDisplayedMenuItem;

    friend class Display;

};