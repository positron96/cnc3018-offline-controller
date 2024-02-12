#pragma once

#include <Arduino.h>
#include "constants.h"

#include "devices/GCodeDevice.h"
#include "Job.h"

#include <U8g2lib.h>

#include <etl/vector.h>
#include <functional>

class Screen;


struct MenuItem {
    int16_t id;
    String text;
    bool togglable;
    bool on;
    uint8_t* font;
    using ItemFunc = std::function<void(MenuItem&)>;
    ItemFunc cmd;

    static MenuItem simpleItem(int16_t id, const char* text, ItemFunc func) {
        return MenuItem{id, text, false, false, nullptr, func};
    }
};


class Display : public JobObserver, public DeviceObserver {
public:
    static constexpr int STATUS_BAR_HEIGHT = 16;
    static constexpr int HOLD_COUNT = 30; // x10 = ms

    static U8G2& u8g2;

    enum {
        BT_ZDOWN = 0,  //
        BT_ZUP,        //
        //                    +--------------------------------------------------------+
        BT_R,          //     |  [Z_UP] +--------------+          [BT_UP]              |
        BT_L,          //     |         |              |                               |
        BT_CENTER,     //     |         |              |  [BT_L] [BT_CENTER]  [BT_R]   |
        BT_UP,         //     |         |              |                               |
        BT_DOWN,       //     | [Z_DOWN]+--------------+          [BT_DOWN]  [BT_STEP] |
        BT_STEP,       //     +--------------------------------------------------------+
        //
        N_BUTTONS
    };

    static uint16_t buttStates;

    enum class ButtonEvent {
        UP, DOWN, HOLD
    };

    Display(Job& job) : job(job), dirty{true}, selMenuItem{0}, menuShown{false} {
        assert(inst == nullptr);
        inst = this;
    }

    ~Display() {}

    void setDirty(bool fdirty = true) { dirty = fdirty; }

    void notification(const DeviceStatusEvent& e) override {
        setDirty();
    }

    void notification(const JobStatusEvent e) override {
        // TODO use events to update screen
        setDirty();
    }

    void begin() { dirty = true; }
    ///
    /// call in each main loop step
    ///
    void step();

    void draw();

    void setScreen(Screen* screen);

    void setDevice(GCodeDevice* dev);

    static Display* getDisplay();

    void processInput();

private:
    static Display* inst;
    Screen* cScreen;
    GCodeDevice* dev;
    Job& job;

    bool dirty;
    size_t selMenuItem = 0;
    bool menuShown;
    bool menuShownWhenDown;

    decltype(buttStates) prevStates;

    int16_t holdCounter[N_BUTTONS];

    void processButtons();

    void processMenuButton(uint8_t bt, ButtonEvent evt);

    void drawStatusBar();

    void drawMenu();

    void ensureSelMenuVisible();

};