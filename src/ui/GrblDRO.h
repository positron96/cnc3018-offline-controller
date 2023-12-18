#pragma once

#include "DRO.h"
#include "devices/GrblDevice.h"

class GrblDRO : public DRO {
public:
    GrblDRO(GrblDevice &d) : DRO(d), dev(d) {}

    ~GrblDRO() {}

    void begin() override;

protected:
    void drawAxisCoords(int sx, int sy, u_int8_t lineHeight) override;

    GrblDevice &dev;
private:
    bool useWCS;
};