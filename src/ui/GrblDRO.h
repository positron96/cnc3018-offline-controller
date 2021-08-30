#pragma once

#include "DRO.h"

class GrblDRO : public DRO {

public:
    GrblDRO() : cMode{Mode::AXES} {}
    void begin() override ;

protected:

    using Evt = Display::ButtonEvent;
    
    void drawContents() override;

    void onButton(int bt, Evt evt) override;

private:
    
    enum class Mode {
        AXES, SPINDLE
    };
    Mode cMode;
    bool buttonWasPressedWithShift;

    void onButtonAxes(int bt, Evt evt);
    void onButtonShift(int bt, Evt evt);
    
};