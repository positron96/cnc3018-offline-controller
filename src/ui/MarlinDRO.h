#pragma once

#include "DRO.h"
#include "devices/MarlinDevice.h"

class MarlinDRO : public DRO {

public:
    MarlinDRO(MarlinDevice &d) : DRO(d) , dev(d) {}

    ~MarlinDRO() {}

    void begin() override;

protected:
    MarlinDevice &dev;
private:
    bool relative;

};