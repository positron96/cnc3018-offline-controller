#pragma once

#include "DRO.h"
#include "devices/MarlinDevice.h"

class MarlinDRO : public DRO {

public:
    MarlinDRO(MarlinDevice &d) : DRO(d) {}

    ~MarlinDRO() {}

    void begin() override;

protected:

private:
    bool relative;

};