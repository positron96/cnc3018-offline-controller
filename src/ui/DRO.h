#pragma once

#include <etl/map.h>
#include <printfloat.h>

#include "constants.h"
#include "Screen.h"
#include "devices/GCodeDevice.h"
#include "FileChooser.h"
#include <vector>

extern FileChooser fileChooser;
extern Job job;

constexpr int LINE_HEIGHT = 11;

using JogDist = unsigned int;

class DRO : public Screen {
public:
    constexpr static uint16_t REFRESH_INTL = 500;

    DRO(GCodeDevice &d)
            : dev(d), nextRefresh{1}, cDist{3}, cFeed{3}, cMode{Mode::AXES} {
    }

    virtual ~DRO() {}

    void begin() override {
        menuItems.push_back(MenuItem::simpleItem(0, "Open", [](MenuItem &) {
            if (job.isRunning()) return;
            Display::getDisplay()->setScreen(&fileChooser); // this will reset the card
        }));
        menuItems.push_back(MenuItem::simpleItem(1, "Pause job", [this](MenuItem &m) {
            if (job.isRunning()) {
                job.setPaused(true);
                m.text = "Resume job";
            } else {
                job.setPaused(false);
                m.text = "Pause job";
            }
            setDirty(true);
        }));
    };

    void step() override {
        if (nextRefresh != 0 && millis() > nextRefresh) {
            nextRefresh = millis() + REFRESH_INTL;
            dev.requestStatusUpdate();
        }
    }

    void enableRefresh(bool r = true) { nextRefresh = r ? millis() : 0; }

    bool isRefreshEnabled() const { return nextRefresh != 0; }

protected:
    enum class Mode : uint8_t {
        // 0    1      2
        AXES, SPINDLE, TEMP, N_VALS
    };
    GCodeDevice &dev;

    uint32_t nextRefresh;
    constexpr static float JOG_DISTS[] = {0.1, 0.5, 1, 5, 10, 50};
    constexpr static size_t N_JOG_DISTS = sizeof(JOG_DISTS) / sizeof(JOG_DISTS[0]);
    JogDist cDist;
    constexpr static int JOG_FEEDS[] = {50, 100, 500, 1000, 2000};
    constexpr static size_t N_JOG_FEEDS = sizeof(JOG_FEEDS) / sizeof(JOG_FEEDS[0]);
    size_t cFeed;
    size_t cSpindleVal;
    std::vector<int>* devSpindleValues;

    Mode cMode;
    bool buttonWasPressedWithShift;

    void drawAxis(char axis, float value, int x, int y) {
        static const int LEN = 13;
        static char buffer[LEN];

        buffer[0] = axis;
        snprintfloat(buffer + 1, LEN - 1, value, 2, 7);
        Display::u8g2.drawStr(x, y, buffer);
    }

    void drawContents() override;

    virtual void drawAxisCoords(int sx, int sy, u_int8_t lineHeight) {
        drawAxis(AXIS[0], dev.getX(), sx, sy);
        drawAxis(AXIS[1], dev.getY(), sx, sy + lineHeight);
        drawAxis(AXIS[2], dev.getZ(), sx, sy + lineHeight * 2);
    };

    void onButton(int bt, Evt arg) override;

    void onButtonAxes(int bt, Evt evt);

    void onButtonShift(int bt, Evt evt);

private:

};
