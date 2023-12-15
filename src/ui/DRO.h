#pragma once

#include <etl/map.h>
#include <printfloat.h>

#include "constants.h"
#include "Screen.h"
#include "devices/GCodeDevice.h"
#include "FileChooser.h"

extern FileChooser fileChooser;

constexpr int LINE_HEIGHT = 11;

using JogDist = unsigned int;

class DRO : public Screen {
public:
    constexpr static uint16_t REFRESH_INTL = 500;

    DRO(GCodeDevice &d) : dev(d), nextRefresh{1}, cDist{1}, cFeed{0}, cMode{Mode::AXES} {}

    virtual ~DRO() {}

    void begin() override {
        menuItems.push_back(MenuItem::simpleItem(0, "Open", [](MenuItem &) {
            Job &job = Job::getJob();
            if (job.isRunning()) return;
            Display::getDisplay()->setScreen(&fileChooser); // this will reset the card
        }));
        menuItems.push_back(MenuItem::simpleItem(1, "Pause job", [this](MenuItem &m) {
            Job &job = Job::getJob();
            if (!job.isRunning())
                return;
            job.setPaused(!job.isPaused());
            m.text = job.isPaused()
                     ? "Resume job"
                     : "Pause job";
            setDirty(true);
        }));
    };

    void loop() override {
        if (nextRefresh != 0 && millis() > nextRefresh) {
            nextRefresh = millis() + REFRESH_INTL;
            dev.requestStatusUpdate();
        }
    }

    void enableRefresh(bool r = true) { nextRefresh = r ? millis() : 0; }

    bool isRefreshEnabled() const { return nextRefresh != 0; }

protected:
    enum class Mode {
        AXES, SPINDLE
    };
    GCodeDevice &dev;

    uint32_t nextRefresh;
    constexpr static float JOG_DISTS[] = {0.1, 1, 5, 10, 50};
    constexpr static size_t N_JOG_DISTS = sizeof(JOG_DISTS) / sizeof(JOG_DISTS[0]);
    JogDist cDist;
    constexpr static int JOG_FEEDS[] = {50, 100, 500, 1000, 2000};
    constexpr static size_t N_JOG_FEEDS = sizeof(JOG_FEEDS) / sizeof(JOG_FEEDS[0]);
    size_t cFeed;
    constexpr static int SPINDLE_VALS[] = { 0, 1, 64, 128, 255};
    constexpr static size_t N_SPINDLE_VALS = sizeof(SPINDLE_VALS) / sizeof(SPINDLE_VALS[0]);
    size_t cSpindleVal;

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

    virtual void drawAxisCoords(int sx, int sy) {
        drawAxis(AXIS[0], dev.getX(), sx, sy);
        drawAxis(AXIS[1], dev.getY(), sx, sy + LINE_HEIGHT);
        drawAxis(AXIS[2], dev.getZ(), sx, sy + LINE_HEIGHT * 2);
    };

    void onButton(int bt, Evt arg) override;

    void onButtonAxes(int bt, Evt evt);

    void onButtonShift(int bt, Evt evt);

private:

};
