#pragma once

#include <Arduino.h>
#include "Screen.h"
#include "Display.h"

class ZProbeSizeScreen : public Screen {
public:

    void begin(Screen *returnScreen);
    void drawContents() override;
    void onButton(int bt, Evt evt) override;

    float getProbeSize() const {
        return probeSize100 / 100.0f;
    }

    uint16_t getProbeSize100() const {
        return probeSize100;
    }

    void setProbeSize(float value) {

        int v = (int)(value * 100.0f + 0.5f);

        if(v < 0)
            v = 0;

        if(v > 9999)
            v = 9999;

        probeSize100 = v;

        setDirty();
    }

private:

    // 19.19 mm = 1919 centésimos
    uint16_t probeSize100 = 1919;

    Screen *returnScreen = nullptr;
};