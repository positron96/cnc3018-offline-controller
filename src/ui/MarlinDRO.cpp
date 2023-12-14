#include "MarlinDRO.h"
#include "gcode/gcode.h"

void MarlinDRO::begin() {
    DRO::begin();
    menuItems.push_back(MenuItem::simpleItem(2, "Home", [this](MenuItem &) {
        dev.scheduleCommand(G28_START_HOMING);
    }));
    menuItems.push_back(MenuItem::simpleItem(3, "Set XY to 0", [this](MenuItem &) {
        dev.scheduleCommand("G92 X0Y0");
    }));
    menuItems.push_back(MenuItem::simpleItem(4, "Relative", [this](MenuItem &m) {
        relative = !relative;
        m.text = relative ? "Abs" : "Relative";
        dev.scheduleCommand(relative ? G90_SET_ABS_COORDINATES : G91_SET_RELATIVE_COORDINATES);
    }));
    menuItems.push_back(MenuItem::simpleItem(5, "Goto XY=0", [this](MenuItem &) {
        dev.scheduleCommand("G0 X0Y0");
    }));
}
