#pragma once

#include <Arduino.h>

#include <U8g2lib.h>

#include <etl/vector.h>
#include <etl/map.h>
#include <printfloat.h>

#include "devices/GCodeDevice.h"
#include "Display.h"


class Screen {
public:
    Screen(): firstDisplayedMenuItem{0} {}
    virtual ~Screen(){}

    static void setDirty(bool fdirty = true) {
        Display::getDisplay()->setDirty(fdirty);
    }

    virtual void begin() { setDirty(true); }

    virtual void step() {}

protected:

    using Evt = Display::ButtonEvent;

    etl::vector<MenuItem, 10> menuItems;

    virtual void drawContents() = 0;

    virtual void onButton(int bt, Display::ButtonEvent arg) {};

    virtual void onMenuItemSelected(MenuItem &item) {};

    virtual void onShow() {};

    virtual void onHide() {};

private:

    size_t firstDisplayedMenuItem;

    friend class Display;
};