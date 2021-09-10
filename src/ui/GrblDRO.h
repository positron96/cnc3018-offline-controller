#pragma once

#include "DRO.h"
#include "../devices/GrblDevice.h"

class GrblDRO : public DRO {

public:
    GrblDRO() : cMode{Mode::AXES} {}
    void begin() override ;

protected:
    
    void drawContents() override;

    void onButton(int bt, Evt evt) override;

private:
    
    enum class Mode {
        AXES, SPINDLE
    };
    Mode cMode;
    bool buttonWasPressedWithShift;
    bool useWCS;

    void onButtonAxes(int bt, Evt evt, GrblDevice *);
    void onButtonShift(int bt, Evt evt, GrblDevice *);
    
};