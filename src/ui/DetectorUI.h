#pragma once

#include "Screen.h"

template<class Detector>
class DetectorScreen: public Screen {
public:
    static constexpr uint16_t REFRESH_INTL = 500;

    DetectorScreen(): nextRefresh{1} {}
    
    void begin() override {
    };

    void loop() override {
        Screen::loop();
        if(nextRefresh!=0 && millis()>nextRefresh) {
            nextRefresh = millis() + REFRESH_INTL;
            setDirty();
        }
    }

    void enableRefresh(bool r) { nextRefresh = r ? millis() : 0;  }
    bool isRefreshEnabled() { return nextRefresh!=0; }

private:

protected:
    uint32_t nextRefresh;
    
    void drawContents() override {
        U8G2 &u8g2 = Display::u8g2;   

        const int LEN = 20;
        char str[LEN];

        int sx = 20;
        int sy = Display::STATUS_BAR_HEIGHT+5;
        constexpr int lh = 11;
        u8g2.setFont(u8g2_font_7x13B_tr );
        if(Detector::getDetectResult()==0) {
            u8g2.drawStr(sx, sy, "Searching...");
            snprintf(str, LEN, "Trying speed %ld", Detector::serialBaud);
            u8g2.drawStr(sx, sy+lh, str);
        } else {
            u8g2.drawStr(sx, sy, "Found");
        }
    };

    void onButton(int bt, Display::ButtonEvent arg) override {};


};